# Dino Run Game

A T-Rex-Run-style game. An OLED panel shows a running dino jumping over scrolling cacti, a 4-digit
7-segment display shows the score, and a rotary encoder's pushbutton controls jump, start, and
restart. The code is a rewrite of a single-file Arduino IDE sketch (`Dino.ino`, credited to Ishani
and Arijit Sengupta, based on the 30DLIS kit).

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/ToneSequence` |
| Other libraries | `avishorp/TM1637` from GitHub, `olikraus/U8glib` from the registry |
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

1. Build: `pio run -d projects/game-dino-run/fw`
2. Flash the Nano: `pio run -d projects/game-dino-run/fw -t upload`. Auto-reset is unreliable on
   this CH340 board, so tap RESET as the upload starts.
3. Run the native tests: `pio test -d projects/game-dino-run/fw -e native`

## Hardware

### BOM

| Role | Part |
| --- | --- |
| MCU | Arduino Nano (ATmega328P) |
| Gameplay display | SH1106 128x64 OLED panel, I2C |
| Score display | TM1637 4-digit 7-segment |
| Input | Rotary encoder module. Only its pushbutton is used |
| Sound | Passive buzzer |

### Pins

From `fw/src/main.cpp`.

| Function | Pin | Note |
| --- | --- | --- |
| Encoder CLK | D2 | Configured as input, never read |
| Encoder SW (button) | D3 | `INPUT_PULLUP`, interrupt on `FALLING` |
| Encoder DT | D4 | Configured as input, never read |
| Score display DIO | D5 | |
| Score display CLK | D6 | |
| Buzzer | D10 | |
| OLED SDA, SCL | A4, A5 | I2C, addressed by `U8glib` |

## How it works

### Game states

- Start: the OLED shows a welcome screen. A press starts a run.
- Playing: the dino runs and a press makes it jump. Two obstacles are always in flight, picked from
  6 cactus shapes, with a gap of 80 to 124 px between them. Hitting one ends the run. The score
  climbs with time and speed, shows live on the 7-segment display, and beeps every 100 points.
- End: a "Game Over" banner shows over the frozen scene. A press returns to Start, and a second
  press starts a new run.

### Firmware modules

Hardware-free game logic lives in `fw/include/` and runs in the native tests. Hardware objects and
rendering live in `fw/src/main.cpp`.

- `GameState.h`: the `Start -> Playing -> End -> Start` transition on a button press.
- `Dino.h`: the run-cycle animation and jump physics, as two separate fields, `legFrame` and
  `jumpHeight`.
- `Obstacles.h`: the scrolling obstacle pair, recycling, and collision detection. `main.cpp`
  passes in the random values, so the logic is deterministic under test.
- `Score.h`: score from elapsed time, and the 100-point milestone threshold.
- `Buzzer.h`: jump, milestone, and hit sounds as `{frequency, duration}` step tables, played by
  `lib/arduino/ToneSequence` and polled every loop with no blocking `delay()`.
- `Sprites.h`: the OLED bitmap data.
- `main.cpp`: pins, the `TM1637Display` and `U8GLIB_SH1106_128X64` objects, the button ISR,
  rendering, and the loop.

The button ISR only debounces by timestamp (180 ms) and sets a flag. The loop handles the press
and starts any sound.

### Changes from the original sketch

- Blocking `delay(150)` calls for the milestone and hit sounds froze animation and input for
  150 ms. `Buzzer.h` replaced them with a non-blocking pattern.
- The ISR used to call `tone()`. It now only sets a flag.
- One variable, `dinoMove`, held both the 0 to 2 leg frame and the 0 to 32 jump height. It is now
  two fields, so a jump always starts from height 0.
- Obstacle shape 6 (`threeCactusSmall`) could never spawn, because `random(1, 6)` excludes its
  upper bound. It is now `random(1, 7)`.
- Removed the unused `currentStateCLK` and `lastStateCLK` variables, and a collision case for
  shape 0, the decorative cloud.
- `resetGame()` used to run every frame on the Start screen. It now runs once, on the Start to
  Playing transition.
- The vendored `TM1637Display.cpp` and `.h` were replaced by the `avishorp/TM1637` GitHub source
  in `lib_deps`. The registry copy of that library has only an unversioned alpha release.

## Folders

- `fw/`: firmware, a PlatformIO project with the Wokwi diagram and native tests in `test/`.

## Simulate

`fw/diagram.json` has a KY-040 rotary encoder, a TM1637 7-segment display, a buzzer, and an OLED.

1. Build: `pio run -d projects/game-dino-run/fw`
2. In VS Code, run F1 > "Wokwi: Start Simulator".
3. Press the encoder's button to start, jump, and restart.

## Known issues

- The diagram uses a `board-ssd1306` OLED, but `main.cpp` drives an SH1106 through
  `U8GLIB_SH1106_128X64`. The simulated image may be offset by a few pixels. This mismatch is
  inherited from the original diagram and is unverified.
- `olikraus/U8glib` is no longer maintained. Building against it gives 3 assembler warnings
  (`ignoring changed section attributes for .progmem.data`) and 30 low-severity `cppcheck` style
  notes, all inside `U8glib.h` and none in this project's code.

## Status

Rewritten and audited, with 24 native tests. Not yet run on real hardware.
