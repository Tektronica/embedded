# arduino-nano/led-dimmer-ws2812 — Project Context

Arduino Nano firmware: a generic **N-channel dimmer → WS2812 LED-strip controller**. Each channel
reads one dimmer (potentiometer) and drives one WS2812 LED strip via a level→color curve. The
firmware models the *mechanism*, not any application — an example use is in `docs/application/`.

## Hardware constraints

- MCU: Arduino Nano (ATmega328) — **2 KB SRAM, 32 KB flash**. The LED framebuffer alone is
  `LED_TOTAL` × 3 = 420 B (~20% of SRAM). Budget memory deliberately.
- 5 V logic, drives WS2812 data directly (no level shifter).
- Power, wiring, and pinout are authoritative in `docs/hardware/`.

## Code conventions

- **No dynamic allocation** in the hot path — avoid `new`, `String`, `std::vector`,
  `std::function`, RTTI. Static allocation only.
- **Two files.** `include/LEDStripDimmer.h` holds all hardware-free logic (`Levels`,
  `adcToLevel`/`emaStep`, `levelColor`) so it unit-tests off-device; `src/main.cpp` is the Arduino
  glue (pins/power, FastLED) and the **read dimmer inputs → render LED strip outputs** loop. Keep
  the pure-vs-hardware split — it's what makes the tests possible.
- **Generic vocabulary, not application vocabulary:** dimmer (input), LED strip (output), channel,
  level. **Never the word "strip" alone — always "LED strip".** No kitchen/hob/cooktop in core code
  or docs; application framing lives only in `docs/application/`.
- **Constants:** prefix / broad→narrow — `PIN_DATA_LED`, `PIN_DIMMER`, `LED_BRIGHTNESS_MAX`,
  `LED_STRIP_LENGTH`, `LED_TOTAL`, `CHANNEL_COUNT`, `PSU_VOLTS`, `ADC_MAX`, `FRAME_DELAY_MS`.
- **No premature abstraction.** An MVC + `IAppliance` plugin layer was dropped as over-built
  (in git history); reintroduce only with a real second use.
- Library: **FastLED** (HSV + `setMaxPowerInVoltsAndMilliamps` current cap).

## Naming

- This project = `arduino-nano/led-dimmer-ws2812/` (board `arduino-nano`, project `led-dimmer-ws2812`) — see the root
  `CLAUDE.md` for the monorepo's folder/naming scheme.
- A different board gets its own top-level folder in this repo (e.g. `esp32/led-dimmer-ws2812`), not a
  separate repo — the board is still the project boundary.
- Disk grouping/category buckets (outside this repo) use a snake `_` prefix (`_embedded`).

## Build / flash

```bash
pio run                 # build (arduino-nano)
pio run -t upload       # build + flash  (CH340 clone: tap RESET as the upload starts)
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

Toolchain: VSCode + PlatformIO (no Arduino IDE). Details in `docs/setup/`.

## Committing

- See the root `CLAUDE.md` for repo-wide commit policy (local-only, no AI attribution, etc.) and the
  `/commit` skill.
- Pre-commit gate for this project: `pio run` (0 errors/warnings) + `pio test -e native`.

## Docs

`docs/` is generic to the hardware/firmware: `setup/`, `hardware/`, `architecture/`, `simulation/`,
`troubleshooting/`, plus `application/` (example use). Update the relevant pillar when behavior changes.
