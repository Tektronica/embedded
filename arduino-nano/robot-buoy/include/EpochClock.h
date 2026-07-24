#pragma once

#include <stdint.h>

// A UTC-seconds clock with no onboard RTC: starts unsynced, and is set from an external trusted
// source rather than ticked internally -- deliberately unaware of *how* a sync value arrives
// (main.cpp is what feeds it a value read from the radio). Between syncs, the current estimate
// is extrapolated from millis() elapsed since the last sync. Hardware-free -- takes millis() as
// a parameter rather than calling it directly -- so it unit-tests off-device.
//
// There is no RTC chip in the BOM, so this resets to unsynced on every power loss; a
// battery-backed RTC (e.g. DS3231) would be needed to keep real time across a reset.
namespace epoch {

class Clock {
 public:
  bool isSynced() const { return synced_; }

  // Current UTC estimate in seconds, extrapolated from the last sync plus elapsed millis().
  // Returns 0 if never synced.
  uint32_t current(uint32_t nowMs) const {
    if (!synced_) return 0;
    return lastSyncUtc_ + (nowMs - lastSyncMs_) / 1000;
  }

  void sync(uint32_t utcSeconds, uint32_t nowMs) {
    lastSyncUtc_ = utcSeconds;
    lastSyncMs_ = nowMs;
    synced_ = true;
  }

 private:
  uint32_t lastSyncUtc_ = 0;
  uint32_t lastSyncMs_ = 0;
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
