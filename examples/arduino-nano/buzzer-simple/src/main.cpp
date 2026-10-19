#include <Arduino.h>

// Naive style, on purpose: blocking tone()+delay(), no debounce, no state machine. This is the
// "before" contrasted against arduino-nano/buzzer-song's non-blocking, multi-song player -- see
// that project's README for why blocking delay() calls are worth avoiding once a sketch grows
// past "one button, one beep."
namespace {

constexpr uint8_t PIN_BUZZER = 9;
constexpr uint8_t PIN_BUTTON = 2;  // momentary pushbutton, wired to GND (INPUT_PULLUP)

constexpr uint16_t BEEP_HZ = 440;
constexpr uint16_t BEEP_MS = 200;

}  // namespace

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    tone(PIN_BUZZER, BEEP_HZ);
    delay(BEEP_MS);
    noTone(PIN_BUZZER);
    delay(BEEP_MS);  // no debounce -- just a pause so a held button beeps, not screeches
  }
}
