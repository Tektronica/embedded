# Robot Buoy

Firmware for a vertical-profiling float. The float dives with a stepper-driven syringe ballast,
parks at depth, resurfaces, and sends telemetry (platform ID, cycle number, current UTC estimate)
to a topside station over an nRF24L01+ radio, then repeats. Phase names (Surface, Descent, Park,
Ascent) and payload fields follow Argo profiling-float conventions. The code is a port of a
standalone Uno sketch built for the MATE ROV "RANGER" profiling-float task
(`profiling-float/src/buoy`), rescoped to the Nano.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | None |
| Other libraries | `waspinator/AccelStepper`, `nrf24/RF24` |
| Custom PCB | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests through `env:native`.
- Wokwi simulator, through `fw/diagram.json` and `fw/wokwi.toml`.

## Getting started

Run these from the repo root. If `pio` isn't on your shell `PATH` (VS Code-only installs), add it
first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d projects/robot-buoy/fw`
2. Flash the Nano: `pio run -d projects/robot-buoy/fw -t upload`. Auto-reset is unreliable on this
   CH340 board, so tap RESET as the upload starts.
3. Run the native tests: `pio test -d projects/robot-buoy/fw -e native`
4. Watch the phase trace: set `DEBUG_TRACE_ENABLED` to `true` in `fw/src/main.cpp`, flash, then
   run `pio device monitor -d projects/robot-buoy/fw`. The monitor runs at 9600 baud.

## Hardware

### BOM

From the original design.

| Role | Part |
| --- | --- |
| MCU | Arduino Nano (ATmega328P) |
| Stepper driver | DRV8825 |
| Stepper motor | Bipolar stepper, the syringe ballast actuator |
| Radio | nRF24L01+ |

### Pins

From `fw/src/main.cpp`.

| Function | Pin | Note |
| --- | --- | --- |
| Ballast direction | D2 | DRV8825 `DIR` |
| Ballast step | D3 | DRV8825 `STEP` |
| Ballast microstep select (MS3, MS2, MS1) | D4, D5, D6 | All high selects 1/32 microstepping |
| Ballast enable | D7 | DRV8825 `~ENABLE`, active low |
| Heartbeat LED | D8 | Toggles every 1 s, polled with `millis()` |
| Radio CSN | D9 | |
| Radio CE | D10 | |
| SPI (MOSI, MISO, SCK) | D11, D12, D13 | Hardware SPI bus. Nothing else may use these pins |

## How it works

### Dive cycle

- Surface: wake the radio, transmit `{UTC estimate, platform ID, cycle number}`, and wait up to
  `RX_TIMEOUT_MS` (5 s) for the topside station to return a corrected UTC value. The clock accepts
  any value before its first sync, then only values within `UTC_TOLERANCE_S` (60 s) of its own
  estimate. The radio then sleeps. Surface lasts at least
  `SURFACE_MIN_DURATION_MS` (2 s), even with an instant reply or no radio connected.
- Descent: the syringe draws water in (more mass, less buoyant) until the stroke completes.
- Park: wait `PARK_DURATION_MS` (5 s). No sensor is in the BOM yet, so nothing is collected.
- Ascent: the syringe expels the water back to its start position. The cycle number increments.

Clockwise is fill and counterclockwise is drain, fixed by the physical wiring. If the motor runs
backwards, swap the stepper's direction-pin polarity, not the firmware calls.

### Configuration

| Aspect | Constant | Value | Note |
| --- | --- | --- | --- |
| Stroke ("depth") | `ballast::FILL_REVOLUTIONS` in `Ballast.h` | 5 revolutions | 200 steps per revolution at 1/32 microstepping. Placeholder, calibrate against real syringe travel |
| Park duration | `PARK_DURATION_MS` in `main.cpp` | 5000 ms | Placeholder |
| Surface reply timeout | `RX_TIMEOUT_MS` in `main.cpp` | 5000 ms | Maximum wait for a topside reply |
| Surface minimum | `SURFACE_MIN_DURATION_MS` in `main.cpp` | 2000 ms | Floor on total Surface time |
| Platform ID | `payload::PLATFORM_ID` in `Payload.h` | 0 | Set to the unit's assigned ID |

Depth is open-loop. The ballast runs a fixed, all-or-nothing stroke and the firmware assumes it
worked. A pressure sensor would let Descent and Ascent confirm the pressure actually changes.

### Firmware modules

- `fw/include/EpochClock.h`: a UTC-seconds clock synced over the radio, with no RTC. It checks
  each incoming value against its own last estimate, and learns a drift correction between syncs
  spaced far enough apart. Hardware-free and tested.
- `fw/include/Cycle.h`: the phase enum and its transition function. Hardware-free and tested.
- `fw/include/Ballast.h`: stroke length in microsteps. Hardware-free and tested.
- `fw/include/Stepper.h`: a generic DRV8825 driver on `AccelStepper` that sets microstepping and
  drives toward a target. Not tested, since `AccelStepper` calls `micros()`.
- `fw/include/Radio.h`: a generic nRF24L01+ driver with pipe setup, power control, and templated
  transmit and receive with a timeout. Not tested, since it needs SPI.
- `fw/include/Payload.h`: the packed telemetry struct, fields ordered largest first.
- `fw/src/main.cpp`: pins, driver instances, and the dive-cycle loop. `DEBUG_TRACE_ENABLED`
  (off by default) prints each phase transition over Serial at 9600 baud and gates
  `Serial.begin()` too.

### Changes from the original sketch

- Radio CSN moved from D11, the hardware SPI MOSI line, to D9.
- The heartbeat LED moved from D13, the SPI SCK line, to D8. A `millis()`-polled blink in
  `loop()` replaced the watchdog interrupt that used to toggle it, so it can never fire mid
  transfer.
- `Radio.h` checks `RF24::isChipConnected()` before `transmit()` and `receive()`. Without that
  check, `RF24::write()` blocks forever when no chip answers, which froze the first Surface phase.
- `setRetries()` replaced `setAutoRetransmit()`, which the resolved `RF24` version does not have.
- UTC validation compares against the float's last estimate instead of `time(nullptr)`, which
  means nothing without an RTC.
- Removed a duplicate `Serial.begin()` and a `while (!Serial) {}` wait that does nothing on the
  Nano.

## Folders

- `fw/`: firmware, a PlatformIO project with the Wokwi diagram and native tests in `test/`.

## Simulate

`fw/diagram.json` has the ballast motor, using an A4988 in place of the DRV8825, and the heartbeat
LED. Wokwi has no nRF24L01+ part, so Surface and Park show no motion. Enable `DEBUG_TRACE_ENABLED`
and open the serial monitor to see all 4 phases.

1. Build: `pio run -d projects/robot-buoy/fw`
2. In VS Code, run F1 > "Wokwi: Start Simulator".

The diagram jumpers the A4988's `RESET` to `SLEEP` and also ties both to 5 V. A real A4988 board
needs only the jumper. Do not use the diagram as the wiring reference for those 2 pins.

## Known issues

- No fail-safe for an unrecognized cycle phase. The original design planned a
  `default: beacon()` ping home, but neither the original sketch nor this port builds it.
- Depth is open-loop, with no pressure sensor.
- Park collects no data. There is no sensor hardware yet.

## Status

Ported and audited, with 12 native tests. Not yet built on real hardware, and the radio needs bench
testing because Wokwi cannot simulate it.
