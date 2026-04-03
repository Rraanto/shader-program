/*********************************************************************
 * App.h
 *
 * Main application class that encapsulates the GLFW window, OpenGL
 * context, shader visualization, and code editor integration.
 *
 * Responsibilities:
 * - Window and OpenGL context management
 * - Shader program lifecycle (compile, link, hot-reload)
 * - 2D camera state and input handling
 * - Integration with Gui (ImGui editor) and Compiler
 *
 * Usage:
 *   App app;
 *   if (!app.initialize()) { return 1; }
 *   app.run();
 *********************************************************************/

#ifndef APP_H
#define APP_H

#include <memory>
#include <string>

// Forward declarations
struct GLFWwindow;
class Gui;
struct Camera;
struct Compiler;

/**
 * Main application class managing the shader editor application lifecycle.
 *
 * The App class owns all subsystems and coordinates between:
 * - GLFW/OpenGL (window and rendering)
 * - Gui (ImGui-based shader editor)
 * - Camera (2D view navigation)
 * - Compiler (GLSL shader compilation)
 */
class App {
public:
  /**
   * Construct App with uninitialized state.
   * Call initialize() before run().
   */
  App();

  /**
   * Destructor automatically calls shutdown() for cleanup.
   */
  ~App();

  // Disallow copy and move
  App(const App &) = delete;
  App &operator=(const App &) = delete;
  App(App &&) = delete;
  App &operator=(App &&) = delete;

  /**
   * Initialize all subsystems.
   *
   * @param width Window width in pixels
   * @param height Window height in pixels
   * @param title Window title
   * @return true on success, false on failure (error logged to stderr)
   */
  bool initialize(int width = 1000, int height = 800,
                  const char *title = "Shader Editor");

  /**
   * Run the main application loop. Blocks until window is closed.
   * Must call initialize() first.
   */
  void run();

  /**
   * Explicit shutdown and cleanup. Called automatically by destructor.
   * Safe to call multiple times.
   */
  void shutdown();

private:
  /**
   * Initialize GLFW and create window.
   * @return true on success
   */
  bool init_glfw();

  /**
   * Initialize OpenGL via GLAD.
   * @return true on success
   */
  bool init_opengl();

  /**
   * Load and compile shaders, create shader program.
   * @return true on success
   */
  bool init_shaders();

  /**
   * Create VAO/VBO for full-screen quad.
   * @return true on success
   */
  bool init_geometry();

  /**
   * Initialize Gui subsystem (ImGui).
   * @return true on success
   */
  bool init_gui();

  /**
   * Initialize Camera with default position.
   */
  void init_camera();

  /**
   * Process input for this frame.
   * Called before rendering.
   */
  void process_input();

  /**
   * Update shader uniforms for this frame.
   * @param time Current time in seconds
   */
  void update_uniforms(float time);

  /**
   * Render the frame: shader viewport and GUI.
   */
  void render();

  /**
   * Check if Gui has new compiled shader and hot-reload it.
   * Called each frame.
   */
  void try_hot_reload();

  /**
   * Link vertex and fragment shaders into a program.
   *
   * @param fragment_shader Compiled fragment shader
   * @param out_program Output program ID
   * @return true on success, false on failure
   */
  bool link_shader_program(unsigned int fragment_shader,
                           unsigned int &out_program);

  /**
   * Cleanup shader program and shader objects.
   */
  void cleanup_shader_program();

  /**
   * Cleanup OpenGL geometry buffers.
   */
  void cleanup_geometry();

  // GLFW callbacks - static trampolines
  static void error_callback(int error, const char *description);
  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods);
  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods);
  static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos);
  static void scroll_callback(GLFWwindow *window, double xoffset,
                              double yoffset);

  // Instance callback handlers
  void on_key(int key, int scancode, int action, int mods);
  void on_mouse_button(int button, int action, int mods);
  void on_cursor_pos(double xpos, double ypos);
  void on_scroll(double xoffset, double yoffset);

  // Window state
  GLFWwindow *_window = nullptr;
  int _window_width = 1000;
  int _window_height = 800;

  // Subsystems
  std::unique_ptr<Gui> _gui;
  std::unique_ptr<Camera> _camera;
  std::unique_ptr<Compiler> _compiler;

  // OpenGL state
  unsigned int _shader_program = 0;
  unsigned int _vertex_shader = 0; // Kept for hot-reload relinking
  unsigned int _vao = 0;
  unsigned int _vbo = 0;
  int _vertex_count = 0;

  // Shader paths
  std::string _vertex_shader_path;
  std::string _fragment_shader_path;

  // Input state
  struct {
    bool _move_right = false;
    bool _move_left = false;
    bool _move_up = false;
    bool _move_down = false;
    bool _zoom_in = false;
    bool _zoom_out = false;
  } _keys;

  struct {
    bool _is_dragging = false;
    double _last_x = 0.0;
    double _last_y = 0.0;
  } _mouse;

  // Camera movement constants
  float _camera_stride = 0.5f; // per-frame camera movement
  float _zoom_stride = 0.1f;   // zoom amount per keypress

  bool _running = false;
};

#endif // APP_H
