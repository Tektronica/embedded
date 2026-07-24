# arduino-nano/led-dimmer-4ch/fw

Firmware for the 4-channel LED dimmer: 4 potentiometers (A0–A3) each drive one PT4115 LED
driver's DIM pin via PWM (D11, D10, D9, D3), one channel per pot. See the
[project README](../README.md) for the full board — BOM, PWM pin reference, KiCad, datasheets.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/led-dimmer-4ch/fw
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` substitute plain LED+resistor pairs for the PT4115 modules. Build
first (`pio run`), then **F1 → "Wokwi: Start Simulator"** and turn the potentiometers to verify
each channel's PWM/pot correspondence without real hardware.

## ADC pins: multiplexer crosstalk vs. floating-pin leakage

The AVR's ADC shares one multiplexer and one sample-and-hold capacitor across all analog pins.
Two unrelated failure modes follow: crosstalk between channels actively being read, and
power/noise leakage from any pin left floating. Different causes, different fixes.

### Rule 1 — Settle the capacitor before trusting a multi-channel read

On a channel switch, the shared capacitor still holds the previous channel's charge until the new
source drives it to its own voltage. Sources above ~10 kΩ can leave a residual bias in the next
reading. Only relevant once firmware alternates between ≥2 channels — a single-channel design
never switches the mux, so this can't happen regardless of what else is wired up.

Fix: read the channel twice and discard the first result, or add a ~0.1 µF cap from pin to GND to
speed up settling.

### Rule 2 — Define unused pins, ADC or not

Every ADC-capable pin is also a digital I/O pin with its input buffer active by default. A
floating pin sits at an indeterminate voltage, causing shoot-through current in that buffer and
acting as a noise antenna — regardless of whether `analogRead()` ever touches it.

Fixes, in order:

1. `pinMode(pin, INPUT_PULLUP)`, or drive it as an output.
2. Set the pin's `DIDR0` bit to disable its digital input buffer (the datasheet's own fix for
   analog-only pins).
3. Tie to GND, directly or through ~10 kΩ, if the state must be defined before `setup()` runs.

A cap to GND alone does not fix this — no DC path, so the pin still drifts. Caps filter real
signals (Rule 1); they don't define an unused pin's state.
