/*
 * test_compiler.cpp
 * Tests for the Compiler class (requires OpenGL context)
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "compiler/compiler/compiler.h"
#include "test_utils.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

// Test: Compile valid vertex shader
TestResult TEST(compile_valid_vertex)() {
  const char *test_name = "compile_valid_vertex";

  // Create temp file
  fs::path temp_dir = fs::temp_directory_path() / "shader_test_compiler";
  fs::create_directories(temp_dir);
  fs::path shader_path = temp_dir / "vertex.glsl";

  std::ofstream shader(shader_path);
  shader << "#version 330 core\n";
  shader << "layout(location = 0) in vec2 aPos;\n";
  shader << "void main() {\n";
  shader << "  gl_Position = vec4(aPos, 0.0, 1.0);\n";
  shader << "}\n";
  shader.close();

  Compiler compiler;
  Compiler::CompileOutput output =
      compiler.compile(shader_path, GL_VERTEX_SHADER);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Valid vertex shader should compile");
  ASSERT_TRUE(output.shader != 0, "Should return valid shader ID");

  return {test_name, true, "Vertex shader compilation works"};
}

// Test: Compile valid fragment shader
TestResult TEST(compile_valid_fragment)() {
  const char *test_name = "compile_valid_fragment";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_compiler";
  fs::create_directories(temp_dir);
  fs::path shader_path = temp_dir / "fragment.glsl";

  std::ofstream shader(shader_path);
  shader << "#version 330 core\n";
  shader << "out vec4 FragColor;\n";
  shader << "void main() {\n";
  shader << "  FragColor = vec4(1.0);\n";
  shader << "}\n";
  shader.close();

  Compiler compiler;
  Compiler::CompileOutput output =
      compiler.compile(shader_path, GL_FRAGMENT_SHADER);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Valid fragment shader should compile");
  ASSERT_TRUE(output.shader != 0, "Should return valid shader ID");

  return {test_name, true, "Fragment shader compilation works"};
}

// Test: Compile shader with syntax error
TestResult TEST(compile_invalid)() {
  const char *test_name = "compile_invalid";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_compiler";
  fs::create_directories(temp_dir);
  fs::path shader_path = temp_dir / "invalid.glsl";

  std::ofstream shader(shader_path);
  shader << "#version 330 core\n";
  shader << "void main() {\n";
  shader << "  invalid syntax here!!!\n";
  shader << "}\n";
  shader.close();

  Compiler compiler;
  Compiler::CompileOutput output =
      compiler.compile(shader_path, GL_FRAGMENT_SHADER);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(!output.success, "Invalid shader should fail to compile");
  ASSERT_TRUE(!output.error.empty(), "Should provide error message");

  return {test_name, true, "Invalid shader detection works"};
}

// Test: Compile shader with includes
TestResult TEST(compile_with_includes)() {
  const char *test_name = "compile_with_includes";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_compiler";
  fs::create_directories(temp_dir);

  // Create library
  fs::path lib_path = temp_dir / "lib.glsl";
  std::ofstream lib(lib_path);
  lib << "vec4 compute_color() { return vec4(1.0); }\n";
  lib.close();

  // Create main shader
  fs::path shader_path = temp_dir / "main.glsl";
  std::ofstream shader(shader_path);
  shader << "#version 330 core\n";
  shader << "#include \"lib.glsl\"\n";
  shader << "out vec4 FragColor;\n";
  shader << "void main() {\n";
  shader << "  FragColor = compute_color();\n";
  shader << "}\n";
  shader.close();

  Compiler compiler({temp_dir});
  Compiler::CompileOutput output =
      compiler.compile(shader_path, GL_FRAGMENT_SHADER);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Shader with includes should compile");

  return {test_name, true, "Shader with includes compilation works"};
}

int main() {
  if (!glfwInit()) {
    std::cout << "Skipping compiler tests: GLFW initialization failed.\n";
    return 77;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(100, 100, "compiler_test", nullptr, nullptr);
  if (window == nullptr) {
    std::cout << "Skipping compiler tests: failed to create OpenGL window.\n";
    glfwTerminate();
    return 77;
  }

  glfwMakeContextCurrent(window);
  if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ==
      0) {
    std::cout << "Skipping compiler tests: GLAD initialization failed.\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return 77;
  }

  TestFunc tests[] = {TEST(compile_valid_vertex), TEST(compile_valid_fragment),
                      TEST(compile_invalid), TEST(compile_with_includes)};
  const int status =
      run_tests("compiler", tests, sizeof(tests) / sizeof(tests[0]));
  glfwDestroyWindow(window);
  glfwTerminate();
  return status;
}
