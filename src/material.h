#pragma once

#include "color.h"
#include "intersectable.h"
#include "perlin.h"
#include "ray.h"

#include <cmath>

// fresnel reflectance approximation... Schlicky
inline double reflectance(double cosine, double refraction_ratio) {
  double r0 = (1 - refraction_ratio) / (1 + refraction_ratio);
  r0 = r0 * r0;
  return r0 + (1 - r0) * std::pow(1 - cosine, 5);
}

inline color exp_color(const color &c) {
  return color(std::exp(c.x()), std::exp(c.y()), std::exp(c.z()));
}

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

class ice : public material {
public:
  ice(double refraction_index = 1.31,
      const color &absorption = color(0.60, 0.15, 0.06), double frost = 0.012,
      double cloudiness = 0.35)
      : refraction_index(refraction_index), absorption(absorption),
        frost(frost), cloudiness(cloudiness) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    vec3 unit_dir = unit_vector(r_in.direction());
    attenuation = color(1, 1, 1);

    if (!rec.front_face) {

      double dist = rec.t * r_in.direction().length();

      if (cloudiness > 0) {

        double d = -std::log(1 - random_double()) / cloudiness;
        if (d < dist) {
          attenuation = exp_color(-d * absorption);
          vec3 dir = unit_dir + 1.3 * random_unit_vector(); // mostly forward...
          if (dir.near_zero())
            dir = unit_dir;
          scattered = ray(r_in.origin() + d * unit_dir, dir);
          return true;
        }
      }
      attenuation = exp_color(-dist * absorption);
    }

    vec3 n = rec.normal;
    if (frost > 0) {
      vec3 jittered = unit_vector(rec.normal + frost * random_unit_vector());
      if (dot(jittered, -unit_dir) > 0)
        n = jittered;
    }

    double ri = rec.front_face ? (1.0 / refraction_index) : refraction_index;
    double cos_theta = std::fmin(dot(-unit_dir, n), 1.0);
    double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    bool total_internal_reflection = ri * sin_theta > 1.0;
    vec3 dir;
    if (total_internal_reflection ||
        random_double() < reflectance(cos_theta, ri)) {
      dir = reflect(unit_dir, n);
      if (dot(dir, rec.normal) <= 0) // jitter pushed it through the surface
        dir = reflect(unit_dir, rec.normal);
    } else {
      dir = refract(unit_dir, n, ri);
      if (dot(dir, rec.normal) >= 0)
        dir = refract(unit_dir, rec.normal, ri);
    }

    scattered = ray(rec.p, dir);
    return true;
  }

private:
  double refraction_index;
  color absorption; // per unit distance, per channel
  double frost;
  double cloudiness; // bubbles per unit distance (0 = perfectly clear)
};

class marble : public material {
public:
  marble(const color &base = color(0.92, 0.90, 0.86),
         const color &vein = color(0.22, 0.23, 0.26), double scale = 2.0,
         double turbulence = 3.0, double polish_fuzz = 0.03)
      : base(base), vein(vein), scale(scale), turbulence(turbulence),
        polish_fuzz(polish_fuzz) {}

  bool scatter(const ray &r_in, const hit_record &rec, color &attenuation,
               ray &scattered) const override {
    vec3 unit_dir = unit_vector(r_in.direction());
    double cos_theta = std::fmin(dot(-unit_dir, rec.normal), 1.0);

    if (random_double() < reflectance(cos_theta, 1.0 / 1.5)) {
      // glossy coat
      vec3 reflected =
          reflect(unit_dir, rec.normal) + polish_fuzz * random_unit_vector();
      if (dot(reflected, rec.normal) > 0) {
        scattered = ray(rec.p, reflected);
        attenuation = color(1, 1, 1);
        return true;
      }
    }

    // diffuse stone underneath...
    auto dir = rec.normal + random_unit_vector();
    if (dir.near_zero())
      dir = rec.normal;
    scattered = ray(rec.p, dir);
    attenuation = albedo_at(rec.local_p);
    return true;
  }

  // makes a nice veiny marble texture
  color albedo_at(const point3 &p) const {
    point3 q = scale * p;

    double phase = 2.0 * (q.x() + 0.6 * q.y() + 0.3 * q.z()) +
                   turbulence * noise.turb(q, 5);
    double closeness = 1 - std::fabs(std::sin(phase));
    double main_vein = std::pow(closeness, 16) + 0.22 * std::pow(closeness, 3);

    point3 q2 = 1.7 * q + point3(17.0, 3.0, 9.0);
    double phase2 = 1.5 * (q2.z() - 0.5 * q2.x()) + 2.5 * noise.turb(q2, 5);
    double fine_vein = 0.35 * std::pow(1 - std::fabs(std::sin(phase2)), 30);

    double mottle = 0.93 + 0.07 * noise.noise(4.0 * q);

    double v = std::fmin(1.0, main_vein + fine_vein);
    return mottle * ((1 - v) * base + v * vein);
  }

private:
  color base, vein;
  double scale, turbulence, polish_fuzz;
  perlin noise;
};