#pragma once

#include "interval.h"
#include "ray.h"
#include "vec3.h"

#include <memory>

class material;

class hit_record {
public:
  point3 p;
  point3 local_p; // for texturing...
  vec3 normal;
  const material *mat = nullptr;
  double t;
  bool front_face;

  void set_face_normal(const ray &r, const vec3 &outward_normal) {
    front_face = dot(r.direction(), outward_normal) < 0;
    normal = front_face ? outward_normal : -outward_normal;
  }
};

class intersectable {
public:
  virtual ~intersectable() = default;

  virtual bool hit(const ray &r, interval ray_t, hit_record &record) const = 0;
};