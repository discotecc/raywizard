#pragma once

#include "intersectable.h"
#include "interval.h"
#include "ray.h"
#include "vec3.h"

#include <cmath>
#include <memory>

struct object_frame {
  point3 origin = point3(0, 0, 0);
  double angle_y = 0; // radians

  point3 to_world(const point3 &local) const {
    double c = std::cos(angle_y), s = std::sin(angle_y);
    return origin + vec3(c * local.x() + s * local.z(), local.y(),
                         -s * local.x() + c * local.z());
  }

  point3 to_local(const point3 &world) const {
    vec3 d = world - origin;
    double c = std::cos(angle_y), s = std::sin(angle_y);
    return vec3(c * d.x() - s * d.z(), d.y(), s * d.x() + c * d.z());
  }
};

class triangle : public intersectable {
public:
  triangle(const point3 &a, const point3 &b, const point3 &c,
           std::shared_ptr<material> mat, const object_frame &frame = {})
      : a(a), e1(b - a), e2(c - a), mat(mat), frame(frame) {
    normal = unit_vector(cross(e1, e2));
  }

  // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
  bool hit(const ray &r, interval ray_t, hit_record &rec) const override {
    vec3 pvec = cross(r.direction(), e2);
    double det = dot(e1, pvec);
    if (std::fabs(det) < 1e-12)
      return false; // ray is parallel to the triangle's plane

    double inv_det = 1.0 / det;
    vec3 tvec = r.origin() - a;
    double u = dot(tvec, pvec) * inv_det;
    if (u < 0 || u > 1)
      return false;

    vec3 qvec = cross(tvec, e1);
    double v = dot(r.direction(), qvec) * inv_det;
    if (v < 0 || u + v > 1)
      return false;

    double t = dot(e2, qvec) * inv_det;
    if (!ray_t.surrounds(t))
      return false;

    rec.t = t;
    rec.p = r.at(t);
    rec.local_p = frame.to_local(rec.p);
    rec.set_face_normal(r, normal);
    rec.mat = mat;
    return true;
  }

private:
  point3 a;
  vec3 e1, e2, normal;
  std::shared_ptr<material> mat;
  object_frame frame;
};
