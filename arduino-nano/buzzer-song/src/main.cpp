#include <Arduino.h>

#include "Button.h"
#include "Song.h"
#include "SongPlayer.h"

namespace {

constexpr uint8_t PIN_BUZZER = 9;
constexpr uint8_t PIN_BUTTON = 2;  // momentary pushbutton, wired to GND (INPUT_PULLUP)

// Serial/Teleplot trace of the active song, off by default -- flip to true to debug.
constexpr bool     DEBUG_TRACE_ENABLED     = false;
constexpr uint32_t DEBUG_TRACE_INTERVAL_MS = 200;

input::Button button;
song::Track   currentTrack = song::Track::Scale;
uint32_t      trackStartMs = 0;

const char* trackName(song::Track t) {
  switch (t) {
    case song::Track::Tetris: return "Tetris";
    case song::Track::Mario:  return "Mario";
    case song::Track::Doom:   return "Doom";
    case song::Track::Nokia:  return "Nokia";
    case song::Track::Scale:
    default:                  return "Scale";
  }
}

void logDebugTrace(song::ToneState toneState) {
  static uint32_t lastPrintMs = 0;
  if (millis() - lastPrintMs < DEBUG_TRACE_INTERVAL_MS) return;
  lastPrintMs = millis();
  Serial.print(">track:"); Serial.println(trackName(currentTrack));
  Serial.print(">on:");    Serial.println(toneState.on);
  Serial.print(">hz:");    Serial.println(toneState.frequencyHz);
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (button.pressed(digitalRead(PIN_BUTTON) == LOW)) {
    currentTrack = song::next(currentTrack);
    trackStartMs = millis();
  }

  song::ToneState toneState = songplayer::update(currentTrack, millis() - trackStartMs);
  if (toneState.on) {
    tone(PIN_BUZZER, toneState.frequencyHz);
  } else {
    noTone(PIN_BUZZER);
  }

  if (DEBUG_TRACE_ENABLED) logDebugTrace(toneState);
}
