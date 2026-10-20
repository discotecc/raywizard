#pragma once

// common interface for rendered frames to various file types

#include <cstdint>
#include <vector>

class frame_writer {
public:
  virtual ~frame_writer() = default;

  virtual void add_frame(int width, int height,
                         const std::vector<uint8_t> &rgb) = 0;
};