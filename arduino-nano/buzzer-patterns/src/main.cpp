#include <Arduino.h>

#include "Button.h"
#include "Buzzer.h"

namespace {

constexpr uint8_t PIN_BUZZER       = 9;
constexpr uint8_t PIN_BTN_KEYPRESS = 2;  // momentary pushbuttons, wired to GND (INPUT_PULLUP)
constexpr uint8_t PIN_BTN_DONE     = 3;
constexpr uint8_t PIN_BTN_ERROR    = 4;

// Serial/Teleplot trace of the active pattern, off by default -- flip to true to debug.
constexpr bool     DEBUG_TRACE_ENABLED     = false;
constexpr uint32_t DEBUG_TRACE_INTERVAL_MS = 200;

input::Button keyPressButton;
input::Button doneButton;
input::Button errorButton;

buzzer::Pattern activePattern  = buzzer::Pattern::None;
uint32_t        patternStartMs = 0;

void startPattern(buzzer::Pattern pattern) {
  activePattern  = pattern;
  patternStartMs = millis();
}

const char* patternName(buzzer::Pattern p) {
  switch (p) {
    case buzzer::Pattern::KeyPress: return "KeyPress";
    case buzzer::Pattern::Done:     return "Done";
    case buzzer::Pattern::Error:    return "Error";
    case buzzer::Pattern::None:
    default:                        return "None";
  }
}

void logDebugTrace(buzzer::ToneState toneState) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < DEBUG_TRACE_INTERVAL_MS) return;
  lastPrintMs = millis();
  Serial.print(">pattern:"); Serial.println(patternName(activePattern));
  Serial.print(">on:");      Serial.println(toneState.on);
  Serial.print(">hz:");      Serial.println(toneState.frequencyHz);
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_KEYPRESS, INPUT_PULLUP);
  pinMode(PIN_BTN_DONE, INPUT_PULLUP);
  pinMode(PIN_BTN_ERROR, INPUT_PULLUP);
}

void loop() {
  if (keyPressButton.pressed(digitalRead(PIN_BTN_KEYPRESS) == LOW)) {
    startPattern(buzzer::Pattern::KeyPress);
  }
  if (doneButton.pressed(digitalRead(PIN_BTN_DONE) == LOW)) {
    startPattern(buzzer::Pattern::Done);
  }
  if (errorButton.pressed(digitalRead(PIN_BTN_ERROR) == LOW)) {
    startPattern(buzzer::Pattern::Error);
  }

  uint32_t elapsedMs = millis() - patternStartMs;
  if (buzzer::isFinished(activePattern, elapsedMs)) activePattern = buzzer::Pattern::None;

  buzzer::ToneState toneState = buzzer::toneStateFor(activePattern, elapsedMs);
  if (toneState.on) {
    tone(PIN_BUZZER, toneState.frequencyHz);
  } else {
    noTone(PIN_BUZZER);
  }

  if (DEBUG_TRACE_ENABLED) logDebugTrace(toneState);
}
