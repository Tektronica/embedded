# Stepper

A potentiometer sets the rotation speed of a bipolar stepper motor through a DRV8825 or A4988
style driver. Two buttons toggle run/stop and clockwise/counterclockwise. The example covers
step-pulse timing and speed control, not positioning, and compares an `AccelStepper` strategy
with a library-free one.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano (`nanoatmega328`), ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/PotInput`, `lib/arduino/Debounce` |
| Other libraries | `waspinator/AccelStepper` (PlatformIO registry) |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests (`env:native`) for `include/Stepper.h`.
- Wokwi simulator (`diagram.json`, `wokwi.toml`).

## Getting started

If `pio` is not on your shell `PATH` (VS Code extension only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

Run from the repo root:

1. Build: `pio run -d examples/arduino-nano/stepper`
2. Build and flash the Nano: `pio run -d examples/arduino-nano/stepper -t upload`
3. Run the native tests: `pio test -d examples/arduino-nano/stepper -e native`
4. Open the serial monitor (only prints when `DEBUG_TRACE_ENABLED` is `true` in `src/main.cpp`):
   `pio device monitor -d examples/arduino-nano/stepper`

## Hardware

- Arduino Nano
- Bipolar stepper motor
- DRV8825 or A4988 driver board
- Potentiometer
- 2 momentary pushbuttons

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Potentiometer wiper | A0 | Outer terminals to 5 V and GND |
| Driver `DIR` | D2 | Owned by `AccelStepper` in Library mode, written directly in DirectPulse mode |
| Driver `STEP` | D3 | |
| Run/stop button | D4 | `INPUT_PULLUP`, other leg to GND |
| Direction button | D5 | `INPUT_PULLUP`, other leg to GND |
| Driver `ENABLE`, `MS1`, `MS2`, `MS3` | None | Tied to GND on the driver board: always enabled, full-step |

Both strategies use the same wiring.

## How it works

The motor starts stopped. Each fresh press of run/stop toggles running, and each fresh press of
the direction button toggles clockwise. Each loop smooths the pot with `potinput::emaStep()`
(`EMA_SHIFT` 3), applies `potinput::Deadzone`, and maps it to 0 to 800 steps/s with
`stepper::potToSpeed()`. Speed is 0 while stopped.

### Stepper strategy

`STEP_MODE` in `src/main.cpp` selects the strategy at compile time. The default is
`StepMode::Library`.

- `StepMode::Library`: `renderLibrary()` drives `AccelStepper` with `moveTo()` and `run()`,
  ramped by `setAcceleration()` at 500 steps/s². `run()` only ramps while approaching a target, so
  the firmware retargets 2,000,000 steps ahead in the current direction and never arrives. On a
  direction change, or when fewer than 200,000 steps remain, it retargets the other way, and
  `run()` decelerates to a stop and accelerates back up. `setMaxSpeed()` follows the pot every
  loop, so stopping also ramps down.
- `StepMode::DirectPulse`: `renderDirectPulse()` toggles `STEP` by hand on a `micros()` timer, with
  no library. The interval comes from `stepper::stepIntervalMicros()`, which is
  `1,000,000 / stepsPerSec`. Speed 0 holds `STEP` low. There is no acceleration curve, so speed
  and direction change immediately, which risks skipped steps on a sudden reversal under load.

Neither strategy uses hardware timer register modes, so both are expected to simulate in Wokwi.

### Firmware layout

- `include/Stepper.h`: hardware-free `potToSpeed()` and `stepIntervalMicros()`. It knows nothing
  about the buttons.
- `lib/arduino/PotInput`: smoothing and the rail deadzone.
- `lib/arduino/Debounce`: `debounce::Button`, one instance per button.
- `src/main.cpp`: pins, button state, both strategies, and the loop.

`DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot trace of `raw`, `speed`, `running`, and
`clockwise` every 200 ms at 115200 baud.

## Simulate

The Wokwi diagram uses an A4988 and a stepper motor, plus a potentiometer and two pushbuttons.
Build, then run **F1 > Wokwi: Start Simulator** in VS Code. Press RUN/STOP to start the motor,
drag the potentiometer to change speed, and press CW/CCW to reverse.

## Known issues

- `StepMode::DirectPulse` has no acceleration ramp. See Stepper strategy.
- Auto-reset is unreliable on this CH340 Nano. Tap RESET as the upload starts.

## Status

Built, with 4 native unit tests for `Stepper.h`. Both strategies are selectable by one constant.
