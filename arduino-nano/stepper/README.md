# arduino-nano/stepper

> Arduino Nano firmware: a potentiometer-controlled stepper motor. The pot sets rotation speed.

This project lives at `arduino-nano/stepper/` in the `embedded` monorepo.

## What it does

One **potentiometer** (A0) sets **rotation speed** (0..`MAX_SPEED_STEPS_PER_SEC`) on a bipolar
stepper motor via a DRV8825/A4988-style driver. Two **buttons** control **run/stop** and
**CW/CCW** direction, each toggling on a fresh press. This project is about step-pulse timing and
control, not positioning — there's no target angle or move-to-position behavior.

This folder is also a testbed comparing two stepper-driving approaches — see below. It's the
general-purpose counterpart to `robot-buoy`'s `Ballast.h`/`Stepper.h`: that project's stepper
usage is a bounded move-to-target stroke (`AccelStepper::moveTo()` + acceleration profile); this
one is continuous, pot-controlled speed (`AccelStepper::setSpeed()`/`runSpeed()`, a different
`AccelStepper` mode) and a from-scratch alternative to it.

## Stepper strategy

`src/main.cpp` has two encapsulated step-generation implementations, selected by one compile-time
constant, `STEP_MODE` (a `StepMode` enum, default `StepMode::Library`):

- **`Library` (default)** — `renderLibrary()`: continuous rotation built from `AccelStepper`'s
  `moveTo()`/`run()`, ramped by `setAcceleration()` — **including on direction reversal**.
  The trick: `run()` only ramps speed while approaching a `moveTo()` target, so "spin
  indefinitely" means retargeting far ahead in the current direction and never actually arriving;
  flipping direction (or nearing the target) retargets far the other way, and `run()`
  automatically decelerates to a stop then accelerates the other way, using the library's own
  ramp — not a custom one. `setMaxSpeed()` is updated from the pot every call, so stopping (speed
  0) also ramps down smoothly instead of cutting the motor immediately. `AccelStepper` owns the
  `DIR` pin itself for the `DRIVER` interface, so this path never writes to it directly.
- **`DirectPulse` (reference implementation)** — `renderDirectPulse()`: toggles the `STEP` pin
  by hand, timed entirely by `micros()`, with no `AccelStepper` involved at all. The interval
  between pulses is `stepper::stepIntervalMicros(speed)` — one line of math
  (`1,000,000 / stepsPerSec`). Writes `DIR` directly, since nothing else owns that pin on this
  path. **No acceleration curve at all** — speed and direction changes are immediate, which for a
  real stepper under load risks skipped steps on a sudden reversal. That's a genuine tradeoff of
  going library-free, not an oversight: building an equivalent ramp by hand would erase most of
  the "one line of math" simplicity this path is demonstrating.

Unlike the `pwm` project's `DirectTimer` path, both strategies here are just `micros()` +
`digitalWrite()` (or `AccelStepper` calls that do the same under the hood) — no hardware timer
register modes are involved, so both are expected to simulate correctly in Wokwi. Flip
`STEP_MODE` to compare them; the wiring is identical either way.

## Design

- **`include/Stepper.h`** — hardware-free logic (unit-tested via `pio test -e native`), shared by
  both strategies:
  - `potToSpeed()` / `emaStep()` — pot scaling + smoothing (same pattern as `pwm`'s `Pwm.h`)
  - `stepIntervalMicros(stepsPerSec)` — target speed → microseconds between pulses
    (`DirectPulse` path only; `Library` path passes speed straight to `AccelStepper`)
  - Knows nothing about the buttons — `main.cpp` reads them and passes plain `speed`/`clockwise`
    values in, keeping this header purely about pot→speed math.
- **`include/Button.h`** — a debounced push-button edge detector (unit-tested), same pattern as
  `led-dimmer-ws2812`'s `Button` class. Generic, not stepper-specific — split into its own header
  rather than living in `Stepper.h`, since debouncing a button and mapping a pot to a speed are
  unrelated concerns.
- **`src/main.cpp`** — pins, two `Button` instances (run/stop, direction) toggling `running`/
  `clockwise` state on each fresh press, both strategies as encapsulated functions, the loop (read
  pot → speed, zeroed if stopped → whichever strategy is selected). `DEBUG_TRACE_ENABLED` (off by
  default) prints a Teleplot-format trace (`raw`/`speed`/`running`/`clockwise`) for debugging.

Kept deliberately small — single-responsibility functions (ADC→speed, speed→interval,
button→edge) and no class hierarchies or interfaces.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/stepper
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Wiring

| Signal | Nano pin | Notes |
|---|---|---|
| Potentiometer wiper | A0 | outer terminals to 5V/GND, wiper to A0 |
| Driver `DIR` | D2 | driven differently per strategy — see "Stepper strategy" above |
| Driver `STEP` | D3 | |
| Run/stop button | D4 | `INPUT_PULLUP`, other leg to GND — toggles on each fresh press |
| Direction button | D5 | `INPUT_PULLUP`, other leg to GND — toggles CW/CCW on each fresh press |
| Driver `ENABLE`, `MS1`, `MS2`, `MS3` | — | hardwired to GND on the driver board (always enabled, full-step) — not driven by the MCU |

Same wiring for both strategies — only the step-generation code differs, not the pins.

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (A4988 + stepper motor, same substitution `robot-buoy`
uses for its real DRV8825, plus two pushbuttons). Build (`pio run`), then **F1 → "Wokwi: Start
Simulator"**. Press RUN/STOP to start the motor, drag the potentiometer to change its speed, and
press CW/CCW to reverse direction.
