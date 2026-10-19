#include "camera.h"
#include "gif_writer.h"
#include "intersectable_list.h"
#include "material.h"
#include "shapes.h"
#include "sphere.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

const int frame_count = 48;   // frames in one loop
const int frame_delay_cs = 4; // hundredths of a second per frame (4 = 25 fps)

double bob(double t, double amplitude, int cycles, double phase) {
  return amplitude * std::sin(2 * pi * (cycles * t + phase));
}

intersectable_list build_sphere_scene(double t) {
  static auto material_ground = std::make_shared<checker>(
      color(0.0, 0.0, 0.0), color(1.0, 1.0, 1.0), 0.5, 0.0);
  static auto material_sky = std::make_shared<lambertian>(color(0.0, 0.0, 0.8));
  static auto material_glass = std::make_shared<dielectric>(1.5);
  static auto material_chrome =
      std::make_shared<metal>(color(0.90, 0.90, 0.92), 0.0);
  static auto material_matte =
      std::make_shared<lambertian>(color(0.0, 0.5, 0.5)); // Win95 desktop teal

  intersectable_list scene;

  (void)material_sky;

  scene.add(std::make_shared<sphere>(point3(0.0, -1000.5, -2.0), 1000.0,
                                     material_ground));

  scene.add(std::make_shared<sphere>(
      point3(0.0, 0.35 + bob(t, 0.15, 1, 0.00), -2.2), 0.60, material_glass));
  scene.add(std::make_shared<sphere>(
      point3(1.1, 0.20 + bob(t, 0.12, 2, 0.25), -3.1), 0.45, material_chrome));
  scene.add(std::make_shared<sphere>(
      point3(-1.1, 0.10 + bob(t, 0.18, 1, 0.50), -2.9), 0.35, material_matte));

  return scene;
}

intersectable_list build_scene(double t) {
  static auto material_ground = std::make_shared<checker>(
      color(0.0, 0.0, 0.0), color(1.0, 1.0, 1.0), 0.5, 0.0);
  static auto material_chrome =
      std::make_shared<metal>(color(0.90, 0.90, 0.92), 0.0);
  static auto material_matte =
      std::make_shared<lambertian>(color(0.0, 0.5, 0.5)); // Win95 desktop teal
  static auto material_ice = std::make_shared<ice>();
  static auto material_marble = std::make_shared<marble>();

  intersectable_list scene;
  scene.add(std::make_shared<sphere>(point3(0.0, -1000.5, -2.0), 1000.0,
                                     material_ground));

  add_cube(scene, point3(0.0, 0.0 + bob(t, 0.12, 1, 0.00), -2.3), 0.65,
           0.4 + (pi / 2) * t, material_ice);

  add_pyramid(scene, point3(-1.15, -0.32 + bob(t, 0.12, 1, 0.50), -2.9), 0.75,
              0.75, 0.3 - 2 * pi * t, material_marble);

  scene.add(std::make_shared<sphere>(
      point3(1.1, 0.20 + bob(t, 0.12, 2, 0.25), -3.1), 0.45, material_chrome));

  scene.add(std::make_shared<sphere>(
      point3(0.15, 0.15 + bob(t, 0.15, 1, 0.75), -4.0), 0.3, material_matte));

  return scene;
}

camera make_camera() {
  camera cam;
  cam.look_from = point3(0, 0.9, 0.8);
  cam.look_at = point3(0, 0.1, -2.6);
  cam.v_fov = 45;
  // cam.image_width = 720; //this takes too long...
  cam.image_width = 1080;
  cam.samples_per_pixel = 50;
  cam.max_depth = 25;
  return cam;
}

// Usage:
//   ./raywizard                        -> writes animation to out.gif
//   ./raywizard name.gif               -> writes animation to name.gif
//   ./raytracer --stxsill > name.ppm   -> single image name.pmm
int main(int argc, char **argv) {
  std::string arg = argc > 1 ? argv[1] : "";
  camera cam = make_camera();

  if (arg == "--still") {
    cam.render(build_scene(0.0));
    return 0;
  }

  std::string out_path = arg.empty() ? "out.gif" : arg;
  gif_writer gif(out_path, frame_delay_cs);

  for (int f = 0; f < frame_count; f++) {
    std::clog << "\rRendering frame " << (f + 1) << " / " << frame_count
              << "   " << std::flush;
    double t = double(f) / frame_count; // 0 <= t < 1, never reaches 1
    auto scene = build_scene(t);
    gif.add_frame(cam.width(), cam.height(), cam.render_frame(scene));
  }

  std::clog << "\rDone. Wrote " << out_path << "            \n";
}
