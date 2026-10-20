#include "camera.h"
#include "frame_writer.h"
#include "gif_writer.h"
#include "intersectable_list.h"
#include "material.h"
#include "mp4_writer.h"
#include "png_writer.h"
#include "shapes.h"
#include "sphere.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>

// gif settings
const int gif_frame_count = 48;   // frames in one loop
const int gif_frame_delay_cs = 4; // hundredths of a second per frame (4 = 25
// fps)

// mp4 settings
const int mp4_fps = 60;
const int mp4_seonds = 10;

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

intersectable_list build_scene1(double t) {
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
      point3(0.15, 0.15 + bob(t, 0.15, 1, 0.75), -4.0), 0.3, material_chrome));

  return scene;
}

intersectable_list build_scene2(double t) {
  static auto material_checker = std::make_shared<checker>(
      color(1, 0.0, 0.0), color(0.0, 0.0, 1.0), 0.025, 0.0);

  static auto material_ground = std::make_shared<checker>(
      color(0.0, 0.0, 0.0), color(1.0, 1.0, 1.0), 0.7, 0.0);
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
           0.4 + (pi / 2) * t, material_chrome);

  add_pyramid(scene, point3(-1.15, -0.32 + bob(t, 0.12, 1, 0.50), -2.9), 0.75,
              0.75, 0.3 - 2 * pi * t, material_checker);

  scene.add(std::make_shared<sphere>(
      point3(1.1, 0.20 + bob(t, 0.12, 2, 0.25), -3.1), 0.45, material_marble));

  scene.add(std::make_shared<sphere>(
      point3(-0.1, 0.6 + bob(t, 0.15, 1, 0.75), -4.0), 0.3, material_ice));

  return scene;
}

// moves camera on a horizontal circle, always looking at center
void orbit_camera(camera &cam, double t) {
  const point3 target(0.0, 0.1, -3.0);
  const double radius = 3.8;
  const double height = 0.9;

  double angle = 2 * pi * t;
  cam.look_from = point3(target.x() + radius * std::sin(angle), height,
                         target.z() + radius * std::cos(angle));
  cam.look_at = target;
}

camera make_camera(bool video) {
  camera cam;
  cam.v_fov = 45;
  cam.samples_per_pixel = 50;
  cam.max_depth = 25;

  if (video) {
    cam.aspect_ratio = 16.0 / 9.0;
    cam.image_width = 1440;
  } else {
    cam.aspect_ratio = 1.0;
    cam.image_width = 1440;
  }
  orbit_camera(cam, 0.0);
  return cam;
}

static bool ends_with(const std::string &s, const std::string &suffix) {
  return s.size() >= suffix.size() &&
         s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char **argv) {
  std::string arg = argc > 1 ? argv[1] : "";
  // camera cam = make_camera();

  if (arg == "--still") {
    camera cam = make_camera(false);
    orbit_camera(cam, 0.0);
    cam.render(build_scene2(0.0));
    return 0;
  }

  std::string out_path = arg.empty() ? "out.gif" : arg;

  if (ends_with(out_path, ".png")) {
    camera cam = make_camera(false);
    png_writer png(out_path);
    png.add_frame(cam.width(), cam.height(),
                  cam.render_frame(build_scene2(0.0)));
    std::clog << "Done... Wrote " << out_path << "\n";
  }

  const bool video = ends_with(out_path, ".mp4");
  const int frame_count = video ? mp4_fps * mp4_seonds : gif_frame_count;

  camera cam = make_camera(video);

  std::unique_ptr<frame_writer> writer;
  if (video)
    writer = std::make_unique<mp4_writer>(out_path, mp4_fps);
  else
    writer = std::make_unique<gif_writer>(out_path, gif_frame_delay_cs);

  for (int f = 0; f < frame_count; f++) {
    std::clog << "\rRendering frame " << (f + 1) << " / " << frame_count
              << "   " << std::flush;
    double t = double(f) / frame_count; // 0 <= t < 1, never reaches 1
    orbit_camera(cam, t);
    auto scene = build_scene2(t);
    writer->add_frame(cam.width(), cam.height(), cam.render_frame(scene));
  }

  writer.reset();
  std::clog << "\rDone... Wrote " << out_path << "\n";
  std::clog << "*fart*" << "\n";
}
