#pragma once

// Writes frames to an H.264 .mp4 by piping raw RGB into ffmpeg.
// Requires ffmpeg on your PATH (macOS: `brew install ffmpeg`).

#include "frame_writer.h"

#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class mp4_writer : public frame_writer {
public:
  // crf => quality, lower = better/larger (18 is visually near lossless)
  mp4_writer(std::string path, int fps, int crf = 18)
      : path(std::move(path)), fps(fps), crf(crf) {}

  ~mp4_writer() override {
    if (pipe)
      ::pclose(pipe);
  }

  mp4_writer(const mp4_writer &) = delete;
  mp4_writer &operator=(const mp4_writer &) = delete;

  void add_frame(int width, int height,
                 const std::vector<uint8_t> &rgb) override {
    if (!pipe)
      open(width, height);
    else if (width != w || height != h)
      throw std::runtime_error("all frames must be the same size");

    if (std::fwrite(rgb.data(), 1, rgb.size(), pipe) != rgb.size())
      throw std::runtime_error("ffmpeg stopped accepting frames");
  }

private:
  std::string path;
  int fps, crf;
  int w = 0, h = 0;
  std::FILE *pipe = nullptr;

  void open(int width, int height) {
    w = width;
    h = height;
    std::string cmd =
        "ffmpeg -y -loglevel error"
        " -f rawvideo -pixel_format rgb24"
        " -video_size " +
        std::to_string(width) + "x" + std::to_string(height) + " -framerate " +
        std::to_string(fps) +
        " -i -"
        // H.264 with yuv420p needs even dimensions... pad by 1px if odd
        " -vf \"pad=ceil(iw/2)*2:ceil(ih/2)*2\""
        " -c:v libx264 -preset medium -crf " +
        std::to_string(crf) +
        " -pix_fmt yuv420p -movflags +faststart"
        " \"" +
        path + "\"";

    pipe = ::popen(cmd.c_str(), "w");
    if (!pipe)
      throw std::runtime_error("could not start ffmpeg (is it installed?)");
  }
};