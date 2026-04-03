/*
 * test_gui_file_io.cpp
 * Headless tests for GUI file-loading behavior
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "gui/gui.h"
#include "test_utils.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

TestResult TEST(load_shader_file_success)() {
  const char *test_name = "load_shader_file_success";

  const fs::path temp_dir = fs::temp_directory_path() / "shader_test_gui_file_io";
  fs::create_directories(temp_dir);
  const fs::path shader_path = temp_dir / "valid.glsl";

  std::ofstream shader_file(shader_path);
  shader_file << "#version 330 core\n";
  shader_file << "void main() {}\n";
  shader_file.close();

  Gui gui;
  const bool loaded = gui.loadShaderFromFile(shader_path.string());

  fs::remove_all(temp_dir);

  ASSERT_TRUE(loaded, "Expected valid shader file to load");
  ASSERT_TRUE(gui.getShaderSource().find("#version 330 core") != std::string::npos,
              "Loaded shader source should be available");

  return {test_name, true, "GUI can load shader source without GL setup"};
}

TestResult TEST(load_shader_file_missing)() {
  const char *test_name = "load_shader_file_missing";

  Gui gui;
  const bool loaded =
      gui.loadShaderFromFile("/tmp/this_shader_file_should_not_exist_123456.glsl");

  ASSERT_TRUE(!loaded, "Missing shader file should fail to load");
  ASSERT_TRUE(gui.getShaderSource().empty(),
              "Missing file should not populate editor source");

  return {test_name, true, "GUI rejects missing shader files"};
}

TestResult TEST(load_shader_file_too_large)() {
  const char *test_name = "load_shader_file_too_large";

  const fs::path temp_dir = fs::temp_directory_path() / "shader_test_gui_large";
  fs::create_directories(temp_dir);
  const fs::path shader_path = temp_dir / "large.glsl";

  std::ofstream shader_file(shader_path);
  shader_file << std::string(70000, 'a');
  shader_file.close();

  Gui gui;
  const bool loaded = gui.loadShaderFromFile(shader_path.string());

  fs::remove_all(temp_dir);

  ASSERT_TRUE(!loaded, "Oversized shader file should be rejected");
  ASSERT_TRUE(gui.getShaderSource().empty(),
              "Oversized file should not populate editor source");

  return {test_name, true, "GUI enforces shader source size limit"};
}

int main() {
  const TestFunc tests[] = {TEST(load_shader_file_success),
                            TEST(load_shader_file_missing),
                            TEST(load_shader_file_too_large)};
  return run_tests("GUI file I/O", tests, sizeof(tests) / sizeof(tests[0]));
}
