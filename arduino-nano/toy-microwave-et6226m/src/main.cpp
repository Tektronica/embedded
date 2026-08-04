#include <Arduino.h>

#include "Buzzer.h"
#include "ET6226M.h"
#include "KeyDebounce.h"
#include "Microwave.h"
#include "SevenSegment.h"

namespace {

constexpr uint8_t PIN_ET6226M_CLK = 2;
constexpr uint8_t PIN_ET6226M_DAT = 3;

constexpr uint8_t PIN_BUZZER = 10;
constexpr uint8_t PIN_MOTOR  = 11;
constexpr uint8_t PIN_FAN    = 12;
constexpr uint8_t PIN_LIGHT  = 13;

constexpr uint16_t BLINK_PERIOD_MS = 1000;        // colon/display blink cycle while ClockSet/Done
constexpr uint32_t DONE_REMINDER_INTERVAL_MS = 10000;  // re-beep this often if Done goes unacknowledged
// The schematic ties DP/KP directly to the display's shared DP line (see KiCad/microwave), so
// this driver runs in EightSegment mode -- DP is a real, per-grid-multiplexed segment here, not
// freed for a keyboard-scan role. COLON_DIGIT_INDEX picks which grid's time slot also asserts DP,
// the same trick the old TM1637 approach used for its colon.
constexpr uint8_t COLON_DIGIT_INDEX = 1;
constexpr uint8_t COLON_BIT = 0x80;

// Segment bytes spelling "End" (bit0..6 = a..g, the same convention et6226m::encodeDigit() uses)
// for the last 3 grids, shown once Done is reached instead of a static/flashing "0:00" -- this
// app's own message, not something the generic digit codec should know a letter alphabet for.
constexpr uint8_t SEGMENTS_E = 0x79;
constexpr uint8_t SEGMENTS_N = 0x54;
constexpr uint8_t SEGMENTS_D = 0x5E;

ET6226M display(PIN_ET6226M_CLK, PIN_ET6226M_DAT, et6226m::SegmentMode::EightSegment);
keydebounce::Debouncer  keyDebouncer;
microwave::Controller   controller;
microwave::State        previousState = microwave::State::Idle;

buzzer::Pattern activeBuzzerPattern  = buzzer::Pattern::None;
uint32_t        buzzerPatternStartMs = 0;
uint32_t        lastTickMs           = 0;
uint32_t        doneReminderMs       = 0;  // last time the Done reminder beeped (or Done began)

// Map a debounced (grid, segment) reading to a microwave::Event. Confirmed against
// KiCad/microwave's schematic: the 16-key matrix is SG1-4 (rows) x GR1-4 (columns), one diode per
// key, reusing the display's own segment/grid lines (see the netlist's SEG_x/ROWn and
// DIG_CAn/COLn nets). LAYOUT keeps the old 4x4 keypad's visual arrangement; which legend ends up
// printed on which physical keycap is a separate decision that doesn't affect this electrical
// wiring -- if it ever needs to change, it's a firmware-only fix, not a rewire.
bool translateKey(et6226m::KeyPosition key, microwave::Event& event) {
  static constexpr char LAYOUT[4][4] = {
      {'1', '2', '3', 'A'},
      {'4', '5', '6', 'B'},
      {'7', '8', '9', 'C'},
      {'*', '0', '#', 'D'},
  };
  if (key.grid < 1 || key.grid > 4 || key.segment < 1 || key.segment > 4) return false;
  char ch = LAYOUT[key.segment - 1][key.grid - 1];  // SG = row, GR = column

  if (ch >= '0' && ch <= '9') {
    event = microwave::Event{microwave::EventType::Digit, static_cast<uint8_t>(ch - '0')};
    return true;
  }
  if (ch == '#') {
    event = microwave::Event{microwave::EventType::Start, 0};
    return true;
  }
  if (ch == '*') {
    event = microwave::Event{microwave::EventType::Cancel, 0};
    return true;
  }
  if (ch == 'A') {
    event = microwave::Event{microwave::EventType::Clock, 0};
    return true;
  }
  if (ch == 'B') {
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

// Renders the current value (MM:SS for the cook timer, HH:MM for the clock) to the ET6226M,
// including the colon (DP, asserted during COLON_DIGIT_INDEX's grid slot) -- except while Done,
// which shows "End" instead of a static/flashing "0:00", with no colon (it's a word, not a time).
// The colon stays solid in every other state. Everything shown blanks instead in two cases: on
// the blink's off phase throughout ClockSet, since that's the one state where you're actively
// changing the clock rather than just reading it or entering a countdown; and, while Done, only
// during the moments the buzzer is actually sounding a done/reminder beep, in exact sync with
// buzzer::toneStateFor's on/off phases -- so the panel flashes with the beeper rather than on its
// own separate timer, and stays steady in between reminders.
void updateDisplay() {
  bool isDone = controller.state() == microwave::State::Done;

  bool blinkPhaseOn = sevenseg::blinkOn(static_cast<uint16_t>(millis() % 60000), BLINK_PERIOD_MS);
  bool blankForClockSet = controller.state() == microwave::State::ClockSet && !blinkPhaseOn;

  bool blankForDoneBeep = false;
  if (isDone && activeBuzzerPattern == buzzer::Pattern::Done) {
    uint32_t beepElapsedMs = millis() - buzzerPatternStartMs;
    blankForDoneBeep = !buzzer::toneStateFor(buzzer::Pattern::Done, beepElapsedMs).on;
  }

  bool blank = blankForClockSet || blankForDoneBeep;
  uint8_t segments[ET6226M::GRID_COUNT];
  if (isDone) {
    segments[0] = 0;
    segments[1] = blank ? uint8_t{0} : SEGMENTS_E;
    segments[2] = blank ? uint8_t{0} : SEGMENTS_N;
    segments[3] = blank ? uint8_t{0} : SEGMENTS_D;
  } else {
    sevenseg::Digits d = sevenseg::secondsToDigits(controller.displayValue());
    segments[0] = blank ? uint8_t{0} : et6226m::encodeDigit(d.minutesTens);
    segments[1] = blank ? uint8_t{0} : et6226m::encodeDigit(d.minutesOnes);
    segments[2] = blank ? uint8_t{0} : et6226m::encodeDigit(d.secondsTens);
    segments[3] = blank ? uint8_t{0} : et6226m::encodeDigit(d.secondsOnes);
    if (!blank) segments[COLON_DIGIT_INDEX] |= COLON_BIT;
  }
  display.setSegments(segments);
}

}  // namespace

void setup() {
  display.begin();

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LIGHT, OUTPUT);
}

void loop() {
  et6226m::KeyPosition rawKey = et6226m::decodeKeyCode(display.readKeyCode());
  et6226m::KeyPosition key = keyDebouncer.scan(rawKey);
  microwave::Event event;
  if (key.grid != 0 && translateKey(key, event)) {
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
