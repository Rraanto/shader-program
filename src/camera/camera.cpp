#include "camera.h"
#include <algorithm>

void Camera::move(const float &horizontal_stride,
                  const float &vertical_stride) {
  this->_center_x += horizontal_stride * this->_zoom_factor;
  this->_center_y += vertical_stride * this->_zoom_factor;
}

float Camera::get_x() { return this->_center_x; }

float Camera::get_y() { return this->_center_y; }

float Camera::get_zoom() { return this->_zoom_factor; }

float Camera::get_scale() { return this->_scale; }

void Camera::zoom_in(const float &stride) {
  this->_zoom_factor = std::max(
      this->_min_zoom_factor, this->_zoom_factor - stride * this->_zoom_factor);
}

void Camera::zoom_out(const float &stride) {
  this->_zoom_factor = std::min(
      this->_max_zoom_factor, this->_zoom_factor + stride * this->_zoom_factor);
}

void Camera::set_center(float x, float y) {
  this->_center_x = x;
  this->_center_y = y;
}

void Camera::set_zoom(float zoom) {
  this->_zoom_factor = std::clamp(zoom, this->_min_zoom_factor, this->_max_zoom_factor);
}

void Camera::reset(float x, float y, float zoom) {
  this->_center_x = x;
  this->_center_y = y;
  this->_zoom_factor = std::clamp(zoom, this->_min_zoom_factor, this->_max_zoom_factor);
}

void Camera::pan(float dx, float dy) {
  // Convert world delta to camera movement
  // Negative because moving camera opposite to drag direction
  this->_center_x -= dx * this->_zoom_factor;
  this->_center_y -= dy * this->_zoom_factor;
}

void Camera::zoom_at(float amount, float x, float y) {
  // Zoom towards a specific point
  // amount > 0 zooms in, amount < 0 zooms out
  float old_zoom = this->_zoom_factor;
  float new_zoom = std::clamp(
      this->_zoom_factor - amount * this->_zoom_factor,
      this->_min_zoom_factor,
      this->_max_zoom_factor);

  // Adjust center to zoom towards mouse position
  float zoom_ratio = new_zoom / old_zoom;
  this->_center_x = x - (x - this->_center_x) * zoom_ratio;
  this->_center_y = y - (y - this->_center_y) * zoom_ratio;

  this->_zoom_factor = new_zoom;
}
