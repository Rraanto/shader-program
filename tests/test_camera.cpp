/*
 * test_camera.cpp
 * Tests for the Camera class
 * Exit code: 0 = all tests passed, 1 = at least one test failed
 */

#include "camera/camera.h"
#include "test_utils.h"

#include <string>

// Test: Initial camera state
TestResult TEST(initial_state)() {
  const char *test_name = "initial_state";
  Camera cam(1.0f, 2.0f);

  ASSERT_FLOAT_EQ(cam.get_x(), 1.0f, 1e-6f, "Initial X should be 1.0");
  ASSERT_FLOAT_EQ(cam.get_y(), 2.0f, 1e-6f, "Initial Y should be 2.0");
  ASSERT_FLOAT_EQ(cam.get_zoom(), 1.0f, 1e-6f, "Initial zoom should be 1.0");

  return {test_name, true, "Initial state correct"};
}

// Test: Move camera
TestResult TEST(move)() {
  const char *test_name = "move";
  Camera cam(0.0f, 0.0f);

  cam.move(1.0f, 0.0f); // Move right
  ASSERT_FLOAT_EQ(cam.get_x(), 1.0f, 1e-6f, "Move right failed");

  cam.move(0.0f, 1.0f); // Move up
  ASSERT_FLOAT_EQ(cam.get_y(), 1.0f, 1e-6f, "Move up failed");

  cam.move(-0.5f, -0.5f); // Move left and down
  ASSERT_FLOAT_EQ(cam.get_x(), 0.5f, 1e-6f, "Move left failed");
  ASSERT_FLOAT_EQ(cam.get_y(), 0.5f, 1e-6f, "Move down failed");

  return {test_name, true, "Move operations correct"};
}

// Test: Zoom in/out
TestResult TEST(zoom)() {
  const char *test_name = "zoom";
  Camera cam(0.0f, 0.0f);

  float initial_zoom = cam.get_zoom();
  cam.zoom_in(0.1f);
  ASSERT_TRUE(cam.get_zoom() < initial_zoom, "Zoom in should decrease factor");

  float zoomed_in = cam.get_zoom();
  cam.zoom_out(0.1f);
  ASSERT_TRUE(cam.get_zoom() > zoomed_in, "Zoom out should increase factor");

  return {test_name, true, "Zoom operations correct"};
}

// Test: Setters
TestResult TEST(setters)() {
  const char *test_name = "setters";
  Camera cam(0.0f, 0.0f);

  cam.set_center(5.0f, 10.0f);
  ASSERT_FLOAT_EQ(cam.get_x(), 5.0f, 1e-6f, "set_center X failed");
  ASSERT_FLOAT_EQ(cam.get_y(), 10.0f, 1e-6f, "set_center Y failed");

  cam.set_zoom(0.5f);
  ASSERT_FLOAT_EQ(cam.get_zoom(), 0.5f, 1e-6f, "set_zoom failed");

  return {test_name, true, "Setters correct"};
}

// Test: Reset
TestResult TEST(reset)() {
  const char *test_name = "reset";
  Camera cam(0.0f, 0.0f);

  cam.move(10.0f, 10.0f);
  cam.zoom_in(0.5f);

  cam.reset(1.0f, 2.0f, 0.8f);
  ASSERT_FLOAT_EQ(cam.get_x(), 1.0f, 1e-6f, "reset X failed");
  ASSERT_FLOAT_EQ(cam.get_y(), 2.0f, 1e-6f, "reset Y failed");
  ASSERT_FLOAT_EQ(cam.get_zoom(), 0.8f, 1e-6f, "reset zoom failed");

  return {test_name, true, "Reset correct"};
}

// Test: Pan (mouse drag)
TestResult TEST(pan)() {
  const char *test_name = "pan";
  Camera cam(0.0f, 0.0f);

  cam.pan(1.0f, 0.0f); // Pan right (should move camera left)
  ASSERT_TRUE(cam.get_x() < 0.0f, "Pan right should decrease X");

  cam.set_center(0.0f, 0.0f);
  cam.pan(0.0f, 1.0f); // Pan up (should move camera down)
  ASSERT_TRUE(cam.get_y() < 0.0f, "Pan up should decrease Y");

  return {test_name, true, "Pan operations correct"};
}

// Test: Zoom at point
TestResult TEST(zoom_at)() {
  const char *test_name = "zoom_at";
  Camera cam(0.0f, 0.0f);

  float old_zoom = cam.get_zoom();
  float target_x = 1.0f;
  float target_y = 1.0f;

  cam.zoom_at(0.1f, target_x, target_y);

  ASSERT_TRUE(cam.get_zoom() < old_zoom, "zoom_at should zoom in");
  ASSERT_TRUE(cam.get_x() != 0.0f || cam.get_y() != 0.0f,
              "zoom_at should adjust center");

  return {test_name, true, "Zoom at point correct"};
}

// Test: Zoom bounds
TestResult TEST(zoom_bounds)() {
  const char *test_name = "zoom_bounds";
  Camera cam(0.0f, 0.0f);

  // Try to zoom in beyond minimum
  for (int i = 0; i < 1000; ++i) {
    cam.zoom_in(0.1f);
  }
  ASSERT_TRUE(cam.get_zoom() > 0.0f, "Zoom should not go below minimum");

  // Try to zoom out beyond maximum
  for (int i = 0; i < 1000; ++i) {
    cam.zoom_out(0.1f);
  }
  ASSERT_TRUE(cam.get_zoom() <= 1.0f, "Zoom should not exceed maximum");

  return {test_name, true, "Zoom bounds respected"};
}

int main() {
  TestFunc tests[] = {TEST(initial_state), TEST(move),         TEST(zoom),
                      TEST(setters),       TEST(reset),       TEST(pan),
                      TEST(zoom_at),       TEST(zoom_bounds)};
  return run_tests("camera", tests, sizeof(tests) / sizeof(tests[0]));
}
