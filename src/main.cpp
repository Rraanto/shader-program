#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>

#include <iostream>
#include <string>
#include <vector>

// compiler and preprocessor
#include "compiler/preprocessor/preprocessor.h"
#include "compiler/compiler/compiler.h"

#include "camera/camera.h"
#include "gui/gui.h"

static void error_callback(int error, const char *description);

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods); // openGL callback

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods);

static void cursor_position_callback(GLFWwindow *window, double xpos,
                                     double ypos);

static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

/* App states */
bool move_right = false;
bool move_up = false;
bool move_down = false;
bool move_left = false;
bool zoom_in = false;
bool zoom_out = false;

/* Mouse states */
bool is_dragging = false;
double last_mouse_x = 0.0;
double last_mouse_y = 0.0;

/* GUI and Camera */
Gui *g_gui = nullptr;
Camera *g_camera = nullptr;
int g_window_width = 1000;
int g_window_height = 800;

int main() {

  const int WIDTH = 1000, HEIGHT = 800;
  g_window_width = WIDTH;
  g_window_height = HEIGHT;

  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    std::cerr << "GLFW initialization failed\n";
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Shader Editor", nullptr, nullptr);

  /*
   * various window checks
   */
  if (!window) {
    std::cerr << "Failed to create GLFW window\n";
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  /*
   * Loading and compiling shaders
   */
  std::vector<std::filesystem::path> include_dirs = {
      std::filesystem::path(SHADERS_DIR)};
  Compiler compiler(include_dirs);

  std::filesystem::path v_shader_path =
      std::filesystem::path(SHADERS_DIR) / "vertex_shader.glsl";
  std::filesystem::path f_shader_path =
      std::filesystem::path(SHADERS_DIR) / "fragment_shader.glsl";

  // vertex shader
  Compiler::CompileOutput compile_output;

  compile_output = compiler.compile(v_shader_path, GL_VERTEX_SHADER);

  if (!compile_output.success) {
    std::cerr << compile_output.error << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  GLuint vertex_shader = compile_output.shader;

  // fragment shader
  compile_output = compiler.compile(f_shader_path, GL_FRAGMENT_SHADER, true);
  if (!compile_output.success) {
    std::cerr << compile_output.error << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  GLuint fragment_shader = compile_output.shader;

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);

  int success = 0;
  char info_log[512] = {0};
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
    std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << "\n";
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(shader_program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glUseProgram(shader_program);

  /*
   * Preparing the viewport: a single 2D rectangle (two triangles)
   */

  float vertices[] = {
      -1.0f, -1.0f, // down left
      1.0f,  -1.0f, // down right
      1.0f,  1.0f,  // up right

      -1.0f, -1.0f, // down left
      1.0f,  1.0f,  // up right
      -1.0f, 1.0f   // up left
  };
  const int _dimension = 2;
  const int _vertex_count =
      sizeof(vertices) / _dimension; // 1 vertex consists of a n-tuple of
                                     // coordinates

  // create memory on the GPU
  unsigned int VBO;
  glGenBuffers(1, &VBO);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  // copy vertices array to those memory
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, _vertex_count * _dimension, vertices,
               GL_STATIC_DRAW);

  // configure how OpenGL should interpret the memory
  glVertexAttribPointer(0, _dimension, GL_FLOAT, GL_FALSE,
                        _dimension * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // we use a single shader program for the whole run
  // so it's enough to set the prg once
  glUseProgram(shader_program);

  /*
   * Initialising a 2D scene
   */
  // coords of the seahorse valley

  // an interesting point
  float _p_x = -0.214268f;
  float _p_y = 0.652873f;
  Camera camera(_p_x, _p_y);
  camera.zoom_in(0.8f);

  // Store globals for callbacks
  g_camera = &camera;

  glfwSetWindowUserPointer(window, &camera);

  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetScrollCallback(window, scroll_callback);

  float _per_frame_camera_stride = 0.5f; // per frame camera stride
  float _zoom_in_out_stride = 0.1f;

  /*
   * Initialize GUI
   */
  Gui gui;
  if (!gui.initialize(window)) {
    std::cerr << "Failed to initialize GUI\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
  }

  gui.setCompiler(&compiler);
  gui.setCamera(&camera);
  gui.setShaderPath(f_shader_path.string());

  // Load initial shader into editor
  if (!gui.loadShaderFromFile(f_shader_path.string())) {
    std::cerr << "Warning: Could not load shader into editor\n";
  }

  g_gui = &gui;

  /*
   * main loop
   */
  while (!glfwWindowShouldClose(window)) {
    float time = (float)glfwGetTime();
    int w, h;
    glfwPollEvents();

    // Keyboard camera movement (only when GUI doesn't want keyboard)
    if (!gui.wantsKeyboardInput()) {
      float right_str = move_right ? _per_frame_camera_stride : 0.0f;
      float left_str = move_left ? -_per_frame_camera_stride : 0.0f;
      float down_str = move_down ? -_per_frame_camera_stride : 0.0f;
      float up_str = move_up ? _per_frame_camera_stride : 0.0f;
      camera.move(right_str + left_str, up_str + down_str);

      camera.zoom_in(zoom_in ? _zoom_in_out_stride : 0.0f);
      camera.zoom_out(zoom_out ? _zoom_in_out_stride : 0.0f);
    }

    glfwGetFramebufferSize(window, &w, &h);
    g_window_width = w;
    g_window_height = h;
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 0.1f); // background color;
    glClear(GL_COLOR_BUFFER_BIT);

    GLint loc_center_uniform = glGetUniformLocation(shader_program, "uCenter");
    GLint loc_scale_uniform = glGetUniformLocation(shader_program, "uScale");
    GLint loc_aspect_uniform = glGetUniformLocation(shader_program, "uAspect");
    GLint loc_time_uniform = glGetUniformLocation(shader_program, "uTime");

    glUniform2f(loc_center_uniform, camera.get_x(), camera.get_y());
    glUniform1f(loc_scale_uniform, camera.get_zoom() * camera.get_scale());
    glUniform1f(loc_aspect_uniform, (float)w / (float)h);
    glUniform1f(loc_time_uniform, time);

    // rendering
    glDrawArrays(GL_TRIANGLES, 0, _vertex_count);

    // Render GUI
    gui.beginFrame();
    gui.render();
    gui.endFrame();

    // Check if GUI has compiled a new shader
    if (gui.hasNewShader()) {
      GLuint new_fragment_shader = gui.getCompiledShader();

      // Create new shader program
      GLuint new_program = glCreateProgram();
      glAttachShader(new_program, vertex_shader);
      glAttachShader(new_program, new_fragment_shader);
      glLinkProgram(new_program);

      int link_success = 0;
      glGetProgramiv(new_program, GL_LINK_STATUS, &link_success);
      if (link_success) {
        // Switch to new program
        glDeleteProgram(shader_program);
        shader_program = new_program;
        glUseProgram(shader_program);

        // Update VAO binding
        glBindVertexArray(VAO);
      } else {
        // Link failed - clean up
        glDeleteProgram(new_program);
      }

      // Clean up the fragment shader
      glDeleteShader(new_fragment_shader);
      gui.clearNewShaderFlag();
    }

    glfwSwapBuffers(window);
  }

  // Cleanup
  gui.shutdown();
  glDeleteProgram(shader_program);
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glfwDestroyWindow(window);
  glfwTerminate();
}

static void error_callback(int error, const char *description) {
  std::cerr << "GLFW error" << error << ": " << description << std::endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  // Pass to GUI first
  if (g_gui && g_gui->wantsKeyboardInput()) {
    return;
  }

  if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    move_right = true;
  if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
    move_right = false;

  if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    move_left = true;
  if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
    move_left = false;
  if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    move_up = true;
  if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
    move_up = false;
  if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    move_down = true;
  if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
    move_down = false;
  if (key == GLFW_KEY_O && action == GLFW_PRESS)
    zoom_in = true;
  if (key == GLFW_KEY_O && action == GLFW_RELEASE)
    zoom_in = false;
  if (key == GLFW_KEY_N && action == GLFW_PRESS)
    zoom_out = true;
  if (key == GLFW_KEY_N && action == GLFW_RELEASE)
    zoom_out = false;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      // Only start drag if not clicking on GUI
      if (g_gui && !g_gui->wantsMouseInput()) {
        is_dragging = true;
        glfwGetCursorPos(window, &last_mouse_x, &last_mouse_y);
      }
    } else if (action == GLFW_RELEASE) {
      is_dragging = false;
    }
  }
}

static void cursor_position_callback(GLFWwindow *window, double xpos,
                                     double ypos) {
  if (is_dragging && g_camera) {
    double dx = xpos - last_mouse_x;
    double dy = ypos - last_mouse_y;
    last_mouse_x = xpos;
    last_mouse_y = ypos;

    if (dx == 0.0 && dy == 0.0) {
      return;
    }

    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0 || h <= 0) {
      return;
    }

    const float ndc_dx = static_cast<float>(2.0 * dx / w);
    const float ndc_dy = static_cast<float>(-2.0 * dy / h);
    const float aspect = static_cast<float>(w) / static_cast<float>(h);
    const float scale = g_camera->get_scale();

    // Dragging moves the world with the cursor, so the camera moves opposite.
    g_camera->move(-ndc_dx * aspect * scale, -ndc_dy * scale);
  }
}

static void scroll_callback(GLFWwindow *window, double xoffset,
                            double yoffset) {
  if (!g_camera || !g_gui || g_gui->wantsMouseInput()) {
    return;
  }

  // Get mouse position in normalized device coordinates
  double mouse_x, mouse_y;
  glfwGetCursorPos(window, &mouse_x, &mouse_y);

  // Convert to world coordinates before zoom
  float ndc_x =
      (static_cast<float>(mouse_x) / static_cast<float>(g_window_width)) *
          2.0f -
      1.0f;
  float ndc_y = 1.0f - (static_cast<float>(mouse_y) /
                        static_cast<float>(g_window_height)) *
                           2.0f;

  // Convert NDC to world coordinates
  float aspect =
      static_cast<float>(g_window_width) / static_cast<float>(g_window_height);
  float world_x =
      ndc_x * g_camera->get_scale() * g_camera->get_zoom() * aspect +
      g_camera->get_x();
  float world_y =
      ndc_y * g_camera->get_scale() * g_camera->get_zoom() + g_camera->get_y();

  // Zoom amount
  float zoom_amount = static_cast<float>(yoffset) * 0.1f;
  g_camera->zoom_at(zoom_amount, world_x, world_y);
}
