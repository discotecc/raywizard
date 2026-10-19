#pragma once

// dependency free gif writer

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

class gif_writer {
public:
  // delay_cs: time each frame is shown, in hundredths of a second
  // loop_count: 0 = loop forever
  gif_writer(const std::string &path, int delay_cs, int loop_count = 0)
      : delay_cs(delay_cs), loop_count(loop_count) {
    f = std::fopen(path.c_str(), "wb");
    if (!f)
      throw std::runtime_error("could not open " + path);
  }

  ~gif_writer() {
    if (f) {
      std::fputc(0x3B, f); // trailer..
      std::fclose(f);
    }
  }

  gif_writer(const gif_writer &) = delete;
  gif_writer &operator=(const gif_writer &) = delete;

  // rgb: width*height*3 bytes, row-major, top row first...
  void add_frame(int width, int height, const std::vector<uint8_t> &rgb) {
    if (!header_written) {
      write_header(width, height);
      w = width;
      h = height;
      header_written = true;
    }
    if (width != w || height != h)
      throw std::runtime_error("all frames must be the same size");

    // Graphic Control Extension: sets the delay for this frame..
    put(0x21);
    put(0xF9);
    put(4);    // block size..
    put(0x04); // disposal = leave frame in place, no transparency..
    put16(delay_cs);
    put(0); // transparent color index (unused)
    put(0); // block terminator

    // Image Descriptor: frame covers the whole canvas, uses global palette
    put(0x2C);
    put16(0);
    put16(0);
    put16(width);
    put16(height);
    put(0);

    write_lzw(quantize(rgb));
  }

private:
  std::FILE *f = nullptr;
  int delay_cs, loop_count;
  int w = 0, h = 0;
  bool header_written = false;

  // color palete... 6 levels for red, 7 green, 6 blue... green is most
  // sensitive
  static constexpr int RL = 6, GL = 7, BL = 6;

  void put(int b) { std::fputc(b & 0xFF, f); }
  void put16(int v) {
    put(v & 0xFF);
    put((v >> 8) & 0xFF);
  }

  void write_header(int width, int height) {
    std::fwrite("GIF89a", 1, 6, f);

    // Logical Screen Descriptor
    put16(width);
    put16(height);
    put(0xF7); // global color table present, 8-bit color, table size 2^(7+1)
    put(0);    // background color index
    put(0);    // pixel aspect ratio (unspecified)

    // Global Color Table: 256 entries of RGB
    for (int i = 0; i < 256; i++) {
      if (i < RL * GL * BL) {
        int r = i / (GL * BL), g = (i / BL) % GL, b = i % BL;
        put(r * 255 / (RL - 1));
        put(g * 255 / (GL - 1));
        put(b * 255 / (BL - 1));
      } else {
        put(0);
        put(0);
        put(0);
      }
    }

    // NETSCAPE2.0 application extension... makes the animation loop...
    put(0x21);
    put(0xFF);
    put(11);
    std::fwrite("NETSCAPE2.0", 1, 11, f);
    put(3);
    put(1);
    put16(loop_count);
    put(0);
  }

  // RGB -> palette indices, with 8x8 ordered dithering.
  std::vector<uint8_t> quantize(const std::vector<uint8_t> &rgb) const {
    static const int bayer[8][8] = {
        {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26},
        {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
        {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 57, 25},
        {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

    std::vector<uint8_t> out(size_t(w) * h);
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // threshold in (-0.5, 0.5): nudges a value up or down before
        // rounding...
        double t = (bayer[y & 7][x & 7] + 0.5) / 64.0 - 0.5;
        const uint8_t *p = &rgb[(size_t(y) * w + x) * 3];
        int r = level(p[0], RL, t), g = level(p[1], GL, t),
            b = level(p[2], BL, t);
        out[size_t(y) * w + x] = uint8_t(r * GL * BL + g * BL + b);
      }
    }
    return out;
  }

  static int level(uint8_t v, int levels, double t) {
    double s = v / 255.0 * (levels - 1) + t;
    int l = int(s + 0.5);
    return l < 0 ? 0 : (l >= levels ? levels - 1 : l);
  }

  // LZW compression and stuff....

  struct bit_writer {
    std::vector<uint8_t> bytes;
    uint32_t acc = 0;
    int nbits = 0;
    void write(int code, int size) {
      acc |= uint32_t(code) << nbits;
      nbits += size;
      while (nbits >= 8) {
        bytes.push_back(uint8_t(acc & 0xFF));
        acc >>= 8;
        nbits -= 8;
      }
    }
    void flush() {
      if (nbits > 0)
        bytes.push_back(uint8_t(acc & 0xFF));
      acc = 0;
      nbits = 0;
    }
  };

  void write_lzw(const std::vector<uint8_t> &indices) {
    const int min_code_size = 8;
    const int clear_code = 1 << min_code_size; // 256
    const int end_code = clear_code + 1;       // 257

    std::vector<uint32_t> dict(4096 * 256, 0);
    uint32_t generation = 1;

    bit_writer bw;
    int code_size = min_code_size + 1;
    int max_code = end_code;
    bw.write(clear_code, code_size);

    int cur = -1;
    for (uint8_t px : indices) {
      if (cur < 0) {
        cur = px;
        continue;
      }
      uint32_t &entry = dict[size_t(cur) * 256 + px];
      if ((entry >> 12) == generation) {
        cur = int(entry & 0xFFF); // extend the current string...
        continue;
      }
      bw.write(cur, code_size);
      entry = (generation << 12) | uint32_t(++max_code);
      if (max_code >= (1 << code_size))
        code_size++;
      if (max_code == 4095) { // dictionary full: reset
        bw.write(clear_code, code_size);
        generation++;
        code_size = min_code_size + 1;
        max_code = end_code;
      }
      cur = px;
    }
    bw.write(cur, code_size);
    bw.write(clear_code, code_size);
    bw.write(end_code, min_code_size + 1);
    bw.flush();

    put(min_code_size);
    for (size_t i = 0; i < bw.bytes.size(); i += 255) {
      size_t n = std::min<size_t>(255, bw.bytes.size() - i);
      put(int(n));
      std::fwrite(&bw.bytes[i], 1, n, f);
    }
    put(0); // end of image data...
  }
};
