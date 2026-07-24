# arduino-nano/game-dino-run

Arduino Nano firmware for a T-Rex-Run-style game: an OLED panel shows a running dino jumping over
scrolling obstacles, a 7-segment display shows the score, and a rotary encoder's pushbutton
controls jump / start / restart.

Originally a single-file Arduino IDE sketch (`Dino.ino`, credited to Ishani and Arijit Sengupta,
based on the 30DLIS kit) with a vendored copy of the TM1637 display library sitting alongside it.
Rewritten to this repo's conventions — hardware-free game logic extracted into tested headers,
hardware glue kept thin in `main.cpp` — and audited for real bugs along the way (see below).

## How it plays

- **Start** — OLED shows a welcome screen. Press the button to begin.
- **Playing** — the dino runs; press the button to jump. Two obstacles are always in flight;
  hitting one ends the run. Score climbs with time and speed, shown live on the 7-segment
  display, with a beep every 100 points.
- **End** — a "Game Over" banner shows, frozen mid-scene at the moment of collision. Press the
  button to return to Start (a second press starts a new run).

## Design

Pure, hardware-free game logic lives in `include/`, unit-tested via `pio test -e native`. Hardware
objects (the OLED panel, the 7-segment display, the interrupt) and rendering live in `src/main.cpp`
— the only place any of it touches actual pins or peripherals.

- **`GameState.h`** — `Start -> Playing -> End -> Start`, the button-press transition.
- **`Dino.h`** — run-cycle animation and jump physics. Two independent pieces of state
  (`legFrame`, `jumpHeight`) replace one overloaded variable in the original — see "Audit" below.
- **`Obstacles.h`** — scrolling obstacle pair, recycle-on-scroll-off logic, and collision
  detection. Randomness is injected as a parameter rather than called internally, so recycling is
  deterministic and testable; `main.cpp` supplies real `random(...)` values.
- **`Score.h`** — score-from-elapsed-time math and the 100-point milestone-beep threshold.
- **`Buzzer.h`** — tone/frequency selection and beep-pattern sequencing (jump, milestone, hit),
  polled every loop with no blocking `delay()` — same pattern as `toy-microwave`'s `Buzzer.h`. See
  "Audit" below for why this replaced the original's approach.
- **`Sprites.h`** — the raw OLED bitmap data, kept separate so neither game logic nor `main.cpp`
  is dominated by hundreds of lines of pixel bytes.
- **`main.cpp`** — pins, the `TM1637Display`/`U8GLIB_SH1106_128X64` objects, the button ISR,
  rendering, and the loop that ties the pure headers together.

**Design principles, honestly assessed:** each header has one clear responsibility (state
machine, physics, obstacles, score, sound), which is most of what SOLID's Single Responsibility
Principle asks for here — a small game doesn't have the kind of extension points Open/Closed or
Dependency Inversion are meant to protect, so introducing interfaces or a plugin layer for a
fixed, small set of states/obstacle shapes would be the over-engineering this repo's other
projects explicitly avoid (see `led-dimmer-ws2812`'s `CLAUDE.md`). No GoF patterns are used
outright; `Buzzer.h`'s `Pattern` + `toneStateFor()` is a lightweight State-pattern-as-data, the
same shape as this repo's other buzzer/cycle headers, chosen over polymorphic classes for the
same reason — no virtual dispatch cost on a 2 KB-RAM part for a problem this small.

## Audit: bugs found and fixed during the rewrite

1. **Blocking `delay(150)` calls inside the game loop** for the milestone and hit sounds froze
   animation and input for 150 ms every time either fired. Fixed with a non-blocking `Buzzer.h`
   (`Pattern` + `toneStateFor(pattern, elapsedMs)`, polled every loop) — the same fix already
   applied in `toy-microwave` for the same class of bug.
2. **`tone()` was called from inside the button interrupt handler.** ISRs should stay minimal;
   the ISR now only debounces (by timestamp) and sets a flag, and the main loop processes the
   press and starts the (non-blocking) buzzer pattern.
3. **One variable did two unrelated jobs**: `dinoMove` was both the 0–2 running-leg animation
   frame and the 0–32 jump-height offset, disambiguated only by which numeric range it was in.
   Split into `legFrame`/`jumpHeight` in `Dino.h`'s `State`. This also fixes a minor, probably
   unintended side effect: a jump's height used to start from whatever run-cycle frame (0, 1, or
   2) happened to be active the instant you jumped, instead of always starting cleanly at 0.
4. **Obstacle shape 6 (`threeCactusSmall`) could never spawn.** Shapes were recycled via
   `random(1, 6)`, which on Arduino is exclusive of the upper bound — only ever producing 1–5.
   Fixed to `random(1, 7)`, so all six bitmap shapes (art that already existed) are actually
   reachable now.
5. **Dead code removed**: `currentStateCLK`/`lastStateCLK` (the rotary encoder's rotation is
   wired up — CLK/DT are still configured as inputs, matching the physical module — but was never
   read anywhere), and a collision-size lookup case for shape 0, which is the decorative cloud
   and can never actually be assigned as an obstacle.
6. **`resetGame()` was called every frame while idle on the Start screen** (inside the render
   function), relying on the last such call before transitioning out being the "real" reset.
   Harmless but confusing; now called once, exactly on the Start → Playing transition.

Vendored `TM1637Display.cpp`/`.h` were removed in favor of `lib_deps` (same GitHub source
`toy-microwave` already uses, for the same reason — the PlatformIO registry re-registration has
an unversioned alpha release). `U8glib` is pulled from the registry directly.

**Known, not fixed:** `U8glib` (`olikraus/U8glib`) is an older, no-longer-actively-maintained
library. Building against it produces 3 benign assembler warnings
(`ignoring changed section attributes for .progmem.data`) and 30 `cppcheck` low-severity style
notes — all inside `U8glib.h` itself, none in this project's own code (confirmed via `pio check`).
Not something to fix here; noted so it isn't mistaken for a defect introduced by this rewrite.

## BOM

| Role | Part |
|---|---|
| MCU | Arduino Nano (ATmega328) |
| Gameplay display | SH1106 128x64 OLED panel (I2C) |
| Score display | TM1637 4-digit 7-segment |
| Input | Rotary encoder module (only its pushbutton is used) |
| Sound | Passive buzzer |

## Pin table

| Function | Pin | Note |
|---|---|---|
| Encoder CLK | D2 | wired, never read (see Audit #5) |
| Encoder SW (button) | D3 | interrupt-driven, `FALLING` |
| Encoder DT | D4 | wired, never read (see Audit #5) |
| Score display DIO | D5 | |
| Score display CLK | D6 | |
| Buzzer | D10 | |
| OLED SDA / SCL | A4 / A5 | I2C, addressed by `U8glib` |

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/game-dino-run
pio run                 # build
pio run -t upload       # build + flash the Nano
pio test -e native      # off-device unit tests
pio device monitor      # serial monitor
```

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` are included (rotary encoder, TM1637 7-segment, buzzer, and an
OLED). Build (`pio run`), then **F1 → "Wokwi: Start Simulator"**. Press the encoder's button to
start, jump, and restart.

**Pre-existing, unverified mismatch, inherited as-is:** the diagram simulates a `board-ssd1306`
part, but `main.cpp` addresses it with `U8GLIB_SH1106_128X64` — a driver for a different
(if related) controller chip. This mismatch predates this rewrite; the original diagram's author
isn't credited as the same person as the firmware's original authors, so it's plausible the real
hardware is genuinely SH1106-based and the simulated SSD1306 was just the closest part Wokwi
offers. I haven't confirmed pixel-perfect compatibility between the two drivers — if the OLED
image looks off by a few pixels in simulation, that mismatch is why.

## Status

Rewritten and audited; all 24 native unit tests pass; not yet verified against real hardware.
