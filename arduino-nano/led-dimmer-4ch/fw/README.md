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
