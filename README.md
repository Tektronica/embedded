# fw-nano-ledStripDimmer

> Arduino Nano firmware: a generic **N-channel dimmer → WS2812 LED-strip controller**. Each channel
> reads one dimmer (potentiometer) and drives one WS2812 LED strip through a level→color curve.

Repo name follows `[codingType]-[targetPlatform]-[projectType]`: `fw` · `nano` · `ledStripDimmer`.
This is the Nano build; a different board is a *separate* repo unless it shares this codebase (then
it's another PlatformIO environment).

## What it does

- **N channels** (default 4) — each channel is one **dimmer input** → one **WS2812 LED strip output**.
- A dimmer's position → a smoothed 0–255 **level** → a **color**, via a swappable curve (`levelColor`).
- All LED strips are data-chained on one pin and addressed as one logical buffer.
- FastLED drives the WS2812 LED strips, with a current cap to protect the supply.

> The default `levelColor` curve ramps deep red → orange. The firmware carries **no application
> meaning** — for an example use (a toy-kitchen cooktop), see **[docs/application](docs/application/)**.

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
cd fw-nano-ledStripDimmer
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
```

**Tests** (pure logic, off-device): `pio test -e native`. Flash gotchas: **[docs/setup](docs/setup/)**.

## Repository structure

```
fw-nano-ledStripDimmer/
├── platformio.ini       # env:nano (build/flash) + env:native (host unit tests)
├── include/
│   └── LEDStripDimmer.h  # hardware-free logic: Levels, ADC→level, levelColor curve (unit-tested)
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

Firmware implemented, built, unit-tested (10/10), flashed to a Nano, and verified in the Wokwi
simulator. Real-hardware bring-up pending wiring.
