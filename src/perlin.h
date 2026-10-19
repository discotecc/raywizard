#pragma once

// Perlin gradient noise: a smooth, random-looking function of 3D position.
// noise(p) returns roughly [-1, 1]; turb(p) layers several octaves of noise
// at doubling frequencies to get the texture of natural stone...

#include "utility.h"
#include "vec3.h"

#include <cmath>
#include <utility>

class perlin {
public:
  perlin() {
    for (int i = 0; i < point_count; i++)
      randvec[i] = unit_vector(vec3::random(-1, 1));
    make_perm(perm_x);
    make_perm(perm_y);
    make_perm(perm_z);
  }

  double noise(const point3 &p) const {
    double u = p.x() - std::floor(p.x());
    double v = p.y() - std::floor(p.y());
    double w = p.z() - std::floor(p.z());
    int i = int(std::floor(p.x()));
    int j = int(std::floor(p.y()));
    int k = int(std::floor(p.z()));

    vec3 c[2][2][2];
    for (int di = 0; di < 2; di++)
      for (int dj = 0; dj < 2; dj++)
        for (int dk = 0; dk < 2; dk++)
          c[di][dj][dk] =
              randvec[perm_x[(i + di) & 255] ^ perm_y[(j + dj) & 255] ^
                      perm_z[(k + dk) & 255]];

    double uu = u * u * (3 - 2 * u);
    double vv = v * v * (3 - 2 * v);
    double ww = w * w * (3 - 2 * w);

    double accum = 0;
    for (int di = 0; di < 2; di++)
      for (int dj = 0; dj < 2; dj++)
        for (int dk = 0; dk < 2; dk++) {
          vec3 weight(u - di, v - dj, w - dk);
          accum += (di * uu + (1 - di) * (1 - uu)) *
                   (dj * vv + (1 - dj) * (1 - vv)) *
                   (dk * ww + (1 - dk) * (1 - ww)) * dot(c[di][dj][dk], weight);
        }
    return accum;
  }

  double turb(const point3 &p, int depth = 7) const {
    double accum = 0;
    point3 temp = p;
    double weight = 1.0;
    for (int i = 0; i < depth; i++) {
      accum += weight * noise(temp);
      weight *= 0.5;
      temp *= 2;
    }
    return std::fabs(accum);
  }

private:
  static const int point_count = 256;
  vec3 randvec[point_count];
  int perm_x[point_count], perm_y[point_count], perm_z[point_count];

  static void make_perm(int *p) {
    for (int i = 0; i < point_count; i++)
      p[i] = i;
    for (int i = point_count - 1; i > 0; i--) {
      int target = int(random_double(0, i + 1));
      std::swap(p[i], p[target]);
    }
  }
};
