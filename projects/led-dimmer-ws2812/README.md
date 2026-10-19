# LED Dimmer WS2812

An N-channel dimmer for WS2812 LED strips. Each channel reads one potentiometer and drives one
WS2812 LED strip through a level-to-color curve. Two optional switches cycle the color palette and
the animation mode. The firmware carries no application meaning. `docs/application/` describes one
example use, a toy-kitchen cooktop.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/PotInput`, `lib/arduino/Debounce` |
| Other libraries | FastLED 3.7.0, from the GitHub release tarball |
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

1. Build: `pio run -d projects/led-dimmer-ws2812/fw`
2. Flash the Nano: `pio run -d projects/led-dimmer-ws2812/fw -t upload`. Auto-reset is unreliable
   on this CH340 board, so tap RESET as the upload starts.
3. Run the native tests: `pio test -d projects/led-dimmer-ws2812/fw -e native`

FastLED comes from GitHub because Zscaler blocks the PlatformIO registry CDN on this network.
See `docs/troubleshooting/` for that and other flashing problems, and `docs/setup/` for toolchain
details.

## Hardware

Default 4-channel build. `docs/hardware/` has the power budget and wiring rules.

### BOM

| Qty | Component | Spec |
| --- | --- | --- |
| 1 | Arduino Nano | ATmega328P, 5 V |
| 4 | WS2812 LED strip | 35 LEDs each, 140 total |
| 4 | Potentiometer | 10 kΩ linear (B10K), single-turn, one per channel |
| 2 | Momentary push switch | Optional, cycles mode and color |
| 1 | 5 V DC power supply | 6 A with the firmware current cap, or 10 A for full-white headroom |
| 1 | Resistor | 330 to 470 Ω, in series on the data line |
| 1 | Capacitor | About 1000 uF, across the 5 V rail near the LEDs |

### Pins

From `fw/src/main.cpp`.

| Signal | Pin | Note |
| --- | --- | --- |
| LED data (`PIN_DATA_LED`) | D6 | One line drives all 140 LEDs on the chained LED strips |
| Channel 0 to 3 dimmers (`PIN_DIMMER`) | A0, A1, A2, A3 | Pot wiper. Outer terminals to 5 V and GND |
| Mode switch (`PIN_SWITCH_MODE`) | D8 | Optional. `INPUT_PULLUP`, switch to GND |
| Color switch (`PIN_SWITCH_COLOR`) | D9 | Optional. `INPUT_PULLUP`, switch to GND |

Power each LED strip in parallel from the 5 V supply and chain only the data line. Never power the
LED strips from the Nano's 5V pin. The supply and the Nano must share ground.

## How it works

Each frame, the firmware reads every dimmer, smooths it with an exponential moving average
(`EMA_SHIFT` = 3), applies a deadzone from `lib/arduino/PotInput`, and scales it to a 0 to 255
level. It then computes every pixel with `pixelColor(palette, mode, level, pixel, stripLen, frame)`
and calls `FastLED.show()`. Channel N owns LED indices `N * 35` to `N * 35 + 34` in one shared
buffer.

- Palette: heat red to orange-red (default), green, blue, white, or a rainbow across each LED
  strip. The color switch cycles it.
- Mode: solid (default), blink, strobe, or chase. The mode switch cycles it. A `frame` counter
  drives the animations.
- Both switches use `INPUT_PULLUP` and `debounce::Button`. An unwired switch never registers a
  press, so the defaults hold.
- `FastLED.setMaxPowerInVoltsAndMilliamps(5, 5500)` caps the draw at 5.5 A. Full white on all
  140 LEDs would draw about 8.4 A.
- `FastLED.delay(16)` paces the loop at about 60 frames per second.

`fw/include/LEDStripDimmer.h` holds all hardware-free logic (`Levels`, `adcToLevel()`, the
`Palette` and `Mode` enums, `pixelColor()`), so the native tests run it on the host.
`fw/src/main.cpp` holds the pins, the FastLED setup, and the loop. The LED framebuffer takes 420
of the Nano's 2048 bytes of SRAM.

## Folders

- `fw/`: firmware, a PlatformIO project with the Wokwi diagram.
- `docs/`: design notes in pillars: `setup/`, `hardware/`, `architecture/`, `simulation/`,
  `troubleshooting/`, and `application/`.

## Simulate

`fw/diagram.json` wires 4 Wokwi LED strips, 4 potentiometers, and the 2 switches.

1. Build: `pio run -d projects/led-dimmer-ws2812/fw`
2. In VS Code, run F1 > "Wokwi: Start Simulator".

`docs/simulation/` has more detail.

## Status

The firmware passes 16 native tests and runs on a Nano. Wokwi confirms the 4-dimmer to
4-LED-strip behavior, but the physical LED strips and dimmers are not wired yet.
