# arduino-nano/robot-buoy

Arduino Nano firmware for a vertical-profiling float: dive via a syringe ballast, park at depth to
collect data, resurface, and transmit telemetry (platform ID, cycle number, current UTC estimate)
to a topside station over an nRF24L01+ radio, then repeat. Cycle-phase naming (Surface / Descent /
Park / Ascent) and payload fields (platform ID, cycle number) follow standard Argo profiling-float
conventions rather than application-specific terms.

Ported from a standalone Uno sketch originally built for the MATE ROV "RANGER" profiling-float
mission task (`profiling-float/src/buoy`) — rewritten to this repo's conventions, rescoped to the
Nano, and audited for hardware conflicts along the way (see below).

## Dive cycle

- **Surface** — power up the radio, transmit `{platform ID, cycle number, current UTC estimate}`
  (see [Payload.h](include/Payload.h)), wait up to `RX_TIMEOUT_MS` (5 s) for the topside station to
  return a corrected UTC value, and adopt it if it's plausible (see
  [EpochClock.h](include/EpochClock.h)). Then descend.
- **Descent** — the ballast syringe draws water in (added mass, less buoyant) until the commanded
  stroke completes. Then park.
- **Park** — lingers at depth for `PARK_DURATION_MS` (5 s). Placeholder dwell -- no sensor hardware
  is in the BOM yet, so nothing is actually collected during it. Then ascend.
- **Ascent** — the syringe expels the water back out (less mass, more buoyant) until back at the
  starting position; the cycle number increments. Then surface, and repeat.

### Per-cycle configuration

| Aspect | Constant | Notes |
|---|---|---|
| "Depth" | `ballast::FILL_REVOLUTIONS` (`Ballast.h`) | A stroke length in motor revolutions, not a real depth -- there's no pressure/depth sensor to close the loop (open-loop; see below), and it's all-or-nothing (full fill or full drain), not a continuously variable target. Placeholder; calibrate against real syringe travel. |
| Park duration | `PARK_DURATION_MS` (`main.cpp`) | Fixed dwell at depth; placeholder until real sensor/mission timing requirements exist. |
| Surface duration | `RX_TIMEOUT_MS` + `SURFACE_MIN_DURATION_MS` (`main.cpp`) | `RX_TIMEOUT_MS` is the *max* wait for a topside reply; `SURFACE_MIN_DURATION_MS` is a floor on total Surface time even if a reply arrives instantly (or the radio isn't connected at all). Actual time at surface is `max(time until reply or timeout, SURFACE_MIN_DURATION_MS)`. |

**Future: closed-loop depth.** Right now depth is entirely open-loop -- the ballast runs a fixed
stroke and the firmware just trusts it worked. A pressure/depth sensor would let `Descent`/`Ascent`
verify actual pressure is changing as commanded (matching how real Argo floats detect faults: a
commanded dive with no pressure change means a stuck syringe, trapped air, or other mechanical
failure, not a successful descent). Not built -- no sensor in the BOM yet -- but the existing split
(`Ballast.h` pure math / `Stepper.h` generic driver / `main.cpp` wiring) means a future `Pressure.h`
could slot into the dive-cycle phases as a verification check without restructuring what's here.

## Buoyancy engine: syringe ballast

A stepper-driven syringe is the buoyancy engine: drawing water in increases the float's mass
without significantly changing its displaced volume (less buoyant → dives); expelling it reverses
that (more buoyant → surfaces). Direction convention (clockwise = fill/dive, counterclockwise =
drain/surface) is fixed by the physical wiring — if it runs backwards once wired up, swap the
stepper's direction-pin polarity rather than the firmware's calls.

`ballast::FILL_REVOLUTIONS` (default 5) is a placeholder — calibrate it against the real syringe's
travel once the mechanism is built. The stepper driver is a DRV8825 (see the source project's
`hw/motor-controller/Stepper/README.md`, which compares it against MP6500 and recommends DRV8825
for 1/32 microstepping and higher current headroom); `AccelStepper`'s `DRIVER` interface is
generic STEP/DIR and works identically with A4988/DRV8825/TB6600 — only the MS1/MS2/MS3
microstepping truth table is driver-specific, and the all-high → 1/32 setting here is correct
specifically for the DRV8825.

## BOM (from the original design)

| Role | Part |
|---|---|
| MCU | Arduino Nano (ATmega328) |
| Stepper driver | DRV8825 |
| Stepper motor | Bipolar stepper (syringe ballast actuator) |
| Radio | nRF24L01+ |

## Pin table

| Function | Pin | Note |
|---|---|---|
| Ballast direction | D2 | DRV8825 `DIR` |
| Ballast step | D3 | DRV8825 `STEP` |
| Ballast microstep select (MS3/MS2/MS1) | D4 / D5 / D6 | all high = 1/32 microstepping |
| Ballast enable | D7 | DRV8825 `~ENABLE`, active-low |
| Heartbeat LED | D8 | plain polled blink, no watchdog/ISR (see below) |
| Radio CSN | D9 | moved off D11 — see audit below |
| Radio CE | D10 | |
| SPI (MOSI/MISO/SCK) | D11 / D12 / D13 | hardware bus — reserved, nothing else may use these |

## Audit: hardware conflicts fixed during the port

The original sketch had two pin conflicts that would have corrupted radio communication:

1. **Radio `CSN` was wired to D11**, which is the ATmega328's hardware SPI `MOSI` line. Asserting
   chip-select on the same pin the radio's own SPI data line uses would directly interfere with
   both signals. Moved to D9.
2. **The heartbeat LED was wired to D13**, the hardware SPI `SCK` line, and toggled from a
   watchdog-timer interrupt. Any ISR firing mid-transfer would glitch the SPI clock and corrupt
   whatever the radio was doing at that moment. Moved to D8, and the watchdog/ISR was removed
   entirely in favor of a plain `millis()`-polled blink in `loop()` (this repo's standard pattern —
   see `toy-microwave`) — the watchdog was only ever used as an interrupt source for the blink, not
   for an actual reset/recovery function, so nothing is lost by removing it. It also removes the
   asynchronous-write class of bug outright, since a polled blink can never fire mid-transfer.

**Found via Wokwi simulation, but a real hardware risk, not a simulator quirk:** `RF24::write()`
blocks waiting for the chip's own `TX_DS`/`MAX_RT` status bits, set by real radio hardware. With no
chip responding at all — the exact situation in Wokwi, since the nRF24L01+ isn't simulated, but
also a real failure mode if the physical radio ever disconnects — those bits never arrive and
`write()` can hang forever, freezing the entire dive cycle on whatever called it (in this case, the
very first `Surface` phase at boot, before `loop()` ever gets to run the heartbeat or ballast
again). Fixed in [Radio.h](include/Radio.h): both `transmit()` and `receive()` check
`RF24::isChipConnected()` first and skip the radio operation entirely if it's not there, so a
missing or failed radio degrades the float's telemetry rather than hanging the whole vehicle.

Also cleaned up: a duplicated `Serial.begin()` call, a `while (!Serial) {}` wait that's a no-op on
the Nano's UART-bridge serial (that pattern is for native-USB boards), a call to
`radio.setAutoRetransmit()` that doesn't exist in the resolved `RF24` library version (now
`setRetries()`, with corrected argument order), and a UTC "validation" that compared against
`time(nullptr)` — meaningless without an RTC. See [EpochClock.h](include/EpochClock.h) for the
replacement: validate a received UTC value against the float's own last-known estimate instead,
which actually has a real basis for comparison. The wire payload also gained
`__attribute__((packed))` and largest-first field ordering, since it's now a multi-field struct
rather than a bare scalar — an unspecified layout would silently disagree with whatever compiler
builds the topside side.

**Known gap, not implemented:** the original design docs (`documentation/pseudo/main_loop.png`)
planned a `default: beacon(); // ping home` fail-safe for an unrecognized cycle phase. It was never
built in the original sketch and isn't ported here either — noted so it isn't silently lost.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/robot-buoy
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

## Structure

- **`include/EpochClock.h`** — UTC-seconds clock synced externally (no RTC), plus a plausibility
  check for incoming sync values. Between syncs spaced far enough apart to measure reliably, it
  also learns a drift correction from how far the local millis() clock ran ahead or behind the
  new reference, and scales future extrapolation by it -- the same clock-discipline idea NTP
  uses, applied in software since there's no oscillator-steering register on this MCU. Hardware-
  free, unit-tested. Deliberately unaware of *how* a sync value arrives (main.cpp feeds it
  whatever the radio received).
- **`include/Cycle.h`** — the `Surface -> Descent -> Park -> Ascent` phase enum and its (pure,
  tested) transition function.
- **`include/Ballast.h`** — syringe stroke-length math (microsteps per fill/drain). Hardware-free,
  unit-tested; knows nothing about motors or drivers.
- **`include/Stepper.h`** — generic DRV8825/`AccelStepper` driver: pins, microstepping setup, and
  "drive toward a target, report whether still moving." Knows nothing about ballast semantics.
  Hardware-coupled (`AccelStepper` calls `micros()` internally), not unit-tested.
- **`include/Radio.h`** — generic nRF24L01+ driver: pipe setup, power up/down, templated
  transmit/receive with a timeout. Knows nothing about the payload's contents. Hardware-coupled
  (SPI), not unit-tested.
- **`include/Payload.h`** — the packed wire struct sent to the topside station (platform ID, cycle
  number, UTC).
- **`src/main.cpp`** — pins, the `stepper::Driver`/`radio::Link` instances, and the dive-cycle
  loop; the only place that combines the generic drivers with this project's specific meaning for
  them (fill = dive, drain = surface, what to transmit at Surface). `DEBUG_TRACE_ENABLED` (off by
  default, same pattern as the `pwm` project) logs each phase transition over Serial — useful in
  the Wokwi simulator, where the radio can't be simulated and the motor's spin direction is
  otherwise the only observable sign of which phase is active. `Serial.begin()` itself is gated
  behind the flag too, so the unused UART costs nothing when it's off.

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` cover the ballast motor (A4988 + stepper, substituting for the real
DRV8825 the way `led-dimmer-4ch` substitutes plain LEDs for its PT4115 drivers) and the heartbeat
LED. Build first (`pio run`), then **F1 → "Wokwi: Start Simulator"**. The nRF24L01+ isn't in
Wokwi's part library, so `Surface` and `Park` produce no visible motion — enable
`DEBUG_TRACE_ENABLED` in `main.cpp` and open the Serial monitor to watch all four phases,
including those two.

`RESET`/`SLEEP` are jumpered together and also tied to 5V in the diagram. On real A4988 breakout
boards only the jumper is needed (`SLEEP` has an onboard pull-up that also satisfies `RESET`'s
HIGH requirement) — the extra tie to 5V here is deliberately belt-and-suspenders for the
simulated part specifically, since it's harmless either way and it isn't confirmed whether Wokwi's
model includes that pull-up. Don't take this diagram as the real-hardware wiring reference for
those two pins; a real build only needs the jumper.

## Status

Ported and audited; not yet built against real hardware. Radio behavior specifically needs
bench/hardware testing — it can't be simulated.
