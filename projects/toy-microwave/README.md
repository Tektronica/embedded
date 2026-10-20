# Toy Microwave

A toy microwave oven control panel. It runs a cook countdown or a kitchen timer on a 4-digit
display, shows the time of day while idle, and takes input from a 16-key matrix. A UMW ET6226M
chip drives the display and scans the keys over one two-wire bus. The firmware also drives a
buzzer and switches logic-level control lines for an off-board turntable motor, fan, and light.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/Debounce`, `lib/arduino/ET6226M`, `lib/arduino/SevenSeg`, `lib/arduino/ToneSequence`, `lib/arduino/WallClock` |
| Other libraries | None |
| Custom PCB | `hw/` in KiCad 10 |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity native tests through `env:native`.
- KiCad 10 for the board, and `tools/release.sh` to regenerate its outputs.

## Getting started

Run these from the repo root. If `pio` isn't on your shell `PATH` (VS Code-only installs), add it
first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d projects/toy-microwave/fw`
2. Flash the Nano: `pio run -d projects/toy-microwave/fw -t upload`. Auto-reset is unreliable on
   this CH340 board, so tap RESET as the upload starts.
3. Run the native tests: `pio test -d projects/toy-microwave/fw -e native`
4. Regenerate BOM, fab, and review outputs: `tools/release.sh projects/toy-microwave/hw`

## Hardware

### BOM

From `hw/bom/microwave-bom.csv`. The Nano module is not in the exported BOM.

| Reference | Qty | Value | Part | Role |
| --- | --- | --- | --- | --- |
| U5 | 1 | ET6226M | UMW ET6226M, SOP-16 | Display driver and key scanner |
| U2 | 1 | CC56-12SURKWA | Kingbright CC56-12SURKWA | 4-digit 7-segment display, common cathode |
| D1-D16 | 16 | 1N4148W | Diodes Inc 1N4148W-7-F | One diode per key in the matrix |
| BZ1 | 1 | Buzzer | Soberton WT-1205 | Tones, key feedback, running hum |
| Q2 | 1 | AO3400A | Alpha & Omega AO3400A, SOT-23 | Low-side buzzer switch |
| D17, D18 | 2 | SS14 | Vishay SS14-E3/61T | `D18` clamps the buzzer's turn-off spike to `+5V` |
| U1 | 1 | AP63205WU | Diodes Inc AP63205WU-7 | 5 V buck regulator |
| L1 | 1 | 4.7 uH | Murata LQH55DN4R7M03L | Buck inductor |
| C1, C2 | 2 | 100 pF | KEMET C0805C101J1GAC7210 | |
| C3 | 1 | 1 uF | KEMET C0805C105K3RACAUTO | |
| C4 | 1 | 10 uF | Taiyo Yuden TMK325AB7106MMHPR | |
| C5 | 1 | 100 nF | KEMET C0805C104M5RACAUTO | |
| C6, C7 | 2 | 22 uF | Samsung CL32A226MAJNNNE | |
| R1-R4 | 4 | 2 kΩ | Yageo RC0805FR-072KL | |
| R5, R6, R9 | 3 | 100 Ω | Yageo RC0805FR-07100RL | `R9` is the buzzer MOSFET gate resistor |
| R7, R8, R10 | 3 | 4.7 kΩ | Yageo RC0805FR-074K7L | |
| R11-R13 | 3 | 10 kΩ | Yageo RC0805FR-0710KL | Pull-downs on the motor, fan, and light control lines |
| J1, J2 | 2 | 4-pin screw terminal | Phoenix Contact 1715747 | `J2` carries the peripheral control lines |

The display must be common cathode, because the ET6226M's SG pins source current and its GR pins
sink it. The CC56-12 is pin-for-pin with the CA56-12 footprint. The 16 keys (`MX1`-`MX16` in the
schematic) are Cherry MX-style switches.

The turntable motor, cooling fan, and interior light are off-board and not on this BOM. The board
only exposes their logic-level control lines on `J2`. The planned parts are a TYC-50 12 V
synchronous turntable motor (5 to 6 RPM), a 12 V 40 x 40 x 10 mm DC fan (about 0.08 A), and an
80 mm white COB LED ring rated for 12 V or 24 V.

### Pins

From `fw/src/main.cpp`.

| Function | Pin | Note |
| --- | --- | --- |
| ET6226M CLK | D8 | |
| ET6226M DAT | D9 | |
| Buzzer | D6 | Through `Q2` |
| Light control | D10 | `J2` pin 1. Not D13, because the bootloader blinks D13's LED at reset |
| Motor control | D11 | `J2` pin 2 |
| Fan control | D12 | `J2` pin 3 |
| GND | | `J2` pin 4 |

Each control line has a 10 kΩ pull-down (`R11`-`R13`), so it reads low through reset. The lines
are logic-level signals for off-board boards that own their own driver and power. Do not connect a
12 V load to `J2` directly.

## How it works

`Microwave.h` holds the state machine: Idle, Setting, Running, Done, and ClockSet. It reacts to
abstract events (`Digit`, `Start`, `Cancel`, `Clock`, `Timer`, `AddThirty`, `Mute`, `Tick`), not
to keypad wiring. Setting, Running, and Done serve two functions, `CookTime` and `Timer`, through
one shared flow. `isCooking()` is true only for Running with `CookTime`, and `main.cpp` gates the
motor, fan, light, and hum on it.

### States

- Idle: shows the time of day as HH:MM in 12-hour form, with no AM or PM indicator. A digit key
  starts a cook time, `B` starts a kitchen timer, and `A` starts setting the clock.
- Setting: digits enter calculator-style as MM:SS. Each press shifts the existing digits left, so
  `3`, `30`, `300` show `0:03`, `0:30`, `3:00`. `#` starts the countdown if the time is nonzero.
  `*` returns to Idle.
- Running: counts down once per second. A cook turns on the motor, fan, light, and hum. A kitchen
  timer runs with all of them off. `*` cancels either one.
- Done: the display shows "End" with no colon. The buzzer plays 4 beeps and repeats them every
  10 s until a key is pressed. The display blanks between beeps, in sync with the buzzer, and stays
  lit during the 10 s gap. Any key returns to Idle.
- ClockSet: enter the time in 24-hour form, for example `1930` for 7:30 PM. The display flashes
  with a 1 s period the whole time. The preview clamps hours to 23 and minutes to 59. `#` accepts
  only a valid time, so `2400` is refused and entry continues. `*` discards the entry.

The clock keeps running in every state except ClockSet. It is a software clock in
`lib/arduino/WallClock`, so it resets to 0:00 on every power loss or reset.

### Keys

| Key | Action |
| --- | --- |
| `0`-`9` | Enter a digit |
| `#` | Start the countdown, or confirm the clock time |
| `*` | Cancel and return to Idle |
| `A` | From Idle, set the clock |
| `B` | From Idle, enter a kitchen timer |
| `C` | From Idle, start a 30 s cook. While Running, add 30 s, capped at 99:59 |
| `D` | Toggle mute in any state. Sound is on at power-up |

The key matrix uses `SG1`-`SG4` as rows and `GR1`-`GR4` as columns, sharing the display's segment
and grid lines. The `LAYOUT` table in `translateKey()` maps each scan position to a legend. A
different keycap arrangement only needs that table changed, not a rewire.

### Firmware modules

- `fw/include/Microwave.h`: the state machine. It delegates time-of-day keeping and countdown
  counting to `wallclock::Clock` and `wallclock::Timer`.
- `fw/include/Buzzer.h`: key press, done (4 beeps), error, and the continuous hum, as
  `{frequency, duration}` step tables played by `lib/arduino/ToneSequence`. `isFinished()` waits
  for the whole sequence.
- `lib/arduino/ET6226M`: the bit-banged CLK/DAT driver and its hardware-free codec. No Arduino
  library exists for this chip. `examples/arduino-nano/display-keyscan-et6226m` shares the same
  driver.
- `lib/arduino/Debounce`: turns the chip's raw key reading into one fresh press.
- `lib/arduino/SevenSeg`: the digit and text font, `secondsToDigits()`, and `blinkOn()`.
- `fw/src/main.cpp`: key read, debounce, decode, event dispatch, buzzer pattern selection,
  motor, fan, and light outputs, and display rendering.

The driver runs in `EightSegment` mode, because the display's shared DP line connects straight to
the ET6226M's `DP/KP` pin. The colon is the DP bit on grid `COLON_DIGIT_INDEX` (1).

`examples/arduino-nano/toy-microwave-tm1637` is the earlier version, with a TM1637 display driver
and a keypad scanned directly by the MCU. It is frozen as a reference.

## Folders

- `fw/`: firmware, a PlatformIO project with app headers in `include/` and native tests in
  `test/`.
- `hw/`: the board design package. See `docs/hw/README.md` for the layout. It also
  holds the Onshape enclosure template in `hw/mech/enclosure-template.dxf`.
- `docs/`: design notes. `buck-converter-5v/` covers the 5 V rail, and `display-driver/` holds
  the ET6226M wiring diagram.

## Known issues

- No Wokwi simulation. Wokwi has no ET6226M part.
- The clock resets on every power cycle. Keeping time across one needs a battery-backed RTC.
- Idle shows 12-hour time with no AM or PM indicator.

## Status

The rev 1 schematic and board are in `hw/`. The firmware passes 38 native tests (31 in
`test_microwave`, 7 in `test_buzzer`) but has not run on the real board yet.
