# arduino-nano/display-keyscan-et6226m

> Arduino Nano firmware demonstrating a from-scratch driver for the UMW ET6226M: a single chip
> that drives a 4-digit 7-segment display and scans a 7x4 keyboard matrix over one two-wire
> CLK/DAT bus.

This project lives at `arduino-nano/display-keyscan-et6226m/` in the `embedded` monorepo.

## Why this exists

`arduino-nano/toy-microwave-tm1637` drove its display via a TM1637 chip and scanned its keypad
separately via bit-banged row/column GPIO (`KeyMatrix.h`/`MatrixScanner.h`). The ET6226M replaces
both with one chip and one bus, so this project exists to prove out the `ET6226M` driver class on
its own before wiring it into `arduino-nano/toy-microwave-et6226m`'s state machine — the same
relationship `arduino-nano/keypad` has to the matrix-keypad technique it demonstrates standalone.

## What it does

Counts 0000-9999 on the display, advancing once every 500ms, and prints any pressed key's
grid/segment over Serial whenever the chip's reported key code changes. `begin()` sets full
brightness and turns the display on; nothing in the demo currently exercises `setBrightness()`/
`setDisplayOn()` beyond that initial call, since proving the driver's API is enough for this
project's purpose.

## Design

- **`include/ET6226MCodec.h`** — hardware-free (unit-tested via `pio test -e native`):
  `encodeDigit()` (digit 0-9 → segment byte), `encodeDisplayControl()` (brightness + on/off →
  the Display Control Command's data byte), and `decodeKeyCode()` (raw key code → which
  grid/segment scan line produced it, reversing the datasheet's Key Code Command table).
- **`include/ET6226M.h`** — the `ET6226M` driver class: hardware-coupled (bit-banged CLK/DAT), not
  unit-tested. A from-scratch driver, not a third-party library wrapper — none exists for this
  chip. The two-wire framing (start/stop conditions, byte+ACK) is shaped like TM1637's, but the
  bit order (MSB-first here, LSB-first on TM1637) and command set are different, so
  `TM1637Display` couldn't be reused. Its API stays centered on the chip's own model (grids 1-4,
  raw segment bytes, raw key codes, brightness/on-off) rather than on anything about what's
  consuming it — that separation is deliberate, not an oversight.
- **`src/main.cpp`** — the minimal demo described above.

## Datasheet notes

Built first from a partial excerpt, then the full 14-page datasheet (UMW/UTD Semiconductor,
Jul.2025). What the full datasheet confirmed or added:

- **Display Control Command (`0x48`)** exists: one data byte packs brightness (3 bits, 0=dimmest
  "1 step" .. 7=brightest "8 step"), a 7-segment/8-segment mode select, sleep mode, and display
  on/off. Reconstructed from the datasheet's own worked examples (`X1H`=8-segment mode,
  `X9H`=7-segment mode, `04H`=sleep mode, and "D0 and D2 cannot be 1 at the same time").
  `encodeDisplayControl()`/`setBrightness()`/`setDisplayOn()` implement brightness and on/off;
  sleep mode (D2) is identified but not exposed, since nothing needs it yet.
- **No auto-increment addressing** — confirmed. The Display Data Command table only has the 4
  fixed grid addresses used today; `setGrid()`'s one-transaction-per-grid design was correct.
- **Key code reads return one key only**, not a full matrix scan — confirmed from the Key Code
  Command's single-byte format.
- **DAT's pull-up is real but weak** — the Electrical Characteristics table gives a "DAT pin
  input pull-up current" in the tens-of-µA range, confirming the "built-in drain mode" pin
  description means genuinely open-drain, and that the chip's own pull-up alone may be too weak
  for a fast, clean rise — the driver's `INPUT_PULLUP` release (rather than driving DAT high)
  adds the AVR's own pull-up in parallel rather than conflicting with it.
- **Bus timing** — the Interface Timing Parameter table gives minimums in the 100-400ns range
  (clock pulse width, data setup/hold). `BUS_DELAY_US = 3` (3000ns) is comfortably above these,
  though the table's exact column-to-parameter alignment didn't survive text extraction cleanly
  enough to trust precisely -- worth a direct look at the datasheet's own table/diagram rendering
  before tightening this delay for real.
- **7-segment mode and the colon** — the datasheet notes "when the circuit operate at 7 segment
  mode or 8 segment mode, DP/KP ports working condition is different, and need the peripheral
  circuit is also different." This driver fixes 7-segment mode (matching our 4-digit 7-segment
  display), which dedicates the single `DP/KP` pin to its `KP` role instead of a lit decimal
  point/colon. What `KP` actually does isn't established from the datasheet text (the pin
  description's "output for keyboard symbol" doesn't specify a function, and it's a separate pin
  from the SG1-7×GR1-4 key-scan matrix `readKeyCode()`/`decodeKeyCode()` actually use either way)
  — but it doesn't need to be: **the colon is handled by wiring an independent LED to its own
  spare GPIO pin, driven with a plain `digitalWrite()`, bypassing the chip's two-wire protocol
  entirely.** That sidesteps the 7-segment/8-segment trade-off rather than resolving it, and
  matches how `toy-microwave-tm1637` already treats the motor/fan/light — plain on/off GPIO glue
  with no dedicated header, not something wrapped into a chip driver class. This belongs in
  whatever `main.cpp` eventually orchestrates `toy-microwave-et6226m`, not in `ET6226M.h`, since
  the LED has nothing to do with this chip at all. One hardware caveat: this only works if the
  actual display module's colon LED is broken out as its own anode/cathode pair rather than
  hard-wired to share a digit's DP trace — worth checking once a specific display module is
  chosen.

Still genuinely open, not resolved by the full datasheet:

- **SG1-7 → segment (a-g) wiring** — which physical LED segment each SG pin drives is a function
  of the display board's wiring, not the chip itself; `encodeDigit()` assumes SG1=a...SG7=g, the
  common convention, but this needs confirming against the actual board once wired up — same kind
  of guess `toy-microwave-tm1637`'s `COLON_DIGIT_INDEX`/`COLON_BIT` needed to be confirmed.
- **Brightness step direction** — the datasheet lists "8 step (highest)" and "1 step (lowest)"
  without pinning down which binary value is which; `encodeDisplayControl()` assumes 0=dimmest,
  7=brightest. Low-risk if wrong (an easy one-line fix once observed on real hardware), but not
  yet verified.
- **Read-back ACK** — `readKeyCode()` reads the chip's returned byte without the master sending
  its own ACK/NACK afterward. The timing waveform section's "ACK/Key Back data" label suggests
  this is correct, but isn't fully unambiguous from the text alone.

## Quick start

Requires [PlatformIO](https://platformio.org/) (VSCode extension or CLI) — no Arduino IDE needed.
If `pio` isn't on your shell `PATH` (VSCode-extension-only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

```bash
cd arduino-nano/display-keyscan-et6226m
pio run                 # build
pio run -t upload       # build + flash the Nano
pio device monitor      # serial monitor
pio test -e native      # off-device unit tests
```

## Simulate (Wokwi)

Not available — the ET6226M isn't a part Wokwi supports.

## Status

Driver and demo written against the full datasheet; not yet run against real hardware. See
"Datasheet notes" above for what's still open once it is.
