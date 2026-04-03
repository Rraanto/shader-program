#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <cmath>
#include <cstddef>
#include <iostream>

struct TestResult {
  const char *name;
  bool passed;
  const char *message;
};

using TestFunc = TestResult (*)();

#define TEST(name) test_##name

#define ASSERT_TRUE(expr, msg)                                               \
  do {                                                                       \
    if (!(expr)) {                                                           \
      return {test_name, false, msg};                                        \
    }                                                                        \
  } while (0)

#define ASSERT_FLOAT_EQ(a, b, tolerance, msg)                                \
  do {                                                                       \
    if (std::abs((a) - (b)) > (tolerance)) {                                 \
      return {test_name, false, msg};                                        \
    }                                                                        \
  } while (0)

inline int run_tests(const char *suite_name, const TestFunc *tests,
                     std::size_t num_tests) {
  int passed = 0;
  int failed = 0;

  std::cout << "Running " << num_tests << ' ' << suite_name << " tests...\n\n";

  for (std::size_t i = 0; i < num_tests; ++i) {
    const TestResult result = tests[i]();
    if (result.passed) {
      std::cout << "[PASS] " << result.name << ": " << result.message << "\n";
      ++passed;
    } else {
      std::cout << "[FAIL] " << result.name << ": " << result.message << "\n";
      ++failed;
    }
  }

  std::cout << "\nResults: " << passed << "/" << num_tests << " passed\n";
  return failed > 0 ? 1 : 0;
}

#endif
