#include "HobModel.h"

void HobModel::setLevel(uint8_t hob, uint8_t level) {
  if (hob >= config::HOB_COUNT) return;
  levels_[hob] = level;
}

uint8_t HobModel::level(uint8_t hob) const {
  if (hob >= config::HOB_COUNT) return 0;
  return levels_[hob];
}
