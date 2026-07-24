#include <Arduino.h>

#include "Ballast.h"
#include "Cycle.h"
#include "EpochClock.h"
#include "Payload.h"
#include "Radio.h"
#include "Stepper.h"

namespace {

// Ballast (DRV8825 stepper driver) -- syringe ballast, see README for the mechanism and BOM.
constexpr uint8_t PIN_DIRECTION = 2;
constexpr uint8_t PIN_STEP = 3;
constexpr uint8_t PIN_MS3 = 4;
constexpr uint8_t PIN_MS2 = 5;
constexpr uint8_t PIN_MS1 = 6;
constexpr uint8_t PIN_ENABLE = 7;

constexpr uint8_t PIN_HEARTBEAT = 8;

// Radio (nRF24L01+). CE/CSN must avoid the hardware SPI pins (D11 MOSI, D12 MISO, D13 SCK) --
// see fw/README's ADC/pin-conflict note; the same "don't collide with a reserved bus" rule
// applies to SPI, not just the ADC mux.
constexpr uint8_t PIN_RADIO_CE = 10;
constexpr uint8_t PIN_RADIO_CSN = 9;

constexpr uint32_t HEARTBEAT_PERIOD_MS = 1000;
constexpr uint32_t RX_TIMEOUT_MS = 5000;  // max wait for a topside reply
constexpr uint32_t SURFACE_MIN_DURATION_MS = 2000;  // floor on total Surface time, even if a reply arrives instantly
constexpr uint32_t PARK_DURATION_MS = 5000;  // placeholder -- tune once real sensor/mission timing is defined
constexpr uint32_t UTC_TOLERANCE_S = 60;
constexpr uint8_t MAX_RETRIES = 5;
constexpr float STEPPER_MAX_SPEED = 3000;
constexpr float STEPPER_ACCELERATION = 500;

// Serial trace of dive-cycle phase transitions, off by default -- flip to true to debug (e.g. in
// the Wokwi simulator, where the radio can't be simulated and the motor's direction is otherwise
// the only observable sign of which phase is active).
constexpr bool DEBUG_TRACE_ENABLED = false;

// This unit is always the float side of the link; the topside station uses the swapped
// writing/reading pipe configuration.
constexpr uint64_t PIPE_FLOAT_TO_TOPSIDE = 0xE8E8F0F0E1LL;
constexpr uint64_t PIPE_TOPSIDE_TO_FLOAT = 0xE8E8F0F0E2LL;

stepper::Driver ballastMotor(PIN_STEP, PIN_DIRECTION);
radio::Link link(PIN_RADIO_CE, PIN_RADIO_CSN);

cycle::Phase phase = cycle::Phase::Surface;
epoch::Clock clock;
uint16_t cycleNumber = 0;

bool heartbeatOn = false;
uint32_t lastHeartbeatMs = 0;

// Draws water in to dive. Returns true while still moving. Direction convention (clockwise =
// fill, counterclockwise = drain) is fixed by the physical wiring -- if it runs backwards once
// wired up, swap the stepper's direction-pin polarity, not this call.
bool fillBallast() { return ballastMotor.driveTo(ballast::strokeSteps()); }

// Expels water to surface. Returns true while still moving.
bool drainBallast() { return ballastMotor.driveTo(0); }

void updateHeartbeat() {
  if (millis() - lastHeartbeatMs >= HEARTBEAT_PERIOD_MS) {
    lastHeartbeatMs = millis();
    heartbeatOn = !heartbeatOn;
    digitalWrite(PIN_HEARTBEAT, heartbeatOn ? HIGH : LOW);
  }
}

// Transmits platform ID + cycle number + current UTC estimate, then waits briefly for the
// topside station to return a corrected UTC value. A plausible correction (see EpochClock.h)
// updates the clock.
void surface() {
  uint32_t surfaceStartMs = millis();
  link.wake();

  payload::Telemetry outgoing{clock.current(millis()), payload::PLATFORM_ID, cycleNumber};
  link.transmit(outgoing);

  uint32_t receivedUtc = 0;
  if (link.receive(receivedUtc, RX_TIMEOUT_MS) &&
      epoch::isPlausible(clock, receivedUtc, millis(), UTC_TOLERANCE_S)) {
    clock.sync(receivedUtc, millis());
  }

  link.sleep();

  // Top up to SURFACE_MIN_DURATION_MS if the exchange above finished early (e.g. an instant
  // reply, or no chip connected at all -- see Radio.h).
  uint32_t elapsedMs = millis() - surfaceStartMs;
  if (elapsedMs < SURFACE_MIN_DURATION_MS) delay(SURFACE_MIN_DURATION_MS - elapsedMs);

  phase = cycle::next(phase);
}

// Lingers at depth for PARK_DURATION_MS before ascending.
// TODO: no sensor hardware in the BOM yet -- this only waits out a timer, doesn't collect
// anything; wire real data collection into this dwell once sensor hardware exists.
void park() {
  static uint32_t parkStartMs = 0;
  static bool parking = false;

  if (!parking) {
    parkStartMs = millis();
    parking = true;
  }
  if (millis() - parkStartMs >= PARK_DURATION_MS) {
    parking = false;
    phase = cycle::next(phase);
  }
}

const char* phaseName(cycle::Phase p) {
  switch (p) {
    case cycle::Phase::Surface: return "SURFACE";
    case cycle::Phase::Descent: return "DESCENT";
    case cycle::Phase::Park:    return "PARK";
    case cycle::Phase::Ascent:  return "ASCENT";
  }
  return "UNKNOWN";
}

// Logs each phase transition once (not every loop iteration). The sentinel `loggedOnce` makes
// sure the very first phase gets logged too, not just later changes.
void logPhaseTrace(cycle::Phase current) {
  static cycle::Phase lastLogged;
  static bool loggedOnce = false;
  if (loggedOnce && current == lastLogged) return;
  loggedOnce = true;
  lastLogged = current;
  Serial.println(phaseName(current));
}

}  // namespace

void setup() {
  if (DEBUG_TRACE_ENABLED) Serial.begin(9600);

  pinMode(PIN_HEARTBEAT, OUTPUT);

  ballastMotor.begin(PIN_ENABLE, PIN_MS1, PIN_MS2, PIN_MS3, STEPPER_MAX_SPEED,
                     STEPPER_ACCELERATION);

  link.begin(PIPE_FLOAT_TO_TOPSIDE, PIPE_TOPSIDE_TO_FLOAT, MAX_RETRIES);
}

void loop() {
  updateHeartbeat();
  if (DEBUG_TRACE_ENABLED) logPhaseTrace(phase);

  switch (phase) {
    case cycle::Phase::Surface:
      surface();
      break;

    case cycle::Phase::Descent:
      if (!fillBallast()) {
        phase = cycle::next(phase);
        delay(500);
      }
      break;

    case cycle::Phase::Park:
      park();
      break;

    case cycle::Phase::Ascent:
      if (!drainBallast()) {
        ++cycleNumber;
        phase = cycle::next(phase);
        delay(500);
      }
      break;
  }
}
