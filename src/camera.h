#pragma once

#include "color.h"
#include "intersectable.h"
#include "interval.h"
#include "material.h"
#include "ray.h"
#include "utility.h"
#include "vec3.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

class camera {
public:
  double aspect_ratio = 1.0;
  int image_width = 100;      // width of rendered image in pixels
  int samples_per_pixel = 10; // anti aliasing
  int max_depth = 10;         // max number of ray bounce recursions

  double v_fov = 90;
  point3 look_from = point3(0, 0, 0);
  point3 look_at = point3(0, 0, -1);
  vec3 v_up = vec3(0, 1, 0);

  // dividing image into groups of pixel_group_size pixels for threads to work
  // on
  int pixel_group_size = 16;
  unsigned thread_count = 0; // 0 = use all hardware threads

  std::vector<uint8_t> render_frame(const intersectable &scene) {
    initialize();
    std::vector<uint8_t> rgb(size_t(image_width) * image_height * 3);
    const int pixel_group_x =
        (image_width + pixel_group_size - 1) / pixel_group_size;
    const int pixel_group_y =
        (image_height + pixel_group_size - 1) / pixel_group_size;
    const int group_count = pixel_group_x * pixel_group_y;
    std::atomic<int> next_group{0};

    auto worker = [&] {
      for (int group = next_group.fetch_add(1, std::memory_order_relaxed);
           group < group_count;
           group = next_group.fetch_add(1, std::memory_order_relaxed)) {
        const int x0 = (group % pixel_group_x) * pixel_group_size;
        const int y0 = (group / pixel_group_x) * pixel_group_size;
        const int x1 = std::min(x0 + pixel_group_size, image_width);
        const int y1 = std::min(y0 + pixel_group_size, image_height);
        for (int j = y0; j < y1; j++) {
          for (int i = x0; i < x1; i++) {
            uint8_t *px = &rgb[(size_t(j) * image_width + i) * 3];
            color_to_bytes(render_pixel(i, j, scene), px[0], px[1], px[2]);
          }
        }
      }
    };

    unsigned n = thread_count
                     ? thread_count
                     : std::max(1u, std::thread::hardware_concurrency());
    n = std::min<unsigned>(n, unsigned(group_count));

    {
      std::vector<std::thread> workers;
      workers.reserve(n - 1);
      for (unsigned t = 1; t < n; ++t)
        workers.emplace_back(worker);
      worker();
      for (auto &th : workers)
        th.join();
    } // all threads joined here

    return rgb;
  }

  void render(const intersectable &scene) {
    auto rgb = render_frame(scene);
    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";
    for (size_t k = 0; k < rgb.size(); k += 3)
      std::cout << int(rgb[k]) << ' ' << int(rgb[k + 1]) << ' '
                << int(rgb[k + 2]) << '\n';
  }

  /*
  void render(const intersectable &scene) {
    initialize();

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = 0; j < image_height; j++) {
      std::clog << "\rScanlines remaining: " << (image_height - j) << ' '
                << std::flush;
      for (int i = 0; i < image_width; i++) {
        write_color(std::cout, render_pixel(i, j, scene));
      }
    }

    std::clog << "\rDone.                 \n";
  }
*/
  /*

  // Renders a single image into memory as width*height*3 RGB bytes
  // (row-major, top row first). Used for animation frames
  std::vector<uint8_t> render_frame(const intersectable &scene) {
    initialize();
    std::vector<uint8_t> rgb(size_t(image_width) * image_height * 3);

    for (int j = 0; j < image_height; j++) {
      for (int i = 0; i < image_width; i++) {
        uint8_t *px = &rgb[(size_t(j) * image_width + i) * 3];
        color_to_bytes(render_pixel(i, j, scene), px[0], px[1], px[2]);
      }
    }
    return rgb;
  }
    */

  int width() const { return image_width; }
  int height() const {
    int h = int(image_width / aspect_ratio);
    return h < 1 ? 1 : h;
  }

  vec3 sample_square() const {
    return vec3(random_double() - 0.5, random_double() - 0.5, 0);
  }

private:
  int image_height;           // Rendered image height
  double pixel_samples_scale; // color scale factor for a sum of pixel samples
  point3 center;              // camera center
  point3 pixel00_loc;         // location of top left pixel of image
  vec3 pixel_delta_u;         // vector to next pixel leftward
  vec3 pixel_delta_v;         // vector to next pixel downward

  void initialize() {
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;
    pixel_samples_scale = 1.0 / samples_per_pixel;
    center = look_from;
    // Determine viewport dimensions.
    auto focal_length = (look_from - look_at).length();
    auto h = std::tan(degrees_to_radians(v_fov) / 2);
    auto viewport_height = 2 * h * focal_length;
    auto viewport_width =
        viewport_height * (double(image_width) / image_height);
    vec3 w = unit_vector(look_from - look_at);
    vec3 u = unit_vector(cross(v_up, w));
    vec3 v = cross(w, u);
    auto viewport_u = viewport_width * u;
    auto viewport_v = viewport_height * -v;
    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;
    auto viewport_upper_left =
        center - (focal_length * w) - viewport_u / 2 - viewport_v / 2;
    pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
  }

  // Averages samples_per_pixel rays through pixel (i,j); returns linear
  // color.
  color render_pixel(int i, int j, const intersectable &scene) const {
    color pixel_color(0, 0, 0);
    for (int sample = 0; sample < samples_per_pixel; sample++) {
      ray r = get_ray(i, j);
      pixel_color += ray_color(r, max_depth, scene);
    }
    return pixel_samples_scale * pixel_color;
  }

  ray get_ray(int i, int j) const {
    // projects a ray from origin of camera directed at a randomly sampled
    // point near pixel location (i,j)

    auto offset = sample_square();
    auto pixel_sample = pixel00_loc + ((i + offset.x()) * pixel_delta_u) +
                        ((j + offset.y()) * pixel_delta_v);
    auto ray_origin = center;
    auto ray_direction = pixel_sample - ray_origin;
    return ray(ray_origin, ray_direction);
  }

  color ray_color(const ray &r, int depth, const intersectable &scene) const {

    if (depth <= 0) {
      return color(0, 0, 0);
    }

    hit_record rec;

    if (scene.hit(r, interval(0.001, infinity), rec)) {
      ray scattered;
      color attenuation;
      if (rec.mat->scatter(r, rec, attenuation, scattered))
        return attenuation * ray_color(scattered, depth - 1, scene);
      return color(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5 * (unit_direction.y() + 1.0);
    return (1.0 - a) * color(1, 1, 1) + a * color(0.5, 0.7, 1.0);
    // return (1.0 - a) * color(0.3, 0.5, 0.9); // + a * color(0.5, 0.7, 1.0);
    // return color(0.7, 0.2, 0.1);
  }
};