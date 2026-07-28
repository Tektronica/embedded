#include <Arduino.h>
#include <TM1637Display.h>

#include "Buzzer.h"
#include "KeyMatrix.h"
#include "MatrixScanner.h"
#include "Microwave.h"
#include "SevenSegment.h"

namespace {

// Matrix rows/cols — a 4x4 matrix keypad (see README's BOM; tested against Wokwi's matrix pad).
constexpr uint8_t ROW_PINS[keymatrix::ROWS] = {9, 8, 7, 6};
constexpr uint8_t COL_PINS[keymatrix::COLS] = {5, 4, 3, 2};

constexpr uint8_t PIN_BUZZER = 10;
constexpr uint8_t PIN_MOTOR  = 11;
constexpr uint8_t PIN_FAN    = 12;
constexpr uint8_t PIN_LIGHT  = 13;

constexpr uint8_t PIN_DISPLAY_CLK = A0;
constexpr uint8_t PIN_DISPLAY_DIO = A1;
constexpr uint8_t DISPLAY_BRIGHTNESS = 7;         // 0..7, this library's max
constexpr uint16_t BLINK_PERIOD_MS = 1000;        // colon/display blink cycle while Setting/Done
constexpr uint32_t DONE_REMINDER_INTERVAL_MS = 10000;  // re-beep this often if Done goes unacknowledged
// Which digit's decimal-point segment drives the physical colon varies by board. 0x80 on digit
// index 1 matches most common 4-digit TM1637 clock boards, but confirm against this one once
// it's wired up and adjust COLON_DIGIT_INDEX/COLON_BIT if the colon doesn't light.
constexpr uint8_t COLON_DIGIT_INDEX = 1;
constexpr uint8_t COLON_BIT = 0x80;

matrixscanner::Scanner    matrixScanner(ROW_PINS, COL_PINS);
keymatrix::Scanner        keyScanner;
microwave::Controller     controller;
microwave::State          previousState = microwave::State::Idle;
TM1637Display             display(PIN_DISPLAY_CLK, PIN_DISPLAY_DIO);

buzzer::Pattern activeBuzzerPattern  = buzzer::Pattern::None;
uint32_t        buzzerPatternStartMs = 0;
uint32_t        lastTickMs           = 0;
uint32_t        doneReminderMs       = 0;  // last time the Done reminder beeped (or Done began)

// Map a settled key character to a microwave::Event. Returns false for keys this project
// doesn't use (C-D) — the keypad is a standard 16-key layout; see README for the full mapping.
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
  if (key == 'A') {
    event = microwave::Event{microwave::EventType::Clock, 0};
    return true;
  }
  if (key == 'B') {
    event = microwave::Event{microwave::EventType::Timer, 0};
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
  if (pattern == buzzer::Pattern::None && controller.isCooking()) {
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
  bool cooking = controller.isCooking();
  digitalWrite(PIN_MOTOR, cooking ? HIGH : LOW);
  digitalWrite(PIN_FAN, cooking ? HIGH : LOW);
  digitalWrite(PIN_LIGHT, cooking ? HIGH : LOW);
}

// Renders the current value (MM:SS for the cook timer, HH:MM for the clock) to the TM1637.
// The colon blinks in every state except Running, where it stays solid — Idle's blink doubles
// as a live "the clock is still running" heartbeat, and Setting/Done reuse the same blink to
// show they're not the live countdown/clock. ClockSet blinks the whole display instead of just
// the colon, since it's the one state where you're actively changing the clock, not just reading
// it or entering a countdown — the display should read as "editing" at a glance.
void updateDisplay() {
  sevenseg::Digits d = sevenseg::secondsToDigits(controller.displayValue());

  bool blinkPhaseOn = sevenseg::blinkOn(static_cast<uint16_t>(millis() % 60000), BLINK_PERIOD_MS);
  bool colonOn = controller.state() == microwave::State::Running ? true : blinkPhaseOn;

  uint8_t segments[4];
  if (controller.state() == microwave::State::ClockSet && !blinkPhaseOn) {
    segments[0] = segments[1] = segments[2] = segments[3] = 0;  // blank on the blink's off phase
  } else {
    segments[0] = display.encodeDigit(d.minutesTens);
    segments[1] = display.encodeDigit(d.minutesOnes);
    segments[2] = display.encodeDigit(d.secondsTens);
    segments[3] = display.encodeDigit(d.secondsOnes);
    if (colonOn) segments[COLON_DIGIT_INDEX] |= COLON_BIT;
  }
  display.setSegments(segments);
}

}  // namespace

void setup() {
  matrixScanner.begin();

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);

  display.setBrightness(DISPLAY_BRIGHTNESS);
}

void loop() {
  char key = keyScanner.scan(matrixScanner.scan());
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
    doneReminderMs = millis();
  } else if (controller.state() == microwave::State::Done &&
             millis() - doneReminderMs >= DONE_REMINDER_INTERVAL_MS) {
    // Done hasn't been acknowledged (no Cancel/Start/Digit yet) -- re-beep so it isn't silently
    // ignored, same as a real microwave nagging until the door opens or a button is pressed.
    startBuzzerPattern(buzzer::Pattern::Done);
    doneReminderMs = millis();
  }
  previousState = controller.state();

  updateBuzzer();
  updateOutputs();
  updateDisplay();
}
