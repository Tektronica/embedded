#pragma once

#include <stdint.h>

// Wire format transmitted to the topside station over the nRF24L01 link, modeled on the fields a
// real Argo profiling float reports at each surfacing: a platform identifier (Argo calls this
// the WMO number -- a unique ID assigned to each unit), the cycle number (which numbered dive
// cycle this is, since deployment), and the current UTC estimate (see EpochClock.h -- there's no
// RTC, so this is only meaningful once synced). Packed and ordered largest-first so the layout
// is unambiguous regardless of which compiler/architecture the topside side is built with.
namespace payload {

constexpr uint16_t PLATFORM_ID = 0;  // TODO: set to this unit's assigned identifier

struct __attribute__((packed)) Telemetry {
  uint32_t utcSeconds;
  uint16_t platformId;
  uint16_t cycleNumber;
};

}  // namespace payload
