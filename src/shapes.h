#pragma once

// build solid objects out of triangles

#include "intersectable_list.h"
#include "triangle.h"

#include <memory>
#include <utility>

namespace shape_detail {

inline void add_tri(intersectable_list &list, const object_frame &frame,
                    const point3 &inside, point3 a, point3 b, point3 c,
                    std::shared_ptr<material> mat) {
  vec3 n = cross(b - a, c - a);
  if (dot(n, a - inside) < 0)
    std::swap(b, c);
  list.add(std::make_shared<triangle>(frame.to_world(a), frame.to_world(b),
                                      frame.to_world(c), mat, frame));
}

inline void add_quad(intersectable_list &list, const object_frame &frame,
                     const point3 &inside, const point3 &a, const point3 &b,
                     const point3 &c, const point3 &d,
                     std::shared_ptr<material> mat) {
  add_tri(list, frame, inside, a, b, c, mat);
  add_tri(list, frame, inside, a, c, d, mat);
}

} // namespace shape_detail

inline void add_box(intersectable_list &list, const point3 &center,
                    const vec3 &size, double angle_y,
                    std::shared_ptr<material> mat) {
  using namespace shape_detail;
  object_frame frame{center, angle_y};
  double x = size.x() / 2, y = size.y() / 2, z = size.z() / 2;

  // 8 corners in local space
  point3 p000(-x, -y, -z), p100(x, -y, -z), p010(-x, y, -z), p110(x, y, -z);
  point3 p001(-x, -y, z), p101(x, -y, z), p011(-x, y, z), p111(x, y, z);
  point3 inside(0, 0, 0);

  add_quad(list, frame, inside, p000, p100, p110, p010, mat); // back   -z
  add_quad(list, frame, inside, p001, p101, p111, p011, mat); // front  +z
  add_quad(list, frame, inside, p000, p001, p011, p010, mat); // left   -x
  add_quad(list, frame, inside, p100, p101, p111, p110, mat); // right  +x
  add_quad(list, frame, inside, p000, p100, p101, p001, mat); // bottom -y
  add_quad(list, frame, inside, p010, p110, p111, p011, mat); // top    +y
}

inline void add_cube(intersectable_list &list, const point3 &center,
                     double side, double angle_y,
                     std::shared_ptr<material> mat) {
  add_box(list, center, vec3(side, side, side), angle_y, mat);
}

inline void add_pyramid(intersectable_list &list, const point3 &base_center,
                        double base_width, double height, double angle_y,
                        std::shared_ptr<material> mat) {
  using namespace shape_detail;
  object_frame frame{base_center, angle_y};
  double w = base_width / 2;

  point3 b0(-w, 0, -w), b1(w, 0, -w), b2(w, 0, w), b3(-w, 0, w);
  point3 apex(0, height, 0);
  point3 inside(0, height / 4, 0);

  add_quad(list, frame, inside, b0, b1, b2, b3, mat); // base
  add_tri(list, frame, inside, b0, b1, apex, mat);
  add_tri(list, frame, inside, b1, b2, apex, mat);
  add_tri(list, frame, inside, b2, b3, apex, mat);
  add_tri(list, frame, inside, b3, b0, apex, mat);
}
