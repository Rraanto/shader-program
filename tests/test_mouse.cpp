/*
 * test_mouse.cpp
 * Tests for mouse interaction with camera (requires OpenGL context)
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "camera/camera.h"
#include "test_utils.h"

#include <cmath>

// Test: Pan moves camera opposite to drag
TestResult TEST(pan_direction)() {
  const char *test_name = "pan_direction";

  Camera cam(0.0f, 0.0f);

  // Pan right (positive dx) should move camera left (decrease x)
  float x_before = cam.get_x();
  cam.pan(1.0f, 0.0f);
  ASSERT_TRUE(cam.get_x() < x_before, "Pan right should move camera left");

  // Reset
  cam.set_center(0.0f, 0.0f);

  // Pan up (positive dy) should move camera down (decrease y)
  float y_before = cam.get_y();
  cam.pan(0.0f, 1.0f);
  ASSERT_TRUE(cam.get_y() < y_before, "Pan up should move camera down");

  return {test_name, true, "Pan direction is correct"};
}

// Test: Pan with zoom factor
TestResult TEST(pan_with_zoom)() {
  const char *test_name = "pan_with_zoom";

  Camera cam(0.0f, 0.0f);
  cam.set_zoom(0.5f); // Zoomed in

  float x_before = cam.get_x();
  cam.pan(1.0f, 0.0f);

  // Pan amount should be scaled by zoom
  ASSERT_TRUE(cam.get_x() != x_before, "Pan should work with zoom");

  return {test_name, true, "Pan with zoom works"};
}

// Test: Zoom at point changes center
TestResult TEST(zoom_at_changes_center)() {
  const char *test_name = "zoom_at_changes_center";

  Camera cam(0.0f, 0.0f);
  float zoom_target_x = 5.0f;
  float zoom_target_y = 5.0f;

  cam.zoom_at(0.1f, zoom_target_x, zoom_target_y);

  // Center should move towards zoom target
  ASSERT_TRUE(std::abs(cam.get_x() - zoom_target_x) < std::abs(0.0f - zoom_target_x),
              "X should move toward zoom target");
  ASSERT_TRUE(std::abs(cam.get_y() - zoom_target_y) < std::abs(0.0f - zoom_target_y),
              "Y should move toward zoom target");

  return {test_name, true, "Zoom at point changes center correctly"};
}

// Test: Screen to world coordinate conversion (simulated)
TestResult TEST(screen_to_world_conversion)() {
  const char *test_name = "screen_to_world_conversion";

  // This tests the math used in the scroll callback
  int window_width = 1000;
  int window_height = 800;
  float aspect = static_cast<float>(window_width) / window_height;

  Camera cam(0.0f, 0.0f);

  // Simulate mouse at center of screen
  int mouse_x = window_width / 2;
  int mouse_y = window_height / 2;

  // Convert to NDC (same as in main.cpp)
  float ndc_x = (static_cast<float>(mouse_x) / window_width) * 2.0f - 1.0f;
  float ndc_y = 1.0f - (static_cast<float>(mouse_y) / window_height) * 2.0f;

  // Convert to world coordinates
  float world_x = ndc_x * cam.get_scale() * cam.get_zoom() * aspect + cam.get_x();
  float world_y = ndc_y * cam.get_scale() * cam.get_zoom() + cam.get_y();

  // At center of screen, world should be close to camera center
  ASSERT_FLOAT_EQ(world_x, cam.get_x(), 0.01f,
                  "Center screen X should map to camera X");
  ASSERT_FLOAT_EQ(world_y, cam.get_y(), 0.01f,
                  "Center screen Y should map to camera Y");

  return {test_name, true, "Screen to world conversion is correct"};
}

// Test: Drag distance calculation
TestResult TEST(drag_distance)() {
  const char *test_name = "drag_distance";

  // Simulate drag from (100, 100) to (150, 120)
  double last_x = 100.0, last_y = 100.0;
  double current_x = 150.0, current_y = 120.0;

  double dx = current_x - last_x; // 50
  double dy = current_y - last_y; // 20

  ASSERT_TRUE(dx > 0, "Drag right should have positive dx");
  ASSERT_TRUE(dy > 0, "Drag down should have positive dy");

  // Distance should be sqrt(50^2 + 20^2) = sqrt(2500 + 400) = sqrt(2900) ≈ 53.85
  double distance = std::sqrt(dx * dx + dy * dy);
  ASSERT_TRUE(distance > 50.0 && distance < 55.0,
              "Drag distance should be calculated correctly");

  return {test_name, true, "Drag distance calculation is correct"};
}

// Test: Zoom limits
TestResult TEST(zoom_limits)() {
  const char *test_name = "zoom_limits";

  Camera cam(0.0f, 0.0f);

  // Zoom in many times
  for (int i = 0; i < 100; ++i) {
    cam.zoom_at(0.5f, 0.0f, 0.0f);
  }

  float min_zoom = cam.get_zoom();

  // Reset and zoom out
  cam.reset(0.0f, 0.0f, 1.0f);
  for (int i = 0; i < 100; ++i) {
    cam.zoom_at(-0.5f, 0.0f, 0.0f);
  }

  float max_zoom = cam.get_zoom();

  ASSERT_TRUE(min_zoom > 0.0f, "Zoom should have a minimum above 0");
  ASSERT_TRUE(max_zoom <= 1.0f, "Zoom should have a maximum at 1.0");
  ASSERT_TRUE(min_zoom < max_zoom, "Min zoom should be less than max zoom");

  return {test_name, true, "Zoom limits are respected"};
}

int main() {
  TestFunc tests[] = {
    TEST(pan_direction),
    TEST(pan_with_zoom),
    TEST(zoom_at_changes_center),
    TEST(screen_to_world_conversion),
    TEST(drag_distance),
    TEST(zoom_limits)
  };
  return run_tests("mouse", tests, sizeof(tests) / sizeof(tests[0]));
}
