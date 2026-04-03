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
  _window = window;

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
  if (_window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    _window = nullptr;
  }
}

void Gui::begin_frame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void Gui::end_frame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::render() {
  _draw_editor_panel();
  _draw_status_bar();
}

bool Gui::wants_keyboard_input() const {
  ImGuiIO const &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

bool Gui::wants_mouse_input() const {
  ImGuiIO const &io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

void Gui::set_compiler(Compiler *compiler) { _compiler = compiler; }

void Gui::set_camera(Camera *camera) { _camera = camera; }

void Gui::set_shader_path(const std::string &path) { _shader_path = path; }

std::string Gui::get_shader_source() const { return _shader_source; }

bool Gui::load_shader_from_file(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    _compiler_error = "Failed to open shader file: " + path;
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  std::string content = buffer.str();
  if (content.size() >= SHADER_BUFFER_SIZE - 1) {
    _compiler_error = "Shader file too large (max 64KB)";
    return false;
  }

  _shader_source = content;
  _shader_path = path;
  _editor_modified = false;
  _compiler_error.clear();

  return true;
}

bool Gui::has_new_shader() const { return _has_new_shader; }

unsigned int Gui::get_compiled_shader() const { return _compiler_shader; }

void Gui::clear_new_shader_flag() { _has_new_shader = false; }

void Gui::_draw_editor_panel() {
  // Fixed left panel for shader editor
  ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);

  ImGui::Begin("Shader Editor", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

  // Shader file path display
  if (!_shader_path.empty()) {
    ImGui::Text("Editing: %s", shaderPath_.c_str());
    ImGui::Separator();
  }

  // Editor text area
  ImGui::Text("Edit shader code (supports #include directives):");
  ImVec2 availableSize = ImGui::GetContentRegionAvail();
  availableSize.y -= 60; // Reserve space for button and status

  _shader_source.reserve(SHADER_BUFFER_SIZE - 1);
  if (ImGui::InputTextMultiline("##shader_editor", shaderSource_.data(),
                                shaderSource_.capacity() + 1, availableSize,
                                ImGuiInputTextFlags_AllowTabInput |
                                    ImGuiInputTextFlags_CallbackResize,
                                ShaderSourceInputTextCallback,
                                &shaderSource_)) {
    _editor_modified = true;
  }

  // Compile button
  ImGui::Separator();
  if (ImGui::Button("Compile and Run", ImVec2(120, 30))) {
    _try_compile_shader();
  }
  ImGui::SameLine();
  if (_editor_modified) {
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "* Modified");
  } else {
    ImGui::Text("Ready");
  }

  ImGui::End();
}

void Gui::_draw_status_bar() {
  ImGui::SetNextWindowPos(ImVec2(0, 700), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(500, 100), ImGuiCond_FirstUseEver);

  ImGui::Begin("Status", nullptr,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);

  // Camera info
  if (_camera) {
    ImGui::Text("Camera: Center (%.6f, %.6f), Zoom: %.6f",
                static_cast<double>(camera_->get_x()),
                static_cast<double>(camera_->get_y()),
                static_cast<double>(camera_->get_zoom()));
  }

  // Compilation errors
  if (!_compiler_error.empty()) {
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Compilation Error:");
    ImGui::TextWrapped("%s", compileError_.c_str());
  } else {
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                       "Shader compiled successfully");
  }

  ImGui::End();
}

void Gui::_try_compile_shader() {
  if (!_compiler) {
    _compiler_error = "No compiler set";
    return;
  }

  _tmp_shader_path = buildTempShaderPath(_shader_path).string();

  // Save editor content to temp file
  if (!_save_editor_to_temp_file(_tmp_shader_path)) {
    return;
  }

  // Compile the temp file
  Compiler::CompileOutput result = _compiler->compile(
      tempShaderPath_, 0x8B30, true); // GL_FRAGMENT_SHADER = 0x8B30

  if (result.success) {
    _compiler_error.clear();
    _has_new_shader = true;
    _compiler_shader = result.shader;
    _editor_modified = false;
  } else {
    _compiler_error = result.error;
    _has_new_shader = false;
  }
}

bool Gui::_save_editor_to_temp_file(const std::string &tempPath) {
  std::ofstream file(tempPath);
  if (!file.is_open()) {
    _compiler_error = "Failed to create temp file: " + tempPath;
    return false;
  }

  file.write(_shader_source.data(),
             static_cast<std::streamsize>(_shader_source.size()));
  file.close();

  return true;
}
