#include <Arduino.h>

#include "Debounce.h"
#include "KeyMatrix.h"

namespace {

constexpr uint8_t ROWS = 4;
constexpr uint8_t COLS = 4;
constexpr uint8_t ROW_PINS[ROWS] = {9, 8, 7, 6};
constexpr uint8_t COL_PINS[COLS] = {5, 4, 3, 2};
constexpr uint8_t PIN_LED = 10;  // brief pulse on each debounced keypress

constexpr uint32_t LED_PULSE_MS = 150;

// Standard 4x4 membrane keypad layout.
constexpr char LAYOUT[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'},
};

keymatrix::MatrixScanner     matrixScanner(ROW_PINS, ROWS, COL_PINS, COLS);
debounce::Debouncer<uint8_t> keyDebouncer(keymatrix::NO_KEY);
uint32_t                     ledOffAtMs = 0;

void updateLed() {
  if (ledOffAtMs != 0 && millis() >= ledOffAtMs) {
    digitalWrite(PIN_LED, LOW);
    ledOffAtMs = 0;
  }
}

}  // namespace

void setup() {
  Serial.begin(9600);
  matrixScanner.begin();
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  if (keyDebouncer.update(matrixScanner.scan()) && keyDebouncer.value() != keymatrix::NO_KEY) {
    uint8_t index = keyDebouncer.value();
    Serial.println(LAYOUT[index / COLS][index % COLS]);
    digitalWrite(PIN_LED, HIGH);
    ledOffAtMs = millis() + LED_PULSE_MS;
  }

  updateLed();
}
