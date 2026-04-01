/*
 * GUI Manager implementation using Dear ImGui
 */

#include "gui.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "compiler/compiler/compiler.h"
#include "camera/camera.h"

namespace {
constexpr size_t SHADER_BUFFER_SIZE = 65536;

std::filesystem::path buildTempShaderPath(const std::string &shaderPath) {
  if (shaderPath.empty()) {
    return std::filesystem::temp_directory_path() / "shader_editor_temp.glsl";
  }

  const std::filesystem::path sourcePath(shaderPath);
  const std::string tempName =
      "." + sourcePath.filename().string() + ".live_preview.glsl";
  return sourcePath.parent_path() / tempName;
}

/**
 * Keeps ImGui's editable text buffer synchronized with Gui::shaderSource_.
 * ImGui calls this resize callback whenever the current string capacity is no
 * longer sufficient, allowing the std::string storage to grow and the internal
 * buffer pointer to be rebound safely.
 */
int ShaderSourceInputTextCallback(ImGuiInputTextCallbackData *data) {
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
    auto *shaderSource = static_cast<std::string *>(data->UserData);
    shaderSource->resize(static_cast<size_t>(data->BufTextLen));
    data->Buf = shaderSource->data();
  }

  return 0;
}
} // namespace

Gui::Gui() {}

Gui::~Gui() { shutdown(); }

bool Gui::initialize(GLFWwindow *window) {
  window_ = window;

  // Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::cerr << "Failed to initialize ImGui GLFW backend\n";
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init("#version 330")) {
    std::cerr << "Failed to initialize ImGui OpenGL3 backend\n";
    return false;
  }

  return true;
}

void Gui::shutdown() {
  if (window_) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    window_ = nullptr;
  }
}

void Gui::beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void Gui::endFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::render() {
  drawEditorPanel();
  drawStatusBar();
}

bool Gui::wantsKeyboardInput() const {
  ImGuiIO const &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

bool Gui::wantsMouseInput() const {
  ImGuiIO const &io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

void Gui::setCompiler(Compiler *compiler) { compiler_ = compiler; }

void Gui::setCamera(Camera *camera) { camera_ = camera; }

void Gui::setShaderPath(const std::string &path) { shaderPath_ = path; }

std::string Gui::getShaderSource() const { return shaderSource_; }

bool Gui::loadShaderFromFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    compileError_ = "Failed to open shader file: " + path;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  std::string content = buffer.str();
  if (content.size() >= SHADER_BUFFER_SIZE - 1) {
    compileError_ = "Shader file too large (max 64KB)";
    return false;
  }

  shaderSource_ = content;
  shaderPath_ = path;
  editorModified_ = false;
  compileError_.clear();

  return true;
}

bool Gui::hasNewShader() const { return hasNewShader_; }

unsigned int Gui::getCompiledShader() const { return compiledShader_; }

void Gui::clearNewShaderFlag() { hasNewShader_ = false; }

void Gui::drawEditorPanel() {
  // Fixed left panel for shader editor
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);

  ImGui::Begin("Shader Editor", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

  // Shader file path display
  if (!shaderPath_.empty()) {
    ImGui::Text("Editing: %s", shaderPath_.c_str());
    ImGui::Separator();
  }

  // Editor text area
  ImGui::Text("Edit shader code (supports #include directives):");
  ImVec2 availableSize = ImGui::GetContentRegionAvail();
  availableSize.y -= 60; // Reserve space for button and status

  shaderSource_.reserve(SHADER_BUFFER_SIZE - 1);
  if (ImGui::InputTextMultiline("##shader_editor", shaderSource_.data(),
                                shaderSource_.capacity() + 1, availableSize,
                                ImGuiInputTextFlags_AllowTabInput |
                                    ImGuiInputTextFlags_CallbackResize,
                                ShaderSourceInputTextCallback,
                                &shaderSource_)) {
    editorModified_ = true;
  }

  // Compile button
  ImGui::Separator();
  if (ImGui::Button("Compile and Run", ImVec2(120, 30))) {
    tryCompileShader();
  }
  ImGui::SameLine();
  if (editorModified_) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Modified");
  } else {
    ImGui::Text("Ready");
  }

  ImGui::End();
}

void Gui::drawStatusBar() {
  ImGui::SetNextWindowPos(ImVec2(0, 700), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(500, 100), ImGuiCond_FirstUseEver);

  ImGui::Begin("Status", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

  // Camera info
  if (camera_) {
    ImGui::Text("Camera: Center (%.6f, %.6f), Zoom: %.6f",
                static_cast<double>(camera_->get_x()),
                static_cast<double>(camera_->get_y()),
                static_cast<double>(camera_->get_zoom()));
  }

  // Compilation errors
  if (!compileError_.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Compilation Error:");
    ImGui::TextWrapped("%s", compileError_.c_str());
  } else {
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                       "Shader compiled successfully");
  }

  ImGui::End();
}

void Gui::tryCompileShader() {
  if (!compiler_) {
    compileError_ = "No compiler set";
    return;
  }

  tempShaderPath_ = buildTempShaderPath(shaderPath_).string();

  // Save editor content to temp file
  if (!saveEditorToTempFile(tempShaderPath_)) {
    return;
  }

  // Compile the temp file
  Compiler::CompileOutput result = compiler_->compile(
      tempShaderPath_, 0x8B30, true); // GL_FRAGMENT_SHADER = 0x8B30

  if (result.success) {
    compileError_.clear();
    hasNewShader_ = true;
    compiledShader_ = result.shader;
    editorModified_ = false;
  } else {
    compileError_ = result.error;
    hasNewShader_ = false;
  }
}

bool Gui::saveEditorToTempFile(const std::string &tempPath) {
  std::ofstream file(tempPath);
  if (!file.is_open()) {
    compileError_ = "Failed to create temp file: " + tempPath;
    return false;
  }

  file.write(shaderSource_.data(),
             static_cast<std::streamsize>(shaderSource_.size()));
  file.close();

  return true;
}
