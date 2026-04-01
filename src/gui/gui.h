/*
 * GUI Manager using Dear ImGui
 * Provides shader editor and status display
 */

#ifndef GUI_H
#define GUI_H

#include <string>

// Forward declarations
struct GLFWwindow;
struct Compiler;
struct Camera;

class Gui {
public:
  Gui();
  ~Gui();

  // Initialize ImGui with GLFW context
  bool initialize(GLFWwindow *window);

  // Cleanup ImGui resources
  void shutdown();

  // Begin a new frame - call at start of main loop iteration
  void beginFrame();

  // End frame and render - call before swap buffers
  void endFrame();

  // Render all GUI panels
  void render();

  // Check if ImGui wants to capture keyboard input
  bool wantsKeyboardInput() const;

  // Check if ImGui wants to capture mouse input
  bool wantsMouseInput() const;

  // Set the compiler reference for shader compilation
  void setCompiler(Compiler *compiler);

  // Set the camera reference for status display
  void setCamera(Camera *camera);

  // Set the shader file path
  void setShaderPath(const std::string &path);

  // Get the current shader source from editor
  std::string getShaderSource() const;

  // Load shader from file into editor
  bool loadShaderFromFile(const std::string &path);

  // Check if shader was successfully compiled this frame
  bool hasNewShader() const;

  // Get the compiled shader program ID (valid if hasNewShader() returns true)
  unsigned int getCompiledShader() const;

  // Clear the new shader flag
  void clearNewShaderFlag();

private:
  void drawEditorPanel();
  void drawStatusBar();
  void tryCompileShader();
  bool saveEditorToTempFile(const std::string &tempPath);

  // State
  GLFWwindow *window_ = nullptr;
  Compiler *compiler_ = nullptr;
  Camera *camera_ = nullptr;

  std::string shaderSource_;   // Raw shader text with #includes
  std::string shaderPath_;     // Path to current shader file
  std::string compileError_;   // Last error message
  std::string tempShaderPath_; // Path to temp file for compilation

  // Compilation results
  bool hasNewShader_ = false;
  unsigned int compiledShader_ = 0;

  // Editor state
  bool editorModified_ = false;
};

#endif
