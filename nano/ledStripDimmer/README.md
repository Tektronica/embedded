# nano/ledStripDimmer

> Arduino Nano firmware: a generic **N-channel dimmer → WS2812 LED-strip controller**. Each channel
> reads one dimmer (potentiometer) and drives one WS2812 LED strip through a level→color curve.

This project lives at `nano/ledStripDimmer/` in the `fw-arduino` monorepo. This is the
Nano build; a different board gets its own sibling folder (e.g. `esp32/ledStripDimmer`) rather than a
separate repo — multiple Nano units could still ship from this folder via PlatformIO environments.

## What it does

- **N channels** (default 4) — each is one **dimmer input** → one **WS2812 LED strip output**; the
  dimmer sets a smoothed 0–255 **level** (brightness).
- **Color** — an optional switch cycles a palette: heat red→orange-red (default), green, blue, white,
  or a rainbow spanning each LED strip.
- **Mode** — an optional switch cycles an animation: solid (default), blink, strobe, chase.
- Both switches are **optional**: unwired (via `INPUT_PULLUP`) they hold the defaults
  (red-orange, solid). All LED strips chain on one data pin; FastLED drives them with a current cap.

> The firmware carries **no application meaning** — for an example use (a toy-kitchen cooktop), see
> **[docs/application](docs/application/)**.

## Hardware

- Arduino Nano (ATmega328, 5 V)
- N × WS2812 LED strips (default 4 × 35 LEDs)
- N × potentiometers (dimmers)
- 5 V PSU (sized for the LED load), data-line resistor, bulk capacitor

Full BOM, power budget, wiring, pinout: **[docs/hardware](docs/hardware/)**.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.

```bash
git clone <repo-url>
cd fw-arduino/nano/ledStripDimmer
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
```

**Tests** (pure logic, off-device): `pio test -e native`. Flash gotchas: **[docs/setup](docs/setup/)**.

## Repository structure

```
nano/ledStripDimmer/
├── platformio.ini       # env:nano (build/flash) + env:native (host unit tests)
├── include/
│   └── LEDStripDimmer.h  # hardware-free logic: Levels, dimmer math, Palette/Mode, pixelColor, Button (unit-tested)
├── src/
│   └── main.cpp         # Arduino glue: pins, FastLED, loop = read dimmer inputs → render LED strip outputs
├── test/
│   └── test_controller/ # native Unity tests
├── docs/                # generic hardware/firmware docs (see below)
│   └── application/     # example use (toy-kitchen cooktop)
└── README.md
```

## Documentation

| Pillar | Contents |
|---|---|
| [docs/](docs/) | Overview |
| [docs/setup](docs/setup/) | Toolchain, building, flashing |
| [docs/hardware](docs/hardware/) | BOM, power budget & injection, wiring, pinout |
| [docs/architecture](docs/architecture/) | Structure, pure-vs-hardware split, pin ownership |
| [docs/simulation](docs/simulation/) | Run in Wokwi (VS Code), no hardware |
| [docs/troubleshooting](docs/troubleshooting/) | Environment/setup gotchas + fixes |
| [docs/application](docs/application/) | Example use: a toy-kitchen cooktop |

## Status

Firmware implemented, built, unit-tested (19/19), and **running on a Nano** (current build flashed).
The four-dimmer → four-LED-strip behavior is verified in the Wokwi simulator; physical LED strips and
dimmers aren't wired up yet.
