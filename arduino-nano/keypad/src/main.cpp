#include <Arduino.h>

#include "KeyMatrix.h"
#include "MatrixScanner.h"

namespace {

constexpr uint8_t ROW_PINS[keymatrix::ROWS] = {9, 8, 7, 6};
constexpr uint8_t COL_PINS[keymatrix::COLS] = {5, 4, 3, 2};
constexpr uint8_t PIN_LED = 10;  // brief pulse on each debounced keypress

constexpr uint32_t LED_PULSE_MS = 150;

matrixscanner::Scanner matrixScanner(ROW_PINS, COL_PINS);
keymatrix::Scanner     keyScanner;
uint32_t               ledOffAtMs = 0;

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
  char key = keyScanner.scan(matrixScanner.scan());
  if (key != '\0') {
    Serial.println(key);
    digitalWrite(PIN_LED, HIGH);
    ledOffAtMs = millis() + LED_PULSE_MS;
  }

  updateLed();
}
