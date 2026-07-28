#pragma once

#include <stdint.h>

// A UTC-seconds clock with no onboard RTC: starts unsynced, and is set from an external trusted
// source rather than ticked internally -- deliberately unaware of *how* a sync value arrives
// (main.cpp is what feeds it a value read from the radio). Between syncs, the current estimate
// is extrapolated from millis() elapsed since the last sync, corrected for the local clock's
// measured drift rate (see sync()) -- the same clock-discipline idea NTP uses (measure drift
// between reference points, correct future extrapolation), applied in software here since there's
// no oscillator-steering register on this MCU the way e.g. ESP32's calibration API has. Hardware-
// free -- takes millis() as a parameter rather than calling it directly -- so it unit-tests
// off-device.
//
// There is no RTC chip in the BOM, so this resets to unsynced (and forgets any learned drift
// correction) on every power loss; a battery-backed RTC (e.g. DS3231) would be needed to keep
// real time and calibration across a reset.
namespace epoch {

class Clock {
 public:
  bool isSynced() const { return synced_; }

  // Current UTC estimate in seconds, extrapolated from the last sync plus elapsed millis(),
  // scaled by the measured drift correction. Returns 0 if never synced.
  uint32_t current(uint32_t nowMs) const {
    if (!synced_) return 0;
    uint32_t elapsedMs = nowMs - lastSyncMs_;
    return lastSyncUtc_ + static_cast<uint32_t>(elapsedMs * driftCorrection_ / 1000);
  }

  // Anchor to a fresh, trusted UTC value. If a previous sync exists and enough time has passed
  // since it to measure reliably, also updates the drift correction from how far off the local
  // clock's elapsed-millis count was against this new reference -- e.g. if 60000ms locally
  // elapsed but the reference says 61s actually passed, the local clock is running slow and
  // future extrapolation should scale up to compensate.
  void sync(uint32_t utcSeconds, uint32_t nowMs) {
    if (synced_) {
      uint32_t elapsedMs = nowMs - lastSyncMs_;
      // Below MIN_CALIBRATION_INTERVAL_MS, whole-second UTC resolution is too coarse to measure
      // drift reliably (e.g. a 5s gap with +/-0.5s of rounding is a 10% error) -- skip updating
      // the correction rather than let a noisy short interval corrupt it.
      if (elapsedMs >= MIN_CALIBRATION_INTERVAL_MS) {
        float measured =
            (static_cast<float>(utcSeconds - lastSyncUtc_) * 1000.0f) / elapsedMs;
        // Reject an implausible measured ratio (e.g. from an accepted-but-still-off sync, or
        // this being the first real gap after a long unsynced stretch) rather than let one
        // outlier wreck all future extrapolation -- real crystal drift is nowhere near this wide.
        if (measured >= MIN_CORRECTION && measured <= MAX_CORRECTION) driftCorrection_ = measured;
      }
    }
    lastSyncUtc_ = utcSeconds;
    lastSyncMs_ = nowMs;
    synced_ = true;
  }

 private:
  static constexpr uint32_t MIN_CALIBRATION_INTERVAL_MS = 60000;
  static constexpr float MIN_CORRECTION = 0.95f;
  static constexpr float MAX_CORRECTION = 1.05f;

  uint32_t lastSyncUtc_ = 0;
  uint32_t lastSyncMs_ = 0;
  float driftCorrection_ = 1.0f;
  bool synced_ = false;
};

// A received UTC value is plausible if it falls within toleranceSeconds of this clock's own
// current estimate -- catches a garbled/corrupted radio packet before accepting it as the new
// sync. An unsynced clock has no basis for comparison, so anything is accepted as the first sync.
inline bool isPlausible(const Clock& clock, uint32_t candidateUtc, uint32_t nowMs,
                         uint32_t toleranceSeconds) {
  if (!clock.isSynced()) return true;
  uint32_t estimate = clock.current(nowMs);
  uint32_t diff = candidateUtc > estimate ? candidateUtc - estimate : estimate - candidateUtc;
  return diff <= toleranceSeconds;
}

}  // namespace epoch
