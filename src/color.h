#pragma once

#include "interval.h"
#include "vec3.h"

#include <iostream>

using color = vec3;

inline double linear_to_gamma(double linear_component) {
  if (linear_component > 0) {
    return std::sqrt(linear_component);
  }
  return 0;
}

// gamma-correct a linear color and convert it to 0-255 bytes
inline void color_to_bytes(const color &pixel_color, uint8_t &rbyte,
                           uint8_t &gbyte, uint8_t &bbyte) {
  auto r = linear_to_gamma(pixel_color.x());
  auto g = linear_to_gamma(pixel_color.y());
  auto b = linear_to_gamma(pixel_color.z());

  static const interval intensity(0.000, 0.999);
  rbyte = uint8_t(256 * intensity.clamp(r));
  gbyte = uint8_t(256 * intensity.clamp(g));
  bbyte = uint8_t(256 * intensity.clamp(b));
}

void write_color(std::ostream &out, const color &pixel_color) {
  auto r = pixel_color.x();
  auto g = pixel_color.y();
  auto b = pixel_color.z();

  r = linear_to_gamma(r);
  g = linear_to_gamma(g);
  b = linear_to_gamma(b);

  static const interval intensity(0.000, 0.999);
  int rbyte = int(256 * intensity.clamp(r));
  int gbyte = int(256 * intensity.clamp(g));
  int bbyte = int(256 * intensity.clamp(b));

  out << rbyte << ' ' << gbyte << ' ' << bbyte << '\n';
}