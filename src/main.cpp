#include "camera.h"
#include "intersectable_list.h"
#include "material.h"
#include "sphere.h"

#include <memory>

int main() {

  intersectable_list scene;

  /*
  auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
  auto material_center = std::make_shared<lambertian>(color(0.1, 0.2, 0.5));
  auto material_left = std::make_shared<metal>(color(0.8, 0.8, 0.8), 0.3);
  auto material_right = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);
  auto material_floating1 = std::make_shared<dielectric>(1.3);

  */

  /*
  scene.add(std::make_shared<sphere>(point3(0.0, -100.5, -1.0), 100.0,
                                     material_ground));
  scene.add(
      std::make_shared<sphere>(point3(0.0, 0.0, -1.2), 0.5, material_center));
  scene.add(
      std::make_shared<sphere>(point3(-1.0, 0.0, -1.0), 0.5, material_left));
  scene.add(
      std::make_shared<sphere>(point3(1.0, 0.0, -1.0), 0.5, material_right));

  scene.add(std::make_shared<sphere>(point3(0.0, 0.8, -1.0), 0.25,
                                     material_floating1));
*/

  auto material_ground = std::make_shared<checker>(
      color(0.0, 0.0, 0.0), color(1.0, 1.0, 1.0), 0.5, 0.0);

  auto material_sky = std::make_shared<lambertian>(color(0.0, 0.0, 0.8));

  auto material_glass = std::make_shared<dielectric>(1.5);
  auto material_chrome = std::make_shared<metal>(color(0.90, 0.90, 0.92), 0.0);
  auto material_matte =
      std::make_shared<lambertian>(color(0.0, 0.5, 0.5)); // Win95 desktop teal

  scene.add(
      std::make_shared<sphere>(point3(0.0, 0.0, -2.0), 5000.0, material_sky));
  scene.add(std::make_shared<sphere>(point3(0.0, -1000.5, -2.0), 1000.0,
                                     material_ground));
  scene.add(
      std::make_shared<sphere>(point3(0.0, 0.35, -2.2), 0.60, material_glass));
  scene.add(
      std::make_shared<sphere>(point3(1.1, 0.20, -3.1), 0.45, material_chrome));
  scene.add(
      std::make_shared<sphere>(point3(-1.1, 0.10, -2.9), 0.35, material_matte));

  camera cam;
  cam.look_from = point3(0, 0.9, 0.8);
  cam.look_at = point3(0, 0.1, -2.6);
  cam.v_fov = 45;
  cam.image_width = 720;
  cam.samples_per_pixel = 50;
  cam.max_depth = 25;

  cam.render(scene);
}