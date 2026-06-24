#include "LedRenderer.h"

#include "HeatRamp.h"

LedRenderer::LedRenderer(CRGB* leds, uint8_t hobCount, uint8_t ledsPerRing)
    : leds_(leds), hobCount_(hobCount), ledsPerRing_(ledsPerRing) {}

void LedRenderer::render(const HobModel& model) {
  for (uint8_t hob = 0; hob < hobCount_; ++hob) {
    heatramp::Hsv c = heatramp::colorFor(model.level(hob));
    CRGB color = (c.v == 0) ? CRGB(CRGB::Black) : CRGB(CHSV(c.h, c.s, c.v));
    uint16_t start = static_cast<uint16_t>(hob) * ledsPerRing_;
    for (uint8_t i = 0; i < ledsPerRing_; ++i) {
      leds_[start + i] = color;
    }
  }
}
