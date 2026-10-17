#pragma once

#include "color.h"
#include "intersectable.h"
#include "ray.h"

class material {
public:
  virtual ~material() = default;

  virtual bool scatter(const ray &r_in, const hit_record &rec,
                       color &attenuation, ray &scattered) const {
    return false;
  }
};

// matte surfaces
class lambertian : public material {
public:
  lambertian(const color &albedo) : albedo(albedo) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    auto scatter_direction = rec.normal + random_unit_vector();
    if (scatter_direction.near_zero()) {
      scatter_direction = rec.normal;
    }
    scattered = ray(rec.p, scatter_direction);
    attenuation = albedo;
    return true;
  }

private:
  color albedo;
};

class metal : public material {
public:
  metal(const color &albedo, double fuzz)
      : albedo(albedo), fuzz(fuzz < 1 ? fuzz : 1) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    vec3 reflected = reflect(r_in.direction(), rec.normal);
    reflected = unit_vector(reflected) + (fuzz * random_unit_vector());
    scattered = ray(rec.p, reflected);
    attenuation = albedo;
    return (dot(scattered.direction(), rec.normal) > 0);
  }

private:
  color albedo;
  double fuzz;
};

class dielectric : public material {
public:
  dielectric(double refraction_index) : refraction_index(refraction_index) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    attenuation = color(1.0, 1.0, 1.0);
    double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;
    vec3 unit_direction = unit_vector(r_in.direction());
    vec3 refracted = refract(unit_direction, rec.normal, ri);

    scattered = ray(rec.p, refracted);
    return true;
  }

private:
  double refraction_index;
};

class checker : public material {
public:
  checker(const color &even, const color &odd, double scale, double gloss = 0.0)
      : even(even), odd(odd), inv_scale(1.0 / scale), gloss(gloss) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    int xi = int(std::floor(inv_scale * rec.p.x()));
    int zi = int(std::floor(inv_scale * rec.p.z()));
    bool is_even = (xi + zi) % 2 == 0;
    attenuation = is_even ? even : odd;

    if (random_double() < gloss) {
      // specular bounce
      vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
      scattered = ray(rec.p, reflected);
    } else {
      // matte bounce
      auto dir = rec.normal + random_unit_vector();
      if (dir.near_zero())
        dir = rec.normal;
      scattered = ray(rec.p, dir);
    }
    return true;
  }

private:
  color even, odd;
  double inv_scale;
  double gloss;
};