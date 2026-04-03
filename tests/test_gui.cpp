/*
 * test_gui.cpp
 * Tests for the GUI class (requires OpenGL context)
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "gui/gui.h"
#include "test_utils.h"
#include "camera/camera.h"
#include "compiler/compiler/compiler.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstring>

// Test: GUI initialization
TestResult TEST(gui_initialization)() {
  const char *test_name = "gui_initialization";

  GLFWwindow* window = glfwCreateWindow(100, 100, "Test", nullptr, nullptr);
  ASSERT_TRUE(window != nullptr, "Window should be created");

  glfwMakeContextCurrent(window);

  Gui gui;
  bool success = gui.initialize(window);
  ASSERT_TRUE(success, "GUI should initialize successfully");

  gui.shutdown();
  glfwDestroyWindow(window);

  return {test_name, true, "GUI initialization works"};
}

// Test: Load shader file
TestResult TEST(gui_load_shader)() {
  const char *test_name = "gui_load_shader";

  GLFWwindow* window = glfwCreateWindow(100, 100, "Test", nullptr, nullptr);
  ASSERT_TRUE(window != nullptr, "Window should be created");

  glfwMakeContextCurrent(window);

  Gui gui;
  gui.initialize(window);

  // Create a temp shader file
  std::string temp_path = "/tmp/test_shader.glsl";
  FILE* f = fopen(temp_path.c_str(), "w");
  fprintf(f, "#version 330 core\nvoid main() {}\n");
  fclose(f);

  bool loaded = gui.loadShaderFromFile(temp_path);
  ASSERT_TRUE(loaded, "Should load shader file");

  std::string source = gui.getShaderSource();
  ASSERT_TRUE(source.find("#version") != std::string::npos,
              "Should contain shader content");

  gui.shutdown();
  glfwDestroyWindow(window);
  remove(temp_path.c_str());

  return {test_name, true, "GUI shader loading works"};
}

// Test: GUI wants input
TestResult TEST(gui_input_handling)() {
  const char *test_name = "gui_input_handling";

  GLFWwindow* window = glfwCreateWindow(100, 100, "Test", nullptr, nullptr);
  ASSERT_TRUE(window != nullptr, "Window should be created");

  glfwMakeContextCurrent(window);

  Gui gui;
  gui.initialize(window);

  // After initialization, GUI might want input
  // This is a basic smoke test
  (void)gui.wantsKeyboardInput();
  (void)gui.wantsMouseInput();

  gui.shutdown();
  glfwDestroyWindow(window);

  return {test_name, true, "GUI input handling works"};
}

int main() {
  if (!glfwInit()) {
    std::cout << "Skipping GUI tests: GLFW initialization failed.\n";
    return 77;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window

  GLFWwindow *loader_window =
      glfwCreateWindow(100, 100, "gui_loader", nullptr, nullptr);
  if (loader_window == nullptr) {
    std::cout << "Skipping GUI tests: failed to create OpenGL window.\n";
    glfwTerminate();
    return 77;
  }

  glfwMakeContextCurrent(loader_window);
  if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ==
      0) {
    std::cout << "Skipping GUI tests: GLAD initialization failed.\n";
    glfwDestroyWindow(loader_window);
    glfwTerminate();
    return 77;
  }

  glfwDestroyWindow(loader_window);
  glfwMakeContextCurrent(nullptr);

  TestFunc tests[] = {
    TEST(gui_initialization),
    TEST(gui_load_shader),
    TEST(gui_input_handling)
  };
  const int status = run_tests("GUI", tests, sizeof(tests) / sizeof(tests[0]));
  glfwTerminate();
  return status;
}
