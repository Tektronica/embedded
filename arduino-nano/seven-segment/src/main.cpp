#include <Arduino.h>
#include <TM1637Display.h>
#include <stdio.h>

#include "Button.h"
#include "SevenSegment.h"

// Demonstrates that Mode is independent of Content: the value on screen is always the same
// live counter -- pressing the button only ever changes currentMode, never what's being shown.
// Static/Flashing display the counter's leading 4 digits; Rolling scrolls the identical counter
// value (repeated, with a gap) across the window. Same content, three presentations.
namespace {

constexpr uint8_t PIN_DISPLAY_CLK = A0;
constexpr uint8_t PIN_DISPLAY_DIO = A1;
constexpr uint8_t DISPLAY_BRIGHTNESS = 7;  // 0..7, this library's max
constexpr uint8_t PIN_BUTTON = 2;          // momentary pushbutton, wired to GND (INPUT_PULLUP)

constexpr uint32_t FRAME_INTERVAL_MS    = 50;  // one sevenseg "frame" tick every 50ms
constexpr uint16_t FLASH_PERIOD_FRAMES  = 20;  // ~1s on/off cycle at 50ms/frame
constexpr uint16_t ROLL_FRAMES_PER_STEP = 6;   // ~300ms per scroll step at 50ms/frame

// "NNNN    NNNN" -- the same counter value shown twice with a gap, so Rolling has something to
// scroll through while Static/Flashing (which only ever show the first 4 characters) still show
// the counter itself, not a truncated fragment of unrelated text.
constexpr uint8_t LABEL_LENGTH = 12;

sevenseg::Mode nextMode(sevenseg::Mode m) {
  switch (m) {
    case sevenseg::Mode::Static:   return sevenseg::Mode::Flashing;
    case sevenseg::Mode::Flashing: return sevenseg::Mode::Rolling;
    case sevenseg::Mode::Rolling:
    default:                       return sevenseg::Mode::Static;
  }
}

uint16_t periodFramesFor(sevenseg::Mode m) {
  switch (m) {
    case sevenseg::Mode::Flashing: return FLASH_PERIOD_FRAMES;
    case sevenseg::Mode::Rolling:  return ROLL_FRAMES_PER_STEP;
    case sevenseg::Mode::Static:
    default:                       return 0;
  }
}

sevenseg::Mode currentMode = sevenseg::Mode::Static;
input::Button  button;
TM1637Display  display(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

uint16_t frame       = 0;
uint32_t lastFrameMs = 0;
uint16_t counter     = 0;  // the one value on screen, counts up once/second regardless of mode
uint32_t lastCountMs = 0;

}  // namespace

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  display.setBrightness(DISPLAY_BRIGHTNESS);
}

void loop() {
  if (button.pressed(digitalRead(PIN_BUTTON) == LOW)) {
    currentMode = nextMode(currentMode);
    frame = 0;  // restart the new mode's animation from its beginning
  }

  if (millis() - lastFrameMs >= FRAME_INTERVAL_MS) {
    lastFrameMs = millis();
    ++frame;
  }
  if (millis() - lastCountMs >= 1000) {
    lastCountMs = millis();
    counter = (counter + 1) % 10000;
  }

  char label[LABEL_LENGTH + 1];
  snprintf(label, sizeof(label), "%04u    %04u", counter, counter);

  sevenseg::Segments segments = sevenseg::render(sevenseg::labelContent(label, LABEL_LENGTH),
                                                  currentMode, frame, periodFramesFor(currentMode));
  display.setSegments(segments.values);
}
