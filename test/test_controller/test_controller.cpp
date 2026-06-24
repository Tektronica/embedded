#include <unity.h>

#include "LEDStripDimmer.h"

using namespace controller;

void setUp() {}
void tearDown() {}

// --- Levels ---

void test_default_levels_are_zero() {
  Levels m;
  for (uint8_t ch = 0; ch < m.count(); ++ch) TEST_ASSERT_EQUAL_UINT8(0, m.level(ch));
}

void test_set_and_get_level() {
  Levels m;
  m.setLevel(0, 200);
  m.setLevel(3, 50);
  TEST_ASSERT_EQUAL_UINT8(200, m.level(0));
  TEST_ASSERT_EQUAL_UINT8(50, m.level(3));
}

void test_out_of_range_channel_is_ignored() {
  Levels m;
  m.setLevel(99, 123);
  TEST_ASSERT_EQUAL_UINT8(0, m.level(99));
  TEST_ASSERT_EQUAL_UINT8(0, m.level(0));
}

// --- Dimmer input ---

void test_adc_endpoints() {
  TEST_ASSERT_EQUAL_UINT8(0, adcToLevel(0));
  TEST_ASSERT_EQUAL_UINT8(255, adcToLevel(1023));
}

void test_adc_clamps_above_max() { TEST_ASSERT_EQUAL_UINT8(255, adcToLevel(5000)); }

void test_ema_reaches_target_exactly() {
  uint16_t s = 0;
  for (int i = 0; i < 200; ++i) s = emaStep(s, 800, 3);
  TEST_ASSERT_EQUAL_UINT16(800, s);
  for (int i = 0; i < 200; ++i) s = emaStep(s, 1023, 3);
  TEST_ASSERT_EQUAL_UINT16(1023, s);  // dimmer max reaches the rail -> full brightness
}

// --- Brightness ---

void test_level_brightness() {
  TEST_ASSERT_EQUAL_UINT8(0, levelBrightness(0));      // off
  TEST_ASSERT_EQUAL_UINT8(255, levelBrightness(255));  // full at max
  TEST_ASSERT_TRUE(levelBrightness(255) > levelBrightness(20));
}

// --- Palettes ---

void test_heat_palette_is_red_orange() {
  TEST_ASSERT_EQUAL_UINT8(0, paletteHue(Palette::HeatRedOrange, 1, 0, LED_STRIP_LENGTH));   // cool = red
  TEST_ASSERT_TRUE(paletteHue(Palette::HeatRedOrange, 255, 0, LED_STRIP_LENGTH) <= 16);     // hot = orange-red
}

void test_fixed_palettes_have_fixed_hue() {
  TEST_ASSERT_EQUAL_UINT8(HUE_GREEN, paletteHue(Palette::Green, 200, 0, LED_STRIP_LENGTH));
  TEST_ASSERT_EQUAL_UINT8(HUE_BLUE, paletteHue(Palette::Blue, 200, 17, LED_STRIP_LENGTH));
}

void test_white_is_desaturated_others_saturated() {
  TEST_ASSERT_EQUAL_UINT8(0, paletteSaturation(Palette::White));
  TEST_ASSERT_EQUAL_UINT8(255, paletteSaturation(Palette::HeatRedOrange));
  TEST_ASSERT_EQUAL_UINT8(255, paletteSaturation(Palette::Rainbow));
}

void test_rainbow_spans_the_strip() {
  uint8_t h0 = paletteHue(Palette::Rainbow, 200, 0, LED_STRIP_LENGTH);
  uint8_t hMid = paletteHue(Palette::Rainbow, 200, LED_STRIP_LENGTH / 2, LED_STRIP_LENGTH);
  uint8_t hEnd = paletteHue(Palette::Rainbow, 200, LED_STRIP_LENGTH - 1, LED_STRIP_LENGTH);
  TEST_ASSERT_EQUAL_UINT8(0, h0);          // first pixel = hue 0
  TEST_ASSERT_TRUE(hMid > h0 && hEnd > hMid);  // hue increases across the strip
}

// --- pixelColor (color + mode + level) ---

void test_pixel_off_when_level_zero() {
  TEST_ASSERT_EQUAL_UINT8(0, pixelColor(Palette::Green, Mode::Solid, 0, 0, LED_STRIP_LENGTH, 0).v);
}

void test_solid_is_always_on() {
  for (uint16_t f = 0; f < 120; ++f)
    TEST_ASSERT_TRUE(pixelColor(Palette::Green, Mode::Solid, 200, 0, LED_STRIP_LENGTH, f).v > 0);
}

void test_blink_toggles_over_time() {
  bool on = pixelColor(Palette::Green, Mode::Blink, 200, 0, LED_STRIP_LENGTH, 0).v > 0;
  bool offLater = pixelColor(Palette::Green, Mode::Blink, 200, 0, LED_STRIP_LENGTH,
                             anim::BLINK_PERIOD - 1).v > 0;
  TEST_ASSERT_TRUE(on);          // on in the first half
  TEST_ASSERT_FALSE(offLater);   // off in the second half
}

void test_chase_lights_only_a_moving_window() {
  uint8_t litAtFrame0 = 0;
  for (uint8_t i = 0; i < LED_STRIP_LENGTH; ++i)
    if (pixelColor(Palette::Green, Mode::Chase, 200, i, LED_STRIP_LENGTH, 0).v > 0) ++litAtFrame0;
  TEST_ASSERT_EQUAL_UINT8(anim::CHASE_WIDTH, litAtFrame0);  // only the segment is lit
}

// --- Switch cycling ---

void test_palette_cycles_and_wraps() {
  TEST_ASSERT_TRUE(nextPalette(Palette::HeatRedOrange) == Palette::Green);
  Palette last = static_cast<Palette>(static_cast<uint8_t>(Palette::Count) - 1);
  TEST_ASSERT_TRUE(nextPalette(last) == Palette::HeatRedOrange);  // wraps to default
}

void test_mode_cycles_and_wraps() {
  TEST_ASSERT_TRUE(nextMode(Mode::Solid) == Mode::Blink);
  Mode last = static_cast<Mode>(static_cast<uint8_t>(Mode::Count) - 1);
  TEST_ASSERT_TRUE(nextMode(last) == Mode::Solid);  // wraps to default
}

// --- Button (debounce + single rising edge) ---

void test_button_fires_once_per_debounced_press() {
  Button b;
  int edges = 0;
  bool seq[] = {false, true, true, true, true, false, false, false};  // press (held), release
  for (bool s : seq)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(1, edges);  // exactly one press detected
}

void test_button_ignores_bounce() {
  Button b;
  int edges = 0;
  bool bounce[] = {true, false, true, false, true, false};  // never DEBOUNCE-consistent
  for (bool s : bounce)
    if (b.pressed(s)) ++edges;
  TEST_ASSERT_EQUAL_INT(0, edges);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_levels_are_zero);
  RUN_TEST(test_set_and_get_level);
  RUN_TEST(test_out_of_range_channel_is_ignored);
  RUN_TEST(test_adc_endpoints);
  RUN_TEST(test_adc_clamps_above_max);
  RUN_TEST(test_ema_reaches_target_exactly);
  RUN_TEST(test_level_brightness);
  RUN_TEST(test_heat_palette_is_red_orange);
  RUN_TEST(test_fixed_palettes_have_fixed_hue);
  RUN_TEST(test_white_is_desaturated_others_saturated);
  RUN_TEST(test_rainbow_spans_the_strip);
  RUN_TEST(test_pixel_off_when_level_zero);
  RUN_TEST(test_solid_is_always_on);
  RUN_TEST(test_blink_toggles_over_time);
  RUN_TEST(test_chase_lights_only_a_moving_window);
  RUN_TEST(test_palette_cycles_and_wraps);
  RUN_TEST(test_mode_cycles_and_wraps);
  RUN_TEST(test_button_fires_once_per_debounced_press);
  RUN_TEST(test_button_ignores_bounce);
  return UNITY_END();
}
