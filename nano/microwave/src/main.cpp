#include <Arduino.h>
#include <TM1637Display.h>

#include "Buzzer.h"
#include "KeyMatrix.h"
#include "Microwave.h"
#include "SevenSegment.h"

namespace {

// Matrix rows/cols — a 4x4 matrix keypad (see README's BOM; tested against Wokwi's matrix pad).
constexpr uint8_t ROW_PINS[keymatrix::ROWS] = {2, 3, 4, 5};
constexpr uint8_t COL_PINS[keymatrix::COLS] = {6, 7, 8, 9};

constexpr uint8_t PIN_BUZZER = 10;
constexpr uint8_t PIN_MOTOR  = 11;
constexpr uint8_t PIN_FAN    = 12;
constexpr uint8_t PIN_LIGHT  = 13;

constexpr uint8_t PIN_DISPLAY_CLK = A0;
constexpr uint8_t PIN_DISPLAY_DIO = A1;
constexpr uint8_t DISPLAY_BRIGHTNESS = 7;         // 0..7, this library's max
constexpr uint16_t BLINK_PERIOD_MS = 1000;        // colon/display blink cycle while Setting/Done
// Which digit's decimal-point segment drives the physical colon varies by board. 0x80 on digit
// index 1 matches most common 4-digit TM1637 clock boards, but confirm against this one once
// it's wired up and adjust COLON_DIGIT_INDEX/COLON_BIT if the colon doesn't light.
constexpr uint8_t COLON_DIGIT_INDEX = 1;
constexpr uint8_t COLON_BIT = 0x80;

keymatrix::Scanner       keyScanner;
microwave::Controller     controller;
microwave::State          previousState = microwave::State::Idle;
TM1637Display             display(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

buzzer::Pattern activeBuzzerPattern  = buzzer::Pattern::None;
uint32_t        buzzerPatternStartMs = 0;
uint32_t        lastTickMs           = 0;

// Scan all rows/columns once; returns the raw key index for keymatrix::Scanner, or NO_KEY.
uint8_t scanRawKeyIndex() {
  for (uint8_t r = 0; r < keymatrix::ROWS; ++r) {
    digitalWrite(ROW_PINS[r], LOW);
    for (uint8_t c = 0; c < keymatrix::COLS; ++c) {
      if (digitalRead(COL_PINS[c]) == LOW) {
        digitalWrite(ROW_PINS[r], HIGH);
        return static_cast<uint8_t>(r * keymatrix::COLS + c);
      }
    }
    digitalWrite(ROW_PINS[r], HIGH);
  }
  return keymatrix::NO_KEY;
}

// Map a settled key character to a microwave::Event. Returns false for keys this project
// doesn't use (A-D) — the keypad is a standard 16-key layout, but only 0-9/#/* map to anything.
bool translateKey(char key, microwave::Event& event) {
  if (key >= '0' && key <= '9') {
    event = microwave::Event{microwave::EventType::Digit, static_cast<uint8_t>(key - '0')};
    return true;
  }
  if (key == '#') {
    event = microwave::Event{microwave::EventType::Start, 0};
    return true;
  }
  if (key == '*') {
    event = microwave::Event{microwave::EventType::Cancel, 0};
    return true;
  }
  return false;
}

void startBuzzerPattern(buzzer::Pattern pattern) {
  activeBuzzerPattern = pattern;
  buzzerPatternStartMs = millis();
}

// Renders the active one-shot pattern (key press / done / error) to the buzzer pin; once it
// finishes, falls back to the ambient Hum while Running, or silence otherwise.
void updateBuzzer() {
  uint32_t elapsedMs = millis() - buzzerPatternStartMs;
  if (buzzer::isFinished(activeBuzzerPattern, elapsedMs)) {
    activeBuzzerPattern = buzzer::Pattern::None;
  }

  buzzer::Pattern pattern = activeBuzzerPattern;
  if (pattern == buzzer::Pattern::None && controller.state() == microwave::State::Running) {
    pattern = buzzer::Pattern::Hum;
  }

  buzzer::ToneState toneState = buzzer::toneStateFor(pattern, elapsedMs);
  if (toneState.on) {
    tone(PIN_BUZZER, toneState.frequencyHz);
  } else {
    noTone(PIN_BUZZER);
  }
}

void updateOutputs() {
  bool running = controller.state() == microwave::State::Running;
  digitalWrite(PIN_MOTOR, running ? HIGH : LOW);
  digitalWrite(PIN_FAN, running ? HIGH : LOW);
  digitalWrite(PIN_LIGHT, running ? HIGH : LOW);
}

// Renders the countdown (MM:SS) to the TM1637, blinking the colon while Setting/Done so the
// panel doesn't sit static in those states.
void updateDisplay() {
  sevenseg::Digits d = sevenseg::secondsToDigits(controller.displaySeconds());

  bool colonOn = true;
  if (controller.state() == microwave::State::Setting ||
      controller.state() == microwave::State::Done) {
    colonOn = sevenseg::blinkOn(static_cast<uint16_t>(millis() % 60000), BLINK_PERIOD_MS);
  }

  uint8_t segments[4] = {
      display.encodeDigit(d.minutesTens),
      display.encodeDigit(d.minutesOnes),
      display.encodeDigit(d.secondsTens),
      display.encodeDigit(d.secondsOnes),
  };
  if (colonOn) segments[COLON_DIGIT_INDEX] |= COLON_BIT;
  display.setSegments(segments);
}

}  // namespace

void setup() {
  for (uint8_t r = 0; r < keymatrix::ROWS; ++r) {
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], HIGH);
  }
  for (uint8_t c = 0; c < keymatrix::COLS; ++c) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);

  display.setBrightness(DISPLAY_BRIGHTNESS);
}

void loop() {
  char key = keyScanner.scan(scanRawKeyIndex());
  microwave::Event event;
  if (key != '\0' && translateKey(key, event)) {
    controller.handle(event);
    startBuzzerPattern(buzzer::Pattern::KeyPress);
  }

  if (millis() - lastTickMs >= 1000) {
    lastTickMs = millis();
    controller.handle(microwave::Event{microwave::EventType::Tick, 0});
  }

  if (previousState != microwave::State::Done && controller.state() == microwave::State::Done) {
    startBuzzerPattern(buzzer::Pattern::Done);
  }
  previousState = controller.state();

  updateBuzzer();
  updateOutputs();
  updateDisplay();
}
