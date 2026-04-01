/*
 * This implements a basic 2D camera
 *
 * Coordinates logic assume unnormalised coordinates
 * (1, 3), (3.14, 83), ... etc
 *
 * HEADLESS
 */
#ifndef CAMERA_H
#define CAMERA_H

struct Camera {
  /*
   * Basic camera center and scale coordinates
   */
private:
  // center coordinates

  // minimum zoom factor allowed
  float _min_zoom_factor = 1e-20;
  float _max_zoom_factor = 1.0;

  float _center_x, _center_y;
  float _zoom_factor; // the smaller the zoom factor the closer things appear
  float _scale;       // abstract-to-apparent ratio

public:
  Camera(const float &x, const float &y)
      : _center_x(x), _center_y(y), _zoom_factor(1.0), _scale(10.0) {}

  float get_x();
  float get_y();
  float get_zoom();
  float get_scale();

  // Setters for GUI control
  void set_center(float x, float y);
  void set_zoom(float zoom);
  void reset(float x, float y, float zoom = 1.0f);

  // Mouse control
  void pan(float dx, float dy);                    // Drag to move
  void zoom_at(float amount, float x, float y);   // Zoom towards point

  /*
   * moves with the specified strides
   *
   * horizontal_stride: amount in float to move right (left if negative)
   * vertical_stride: amount in float to move up (down if negative)
   */
  void move(const float &horizontal_stride, const float &vertical_stride);

  /*
   * Zooms in
   */
  void zoom_in(const float &stride);

  /*
   * Zooms out
   */
  void zoom_out(const float &stride);
};

#endif
