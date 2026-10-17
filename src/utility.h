#pragma once

#include <cstdlib>
#include <limits>

constexpr double infinity = std::numeric_limits<double>::infinity();
constexpr double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
  return degrees * pi / 180.0;
}

inline double random_double() { return std::rand() / (RAND_MAX + 1.0); }

inline double random_double(double min, double max) {
  return min + (max - min) * random_double();
}
