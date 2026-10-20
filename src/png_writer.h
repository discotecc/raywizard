#pragma once

// single image:   png_writer png("render.png");
//                 png.add_frame(w, h, rgb);
// image sequence: png_writer png("frames/shot.png", /*numbered=*/true);
//                 -> frames/shot_0000.png, frames/shot_0001.png, ...

#include "frame_writer.h"

#include <zlib.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class png_writer : public frame_writer {
public:
  explicit png_writer(std::string path, bool numbered = false)
      : path(std::move(path)), numbered(numbered) {}

  void add_frame(int width, int height,
                 const std::vector<uint8_t> &rgb) override {
    if (!numbered && frame_index > 0)
      throw std::runtime_error(
          "png_writer: more than one frame; use numbered = true");

    std::string out = numbered ? numbered_path(frame_index) : path;
    write_png(out, width, height, rgb);
    frame_index++;
  }

  static void write_png(const std::string &out_path, int width, int height,
                        const std::vector<uint8_t> &rgb) {
    if (width <= 0 || height <= 0 || rgb.size() != size_t(width) * height * 3)
      throw std::runtime_error("png_writer: bad image dimensions");

    std::vector<uint8_t> raw = filter_rows(width, height, rgb);

    uLongf packed_size = compressBound(uLong(raw.size()));
    std::vector<uint8_t> packed(packed_size);
    if (compress2(packed.data(), &packed_size, raw.data(), uLong(raw.size()),
                  Z_BEST_COMPRESSION) != Z_OK)
      throw std::runtime_error("png_writer: zlib compression failed");
    packed.resize(packed_size);

    std::FILE *f = std::fopen(out_path.c_str(), "wb");
    if (!f)
      throw std::runtime_error("could not open " + out_path);

    static const uint8_t signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    std::fwrite(signature, 1, 8, f);

    // size, 8 bits per channel, color type 2 (rgb), no interlace
    std::vector<uint8_t> ihdr;
    put32(ihdr, uint32_t(width));
    put32(ihdr, uint32_t(height));
    ihdr.push_back(8); // bit depth
    ihdr.push_back(2); // color type: truecolor RGB
    ihdr.push_back(0); // compression: deflate
    ihdr.push_back(0); // filter method: adaptive
    ihdr.push_back(0); // interlace: none

    write_chunk(f, "IHDR", ihdr);
    write_chunk(f, "IDAT", packed);
    write_chunk(f, "IEND", {});

    bool ok = std::ferror(f) == 0;
    ok = (std::fclose(f) == 0) && ok;
    if (!ok)
      throw std::runtime_error("png_writer: failed writing " + out_path);
  }

private:
  std::string path;
  bool numbered;
  int frame_index = 0;

  // "shot.png" + 7 -> "shot_0007.png"
  std::string numbered_path(int index) const {
    std::string stem = path, ext = ".png";
    auto dot = path.find_last_of('.');
    auto slash = path.find_last_of("/\\");
    if (dot != std::string::npos &&
        (slash == std::string::npos || dot > slash)) {
      stem = path.substr(0, dot);
      ext = path.substr(dot);
    }
    char num[16];
    std::snprintf(num, sizeof num, "_%04d", index);
    return stem + num + ext;
  }

  static void put32(std::vector<uint8_t> &v, uint32_t x) {
    v.push_back(uint8_t(x >> 24));
    v.push_back(uint8_t(x >> 16));
    v.push_back(uint8_t(x >> 8));
    v.push_back(uint8_t(x));
  }

  static void write_chunk(std::FILE *f, const char type[4],
                          const std::vector<uint8_t> &data) {
    std::vector<uint8_t> header;
    put32(header, uint32_t(data.size()));
    std::fwrite(header.data(), 1, 4, f);
    std::fwrite(type, 1, 4, f);
    if (!data.empty())
      std::fwrite(data.data(), 1, data.size(), f);

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const Bytef *>(type), 4);
    if (!data.empty())
      crc = crc32(crc, data.data(), uInt(data.size()));
    std::vector<uint8_t> tail;
    put32(tail, uint32_t(crc));
    std::fwrite(tail.data(), 1, 4, f);
  }

  static uint8_t paeth(int a, int b, int c) {
    int p = a + b - c;
    int pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc)
      return uint8_t(a);
    return uint8_t(pb <= pc ? b : c);
  }

  static std::vector<uint8_t> filter_rows(int width, int height,
                                          const std::vector<uint8_t> &rgb) {
    const size_t stride = size_t(width) * 3;
    const int bpp = 3; // bytes per pixel
    std::vector<uint8_t> out;
    out.reserve((stride + 1) * height);

    std::vector<uint8_t> zero_row(stride, 0), candidate(stride), best(stride);

    for (int y = 0; y < height; y++) {
      const uint8_t *cur = &rgb[y * stride];
      const uint8_t *up = y > 0 ? &rgb[(y - 1) * stride] : zero_row.data();

      uint8_t best_type = 0;
      long best_score = -1;

      for (uint8_t type = 0; type < 5; type++) {
        long score = 0;
        for (size_t i = 0; i < stride; i++) {
          int a = i >= size_t(bpp) ? cur[i - bpp] : 0; // left
          int b = up[i];                               // above
          int c = i >= size_t(bpp) ? up[i - bpp] : 0;  // above-left
          uint8_t pred = 0;
          switch (type) {
          case 1:
            pred = uint8_t(a);
            break;
          case 2:
            pred = uint8_t(b);
            break;
          case 3:
            pred = uint8_t((a + b) / 2);
            break;
          case 4:
            pred = paeth(a, b, c);
            break;
          }
          uint8_t v = uint8_t(cur[i] - pred);
          candidate[i] = v;
          score += std::abs(int(int8_t(v)));
        }
        if (best_score < 0 || score < best_score) {
          best_score = score;
          best_type = type;
          best.swap(candidate);
        }
      }

      out.push_back(best_type);
      out.insert(out.end(), best.begin(), best.end());
    }
    return out;
  }
};