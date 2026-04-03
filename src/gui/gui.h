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
  void begin_frame();

  // End frame and render - call before swap buffers
  void end_frame();

  // Render all GUI panels
  void render();

  // Check if ImGui wants to capture keyboard input
  bool wants_keyboard_input() const;

  // Check if ImGui wants to capture mouse input
  bool wants_mouse_input() const;

  // Set the compiler reference for shader compilation
  void set_compiler(Compiler *compiler);

  // Set the camera reference for status display
  void set_camera(Camera *camera);

  // Set the shader file path
  void set_shader_path(const std::string &path);

  // Get the current shader source from editor
  std::string get_shader_source() const;

  // Load shader from file into editor
  bool load_shader_from_file(const std::string &path);

  // Check if shader was successfully compiled this frame
  bool has_new_shader() const;

  // Get the compiled shader program ID (valid if hasNewShader() returns true)
  unsigned int get_compiled_shader() const;

  // Clear the new shader flag
  void clear_new_shader_flag();

private:
  void _draw_editor_panel();
  void _draw_status_bar();
  void _try_compile_shader();
  bool _save_editor_to_temp_file(const std::string &tempPath);

  // State
  GLFWwindow *_window = nullptr;
  Compiler *_compiler = nullptr;
  Camera *_camera = nullptr;

  std::string _shader_source;   // Raw shader text with #includes
  std::string _shader_path;     // Path to current shader file
  std::string _compiler_error;  // Last error message
  std::string _tmp_shader_path; // Path to temp file for compilation

  // Compilation results
  bool _has_new_shader = false;
  unsigned int _compiler_shader = 0;

  // Editor state
  bool _editor_modified = false;
};

#endif
