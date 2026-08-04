# arduino-nano/toy-microwave-et6226m

Arduino Nano firmware for a toy/prop microwave oven control panel: a countdown timer on a 4-digit
display, a turntable motor, a done/alert buzzer, a cooling fan and buzzer-tone layer for the
running "hum," an interior light, and a matrix keypad for setting time, start/stop, and the
time-of-day clock shown while idle. Display rendering and keypad scanning both go through a
single UMW ET6226M chip over one two-wire bus, rather than a separate display driver chip plus a
bit-banged matrix scan — see `arduino-nano/toy-microwave-tm1637` for that earlier approach, frozen
as a reference once this project moved to the ET6226M.

## BOM

| Qty | Role (generic) | Part chosen | Notes |
|---|---|---|---|
| 1 | 4-digit 7-segment display | Kingbright CA56-12EWA, common anode | Per `KiCad/microwave`'s schematic (`U2`). Driven entirely through the ET6226M -- no separate display driver chip. |
| 1 | Display + keypad-scan driver | UMW ET6226M (SOP-16) | Per the schematic (`U5`). Two-wire CLK/DAT to the MCU; drives the display's 4 digits and scans the 16-key matrix, sharing its GR/SG lines with the display per the schematic's `SEG_x/ROWn` and `DIG_CAn/COLn` nets. Runs in `EightSegment` mode, since `DP/KP` is wired directly to the display's shared DP line, not into the key matrix. |
| 16 | Keyswitches | Cherry MX-style (`MX1`-`MX16` in the schematic) | Diode-per-key matrix (`D1`-`D16`), `SG1-4` as rows / `GR1-4` as columns -- the same 4x4 layout and key legends (`1-9,0,*,#,A-D`) as `toy-microwave-tm1637`'s membrane keypad, just wired through the ET6226M instead of directly to the MCU. |
| 1 | Buzzer (done/alert tone, button feedback, running hum) | 12085 buzzer, 12×8.5mm, 42Ω, 3–12V | Sold in a 20-pack. Passive — driven via `tone()`/PWM; also layers a quiet low-frequency tone during Running, alongside the fan and motor, for the ambient hum. |
| 1 | Turntable motor | TYC-50 synchronous motor, 12V DC, 5–6 RPM, CW/CCW, 4W | Matches a real microwave turntable's rotation speed; its own running sound also contributes to the hum. |
| 1 | Cooling fan (running hum) | Generic 12V DC brushless fan, 40×40×10mm, sleeve bearing, ~0.08A | A commodity spec, not a specific listing — any 12V 40mm fan matches. Matches the 12V rail already used by the motor/light rather than introducing a separate 5V domain. Needs a transistor/MOSFET driver, same as the motor — not a direct GPIO connection. |
| 1 | Interior/status light | 80mm white COB LED ring ("angel eye" halo ring) | Sold in a 2-pack. 12V/24V rated — confirm current draw and how it's switched (relay/MOSFET vs. direct GPIO) before wiring. |

## Firmware

Self-contained PlatformIO project (`platformio.ini`, `include/`, `src/`, `test/`), fully wired up:

- **`SevenSegment.h`** — display-value formatting (a total-seconds-style value → two digit-pair values) and a blink-cycle helper, used to blank the whole display while ClockSet. Shared by both the cook-timer's MM:SS and the clock's HH:MM, since the digit-pair math is identical regardless of what the two halves mean. Unchanged from the TM1637 approach — this logic was never chip-specific, only how the resulting digits get rendered was.
- **`Buzzer.h`** — tone/frequency selection and beep-pattern timing (key press, done — 4 beeps, error) plus the continuous Running hum, and `isFinished()` to know when a one-shot pattern's whole sequence (not just its current on/off phase) has completed.
- **`ET6226MCodec.h`** — hardware-free (unit-tested via `pio test -e native`): `encodeDigit()` (digit 0-9 → segment byte), `encodeDisplayControl()` (brightness + on/off → the Display Control Command's data byte), and `decodeKeyCode()` (raw key code → which grid/segment scan line produced it). Copied from `arduino-nano/display-keyscan-et6226m`, the standalone project this driver was proven out in first.
- **`ET6226M.h`** — the `ET6226M` driver class: hardware-coupled (bit-banged CLK/DAT), not unit-tested. Also copied from `arduino-nano/display-keyscan-et6226m` — the driver needed zero changes to work here, which is the point of keeping it centered on the chip's own model (grids, segments, key codes) rather than on anything toy-microwave-specific.
- **`KeyDebounce.h`** — debounces the ET6226M's raw key readings into a settled, fresh-press-only event, the same N-consistent-scan pattern the old `KeyMatrix.h` used for the bit-banged matrix. Mechanical switch bounce doesn't go away just because a smarter chip is doing the scanning, so this debounce layer is still needed; unlike `KeyMatrix.h`, there's no layout lookup here since the ET6226M already resolves which grid/segment was pressed — that lookup lives in `main.cpp`'s `translateKey()` instead.
- **`Clock.h`** (namespace `wallclock`, to avoid shadowing the C standard library's `clock()`) — generic, hardware-free time-keeping building blocks, normalized to stay reusable outside this one project: `Clock`, a time-of-day counter (seconds since midnight, advanced by `tick()`); `Timer`, a generic countdown (`start(seconds)`/`tick()`/`cancel()`/`remaining()`/`isRunning()`) that knows nothing about what happens at zero — same trio a real RTC chip (e.g. DS3231) typically exposes (clock, timer, alarm — the last not added until something actually needs it); and digit-entry helpers split into small, composable pieces rather than one clock-specific decoder — `shiftEnteredDigit`/`splitDigits` are generic 4-digit-buffer mechanics (also used by `Microwave.h`'s MM:SS entry, so the shift logic isn't duplicated across the two), while `isValidTime(hour, minute, TimeFormat)` and `to12Hour` are the time-specific policy layered on top (`TimeFormat::H24`/`H12` for validating either clock convention, even though only H24 is wired up live today). No RTC chip is in the BOM, so `Clock` is a plain software clock that resets to 0:00 on every power loss or reset — fine for a toy/prop build; a battery-backed RTC would be needed for it to survive a power cycle.
- **`Microwave.h`** — the app-level orchestrator: the Idle → Setting → Running → Done countdown state machine (plus `ClockSet`, reachable from Idle), driven by abstract `Event`s (`Digit`/`Start`/`Cancel`/`Clock`/`Timer`/`Tick`) rather than any specific keypad wiring. Setting/Running/Done serve two `Function`s — `CookTime` and `Timer` (a plain kitchen timer) — sharing one flow rather than duplicating it, since digit entry, Start/Cancel, and counting down to Done are identical either way; `isCooking()` (`Running` + `Function::CookTime`) is the only thing that distinguishes them, and it's what `main.cpp` gates the motor/fan/light/hum on. Delegates time-of-day keeping and countdown-counting to `Clock.h`'s `wallclock::Clock`/`wallclock::Timer` rather than absorbing either concern directly — `Microwave.h` is the main app, `Clock.h` is a service it composes. Unchanged by the ET6226M migration — this state machine never touched the display or keypad hardware directly.
- **`main.cpp`** — reads the ET6226M's key code → debounces → decodes to grid/segment → `Event` → state machine → buzzer pattern selection, motor/fan/light on/off, and ET6226M display rendering (colon included, via the DP bit on `COLON_DIGIT_INDEX`'s grid -- the same trick `toy-microwave-tm1637` used with TM1637's DP bit).
- Motor, fan, and light have no dedicated header — all three are plain on/off hardware glue in `main.cpp`.

### Behavior

- **Idle** — displays the live time-of-day clock (HH:MM, 12-hour — the clock is kept internally as 24-hour and converted for display; no AM/PM indicator on this display, so e.g. 8:15 looks the same whether it's AM or PM). A digit key starts entering a cook time; `B` starts entering a kitchen timer; `A` starts entering a new clock time.
- **Setting** — entering a countdown digit-by-digit (MM:SS, calculator-style: each press appends to the low end and shifts existing digits left, e.g. `3` → `0:03`, `30` → `0:30`, `300` → `3:00`), for whichever `Function` was selected to get here (`CookTime` via a digit key, `Timer` via `B`). `#` starts the countdown if a nonzero time is entered; `*` cancels back to Idle.
- **Running** — counts down once per second. If this countdown is `CookTime`, the motor, fan, and light are on, plus the ambient hum; a `Timer` (kitchen timer) countdown runs silently in the background with none of that hardware on. `*` cancels back to Idle at any point, ending a cook or a timer the same way.
- **Done** — countdown reached 0:00; buzzer plays a 4-beep done pattern, whether it was a cook or a kitchen timer, and repeats every 10 seconds as a reminder for as long as Done goes unacknowledged. The display flashes in exact sync with each beep of that pattern — dark whenever the buzzer is silent between beeps, lit whenever it's sounding — and stays steady during the quiet gap between reminders. Any key returns to Idle.
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
assigned speculatively now. `main.cpp`'s `translateKey()` maps each key legend onto its confirmed
`(grid, segment)` scan position (`KiCad/microwave`'s schematic: `SG1-4`=rows, `GR1-4`=columns) --
this LAYOUT table is the only place a different physical keycap arrangement would need updating;
the electrical wiring stays the same regardless of which legend is assigned where.

**Driver**: `ET6226M.h`/`ET6226MCodec.h`, a from-scratch driver (no Arduino library exists for
this chip) — no `lib_deps` entry needed. The colon (`COLON_DIGIT_INDEX`/`COLON_BIT` in `main.cpp`)
uses the same DP-bit-on-one-grid trick `toy-microwave-tm1637` used with TM1637, confirmed against
the schematic rather than guessed — the Kingbright display's shared DP line is wired straight to
the ET6226M's `DP/KP` pin (`EightSegment` mode).

Pin assignments: ET6226M `CLK`/`DAT` on D2/D3, buzzer D10, motor D11, fan D12, light D13. The old
TM1637 approach's separate matrix row/column pins (D2-D9) are gone entirely — the keypad is
scanned by the ET6226M, not the MCU.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/toy-microwave-et6226m
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

## Simulate (Wokwi)

Not available — the ET6226M isn't a part Wokwi supports.

## Status

Schematic exists (`KiCad/microwave`): ET6226M-driven display and 16-key diode matrix, ATmega328PB
MCU. Firmware logic is implemented and tested (74 unit tests across seven headers) against that
schematic, but not yet run on real hardware.
