/*
 * test_preprocessor.cpp
 * Tests for the Preprocessor class
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "preprocessor/preprocessor.h"
#include "test_utils.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// Test: Simple include expansion
TestResult TEST(single_include)() {
  const char *test_name = "single_include";

  // Create temp directory
  fs::path temp_dir = fs::temp_directory_path() / "shader_test_single";
  fs::create_directories(temp_dir);

  // Create a library file
  fs::path lib_path = temp_dir / "library.glsl";
  std::ofstream lib(lib_path);
  lib << "// Library content\n";
  lib << "float test_func() { return 1.0; }\n";
  lib.close();

  // Create a main file that includes the library
  fs::path main_path = temp_dir / "main.glsl";
  std::ofstream main_file(main_path);
  main_file << "#include \"library.glsl\"\n";
  main_file << "void main() {}\n";
  main_file.close();

  // Preprocess
  Preprocessor pp({temp_dir});
  Preprocessor::Output output = pp.process_source(main_path);

  // Cleanup
  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Preprocessing should succeed");
  ASSERT_TRUE(output.processed_source.find("test_func") != std::string::npos,
              "Include should be expanded");

  return {test_name, true, "Single include works"};
}

// Test: Multiple includes
TestResult TEST(multiple_includes)() {
  const char *test_name = "multiple_includes";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_multi";
  fs::create_directories(temp_dir);

  // Create multiple library files
  fs::path lib1_path = temp_dir / "lib1.glsl";
  std::ofstream lib1(lib1_path);
  lib1 << "// Lib1\n";
  lib1.close();

  fs::path lib2_path = temp_dir / "lib2.glsl";
  std::ofstream lib2(lib2_path);
  lib2 << "// Lib2\n";
  lib2.close();

  fs::path main_path = temp_dir / "main.glsl";
  std::ofstream main_file(main_path);
  main_file << "#include \"lib1.glsl\"\n";
  main_file << "#include \"lib2.glsl\"\n";
  main_file << "void main() {}\n";
  main_file.close();

  Preprocessor pp({temp_dir});
  Preprocessor::Output output = pp.process_source(main_path);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Preprocessing should succeed");
  ASSERT_TRUE(output.processed_source.find("// Lib1") != std::string::npos,
              "First include should be expanded");
  ASSERT_TRUE(output.processed_source.find("// Lib2") != std::string::npos,
              "Second include should be expanded");

  return {test_name, true, "Multiple includes work"};
}

// Test: Circular include detection
TestResult TEST(circular_include)() {
  const char *test_name = "circular_include";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_circular";
  fs::create_directories(temp_dir);

  // Create files that include each other
  fs::path a_path = temp_dir / "a.glsl";
  fs::path b_path = temp_dir / "b.glsl";

  std::ofstream a_file(a_path);
  a_file << "#include \"b.glsl\"\n";
  a_file << "// A\n";
  a_file.close();

  std::ofstream b_file(b_path);
  b_file << "#include \"a.glsl\"\n";
  b_file << "// B\n";
  b_file.close();

  Preprocessor pp({temp_dir});
  Preprocessor::Output output = pp.process_source(a_path);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(!output.success, "Circular include should fail");
  ASSERT_TRUE(output.error.find("circular") != std::string::npos ||
                  output.error.find("Circular") != std::string::npos,
              "Error should mention circular dependency");

  return {test_name, true, "Circular include detection works"};
}

// Test: Missing include file
TestResult TEST(missing_include)() {
  const char *test_name = "missing_include";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_missing";
  fs::create_directories(temp_dir);

  fs::path main_path = temp_dir / "main.glsl";
  std::ofstream main_file(main_path);
  main_file << "#include \"nonexistent.glsl\"\n";
  main_file.close();

  Preprocessor pp({temp_dir});
  Preprocessor::Output output = pp.process_source(main_path);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(!output.success, "Missing include should fail");

  return {test_name, true, "Missing include detection works"};
}

// Test: No includes (pass-through)
TestResult TEST(no_includes)() {
  const char *test_name = "no_includes";

  fs::path temp_dir = fs::temp_directory_path() / "shader_test_none";
  fs::create_directories(temp_dir);

  fs::path main_path = temp_dir / "main.glsl";
  std::ofstream main_file(main_path);
  main_file << "void main() {\n";
  main_file << "  gl_FragColor = vec4(1.0);\n";
  main_file << "}\n";
  main_file.close();

  Preprocessor pp;
  Preprocessor::Output output = pp.process_source(main_path);

  fs::remove_all(temp_dir);

  ASSERT_TRUE(output.success, "Preprocessing should succeed");
  ASSERT_TRUE(output.processed_source.find("void main()") != std::string::npos,
              "Content should be preserved");

  return {test_name, true, "No includes passthrough works"};
}

int main() {
  TestFunc tests[] = {TEST(single_include), TEST(multiple_includes),
                      TEST(circular_include), TEST(missing_include),
                      TEST(no_includes)};
  return run_tests("preprocessor", tests, sizeof(tests) / sizeof(tests[0]));
}
