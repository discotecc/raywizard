#pragma once

#include "intersectable.h"
#include "interval.h"
#include "ray.h"

#include <memory>
#include <vector>

class intersectable_list : public intersectable {

public:
  std::vector<std::shared_ptr<intersectable>> objects;
  intersectable_list() {};
  intersectable_list(std::shared_ptr<intersectable> object) { add(object); };

  void clear() { objects.clear(); };

  void add(std::shared_ptr<intersectable> object) {
    objects.push_back(object);
  };

  bool hit(const ray &r, interval ray_t, hit_record &record) const override {
    hit_record temp_record;
    bool hit_anything = false;
    auto closest_so_far = ray_t.max;

    for (const auto &object : objects) {
      if (object->hit(r, interval(ray_t.min, closest_so_far), temp_record)) {
        hit_anything = true;
        closest_so_far = temp_record.t;
        record = temp_record;
      }
    }
    return hit_anything;
  }
};