#include <Arduino.h>

#include "ET6226M.h"

namespace {

constexpr uint8_t PIN_CLK = 8;
constexpr uint8_t PIN_DAT = 9;
constexpr uint32_t DIGIT_ADVANCE_MS = 500;

ET6226M  display(PIN_CLK, PIN_DAT);
uint16_t counter = 0;
uint32_t lastAdvanceMs = 0;
uint8_t  lastKeyCode = 0x00;

void showCounter() {
  uint8_t segments[ET6226M::GRID_COUNT] = {
      et6226m::encodeDigit(static_cast<uint8_t>((counter / 1000) % 10)),
      et6226m::encodeDigit(static_cast<uint8_t>((counter / 100) % 10)),
      et6226m::encodeDigit(static_cast<uint8_t>((counter / 10) % 10)),
      et6226m::encodeDigit(static_cast<uint8_t>(counter % 10)),
  };
  display.setSegments(segments);
}

}  // namespace

void setup() {
  Serial.begin(9600);
  display.begin();
}

void loop() {
  if (millis() - lastAdvanceMs >= DIGIT_ADVANCE_MS) {
    lastAdvanceMs = millis();
    counter = static_cast<uint16_t>((counter + 1) % 10000);
    showCounter();
  }

  uint8_t keyCode = display.readKeyCode();
  if (keyCode != lastKeyCode) {
    lastKeyCode = keyCode;
    et6226m::KeyPosition key = et6226m::decodeKeyCode(keyCode);
    if (key.grid != 0) {
      Serial.print("key grid=");
      Serial.print(key.grid);
      Serial.print(" segment=");
      Serial.println(key.segment);
    }
  }
}
