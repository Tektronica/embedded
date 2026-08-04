# arduino-nano/toy-microwave-tm1637

Arduino Nano firmware for a toy/prop microwave oven control panel: a countdown timer on a 4-digit
display, a turntable motor, a done/alert buzzer, a cooling fan and buzzer-tone layer for the
running "hum," an interior light, and a 4x4 matrix keypad for setting time, start/stop, and the
time-of-day clock shown while idle.

## BOM

| Qty | Role (generic) | Part chosen | Notes |
|---|---|---|---|
| 1 | 4-digit 7-segment display (countdown timer) | WWZMDiB 4-Digit 7-Segment LED Display Board for Arduino | Sold in a 5-pack. TM1637 driver chip — 2-wire CLK/DIO protocol, only 2 Nano pins needed. |
| 1 | Buzzer (done/alert tone, button feedback, running hum) | 12085 buzzer, 12×8.5mm, 42Ω, 3–12V | Sold in a 20-pack. Passive — driven via `tone()`/PWM; also layers a quiet low-frequency tone during Running, alongside the fan and motor, for the ambient hum. |
| 1 | Turntable motor | TYC-50 synchronous motor, 12V DC, 5–6 RPM, CW/CCW, 4W | Matches a real microwave turntable's rotation speed; its own running sound also contributes to the hum. |
| 1 | Cooling fan (running hum) | Generic 12V DC brushless fan, 40×40×10mm, sleeve bearing, ~0.08A | A commodity spec, not a specific listing — any 12V 40mm fan matches. Matches the 12V rail already used by the motor/light rather than introducing a separate 5V domain. Needs a transistor/MOSFET driver, same as the motor — not a direct GPIO connection. |
| 1 | Interior/status light | 80mm white COB LED ring ("angel eye" halo ring) | Sold in a 2-pack. 12V/24V rated — confirm current draw and how it's switched (relay/MOSFET vs. direct GPIO) before wiring. |
| 1 | Keypad (time entry, start/stop) | Generic 4x4 matrix membrane keypad (16-key, `1-9,0,*,#,A-D` layout) | Chosen as a matrix (not individually-wired mechanical switches) to keep the pin count to 8 (4 rows + 4 cols) alongside the display/motor/fan/light. Mechanical keyswitches (Cherry MX or similar) can still sit under a matrix if tactile feel matters — the matrix wiring is the same either way. |

## Firmware

Self-contained PlatformIO project (`platformio.ini`, `include/`, `src/`, `test/`), fully wired up:

- **`SevenSegment.h`** — display-value formatting (a total-seconds-style value → two digit-pair values) and a blink-cycle helper, used to blank the whole display while ClockSet. Shared by both the cook-timer's MM:SS and the clock's HH:MM, since the digit-pair math is identical regardless of what the two halves mean.
- **`Buzzer.h`** — tone/frequency selection and beep-pattern timing (key press, done — 4 beeps, error) plus the continuous Running hum, and `isFinished()` to know when a one-shot pattern's whole sequence (not just its current on/off phase) has completed.
- **`KeyMatrix.h`** — debounced key detection across a 4x4 matrix, using the same debounce pattern as `arduino-nano/led-dimmer-ws2812`'s `Button` class generalized to a 16-way key index. Named to avoid colliding with the well-known Arduino `Keypad` library.
- **`MatrixScanner.h`** — the generic row/column GPIO scan technique (drive each row low, read back the active column), separate from `KeyMatrix.h`'s debouncing/layout lookup. Hardware-coupled, not unit-tested — same shape as `stepper`'s `Stepper.h` and `robot-buoy`'s `Radio.h`. Identical technique, standalone with its own tests, lives at `arduino-nano/keypad`.
- **`Clock.h`** (namespace `wallclock`, to avoid shadowing the C standard library's `clock()`) — generic, hardware-free time-keeping building blocks, normalized to stay reusable outside this one project: `Clock`, a time-of-day counter (seconds since midnight, advanced by `tick()`); `Timer`, a generic countdown (`start(seconds)`/`tick()`/`cancel()`/`remaining()`/`isRunning()`) that knows nothing about what happens at zero — same trio a real RTC chip (e.g. DS3231) typically exposes (clock, timer, alarm — the last not added until something actually needs it); and digit-entry helpers split into small, composable pieces rather than one clock-specific decoder — `shiftEnteredDigit`/`splitDigits` are generic 4-digit-buffer mechanics (also used by `Microwave.h`'s MM:SS entry, so the shift logic isn't duplicated across the two), while `isValidTime(hour, minute, TimeFormat)` and `to12Hour` are the time-specific policy layered on top (`TimeFormat::H24`/`H12` for validating either clock convention, even though only H24 is wired up live today). No RTC chip is in the BOM, so `Clock` is a plain software clock that resets to 0:00 on every power loss or reset — fine for a toy/prop build; a battery-backed RTC would be needed for it to survive a power cycle.
- **`Microwave.h`** — the app-level orchestrator: the Idle → Setting → Running → Done countdown state machine (plus `ClockSet`, reachable from Idle), driven by abstract `Event`s (`Digit`/`Start`/`Cancel`/`Clock`/`Timer`/`Tick`) rather than any specific keypad wiring. Setting/Running/Done serve two `Function`s — `CookTime` and `Timer` (a plain kitchen timer) — sharing one flow rather than duplicating it, since digit entry, Start/Cancel, and counting down to Done are identical either way; `isCooking()` (`Running` + `Function::CookTime`) is the only thing that distinguishes them, and it's what `main.cpp` gates the motor/fan/light/hum on. Delegates time-of-day keeping and countdown-counting to `Clock.h`'s `wallclock::Clock`/`wallclock::Timer` rather than absorbing either concern directly — `Microwave.h` is the main app, `Clock.h` is a service it composes.
- **`main.cpp`** — matrix scan → debounced key → `Event` → state machine → buzzer pattern selection, motor/fan/light on/off, and TM1637 display rendering.
- Motor, fan, and light have no dedicated header — all three are plain on/off hardware glue in `main.cpp`.

### Behavior

- **Idle** — displays the live time-of-day clock (HH:MM, 12-hour — the clock is kept internally as 24-hour and converted for display; no AM/PM indicator on this display, so e.g. 8:15 looks the same whether it's AM or PM). A digit key starts entering a cook time; `B` starts entering a kitchen timer; `A` starts entering a new clock time.
- **Setting** — entering a countdown digit-by-digit (MM:SS, calculator-style: each press appends to the low end and shifts existing digits left, e.g. `3` → `0:03`, `30` → `0:30`, `300` → `3:00`), for whichever `Function` was selected to get here (`CookTime` via a digit key, `Timer` via `B`). `#` starts the countdown if a nonzero time is entered; `*` cancels back to Idle.
- **Running** — counts down once per second. If this countdown is `CookTime`, the motor, fan, and light are on, plus the ambient hum; a `Timer` (kitchen timer) countdown runs silently in the background with none of that hardware on. `*` cancels back to Idle at any point, ending a cook or a timer the same way.
- **Done** — countdown reached 0:00; the display shows "End" (no colon) instead of a static "0:00", whether it was a cook or a kitchen timer. Buzzer plays a 4-beep done pattern and repeats every 10 seconds as a reminder for as long as Done goes unacknowledged. The display flashes in exact sync with each beep of that pattern — dark whenever the buzzer is silent between beeps, lit whenever it's sounding — and stays steady during the quiet gap between reminders. Any key returns to Idle.
- **ClockSet** — entering a new time-of-day, HH:MM, same calculator-style digit entry as a countdown, typed in 24-hour form (e.g. `1930` for 7:30 PM). The whole display flashes for as long as ClockSet is active, so editing the clock is visually distinct from just reading it. While typing, the live preview clamps hours to 23 and minutes to 59; `#` only confirms and returns to Idle if the entered hour:minute is actually a valid 24-hour time (e.g. `2400` is refused, not silently clamped to 23:00) — otherwise it's a no-op and entry continues. `*` cancels, discarding the entry and leaving the clock unchanged.
- The clock keeps advancing in the background in every state except while it's actively being set (cooking or timing doesn't stop the time of day).

### 4x4 keypad mapping

| Key(s) | Action |
|---|---|
| `0`-`9` | Enter a digit (cook time or kitchen timer while Idle/Setting, clock time while ClockSet) |
| `#` | Start / confirm (starts the countdown from Setting; confirms the new clock time from ClockSet) |
| `*` | Cancel (back to Idle, discarding whatever was being entered, or ending a running countdown) |
| `A` | Clock — from Idle, begin setting the time-of-day clock |
| `B` | Timer — from Idle, begin entering a plain kitchen timer (counts down and beeps; doesn't turn on the motor/fan/light) |
| `C`, `D` | Reserved, unused for now |

`C`/`D` are left open for future features (e.g. a light toggle or a child-lock) rather than
assigned speculatively now.

**Display library**: `avishorp/TM1637` pulled directly from GitHub (see `platformio.ini`'s
`lib_deps` — the PlatformIO registry re-registration has an odd unversioned release, so this
points at the actual upstream repo instead). The colon segment's exact bit position
(`COLON_DIGIT_INDEX`/`COLON_BIT` in `main.cpp`) is a common-board guess, not yet confirmed against
this specific display — check it once the display is wired up and adjust if the colon doesn't light.

Pin assignments (placeholder pending a real schematic): matrix rows D9–D6, columns D5–D2, buzzer
D10, motor D11, fan D12, light D13, display CLK A0 / DIO A1.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/toy-microwave-tm1637
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

## Simulate (Wokwi)

`wokwi.toml` + `diagram.json` cover the keypad, display, buzzer, and motor/fan/light LEDs, plus a
text label next to the keypad spelling out what `A`/`B`/`#`/`*` do (digits are self-explanatory).
Build first (`pio run`), then **F1 → "Wokwi: Start Simulator"** and click the matrix keypad's keys
to drive the state machine and watch the countdown on the display.

## Status

BOM set. No hardware design yet. Firmware logic is implemented and tested (64 unit tests across
the five headers) and wired end-to-end, including the TM1637 display.

**Retired.** The live project is pivoting to the UMW ET6226M, which drives the display and scans
the keypad through one chip instead of a separate TM1637 + 4x4 matrix keypad — see
`arduino-nano/toy-microwave-et6226m/`. This project is frozen as the complete, working reference
for the original hardware approach, not under active development.
