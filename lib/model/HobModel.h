#pragma once

#include <stdint.h>

#include "Config.h"

// Model: the heat level of each hob, 0 (off) .. 255 (max). Pure state and logic — no hardware,
// so it is unit-tested off-device.
class HobModel {
 public:
  void setLevel(uint8_t hob, uint8_t level);
  uint8_t level(uint8_t hob) const;
  uint8_t hobCount() const { return config::HOB_COUNT; }

 private:
  uint8_t levels_[config::HOB_COUNT] = {0};
};
