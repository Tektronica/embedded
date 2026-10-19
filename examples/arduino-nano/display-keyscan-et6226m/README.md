# Display and Keyscan (ET6226M)

The example app for `lib/arduino/ET6226M`, a from-scratch driver for the UMW ET6226M. One ET6226M drives a 4-digit 7-segment display and scans a 7x4 key matrix over a single two-wire CLK/DAT bus. The demo counts 0000 to 9999 on the display and prints each pressed key over Serial. It proves the driver on its own, separate from `projects/toy-microwave`, which uses the same driver in its state machine.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz (`nanoatmega328`) |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/ET6226M`, `lib/arduino/SevenSeg` |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.

## Getting started

Run from the repo root. If `pio` is not on your shell `PATH` (VS Code extension only installs), add it first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d examples/arduino-nano/display-keyscan-et6226m`
2. Flash the Nano: `pio run -d examples/arduino-nano/display-keyscan-et6226m -t upload`
3. Open the serial monitor at 9600 baud: `pio device monitor -d examples/arduino-nano/display-keyscan-et6226m`

Auto-reset is unreliable on this CH340 board. Tap RESET as the upload starts. This example has no tests of its own. The codec and font are tested in their libraries: `pio test -d lib/arduino/ET6226M` and `pio test -d lib/arduino/SevenSeg`.

## Hardware

- Arduino Nano
- UMW ET6226M with a 4-digit 7-segment display and key matrix
- Datasheet: `docs/ET6226M.pdf`, pin diagram: `docs/et6226m-diagram.png`

| Signal | Nano pin | Notes |
| --- | --- | --- |
| ET6226M CLK | D8 | Bit-banged clock |
| ET6226M DAT | D9 | Open-drain data. Released with `INPUT_PULLUP` instead of driven high |

## How it works

`setup()` starts Serial at 9600 baud and calls `display.begin()`, which sets full brightness and turns the display on. The loop advances a counter every 500 ms, wrapping at 10000, and writes its four digits with `sevenseg::encodeDigit()` and `setSegments()`. It also reads the key code every pass. When the code changes and decodes to a real key, it prints `key grid=<n> segment=<n>`. The demo does not call `setBrightness()` or `setDisplayOn()` after `begin()`.

- `lib/arduino/ET6226M`, `ET6226MCodec.h`: hardware-free and unit-tested. `encodeDisplayControl()` packs brightness, on/off, and segment mode into the Display Control Command data byte. `decodeKeyCode()` maps a raw key code back to the grid and segment scan line that produced it.
- `lib/arduino/ET6226M`, `ET6226M.h`: the `ET6226M` driver class. Hardware-coupled, bit-banged CLK/DAT, no unit tests. No third-party library exists for this chip. The framing (start, stop, byte plus ACK) matches TM1637, but the ET6226M sends MSB first where TM1637 sends LSB first, and the command set differs, so `TM1637Display` cannot be reused. The API follows the chip's own model: grids 1 to 4, raw segment bytes, raw key codes, brightness, and on/off.
- `lib/arduino/SevenSeg`: the shared 7-segment font (`encodeDigit()`, `encodeChar()`, `encodeText()`). The ET6226M takes raw segment bytes, so the font lives in its own library.
- `src/main.cpp`: the demo above.

### Datasheet notes

The driver was first written from a partial excerpt, then checked against the full 14-page datasheet (UMW/UTD Semiconductor, Jul. 2025).

- Display Control Command (`0x48`): one data byte packs brightness (3 bits), a 7-segment or 8-segment mode select (D3), sleep mode (D2), and display on/off (D0). The datasheet puts "8 step" at `000` and "7 step" at `111`, so level N encodes as `(N + 1) mod 8`. This was reconstructed from the datasheet's worked examples (`X1H` is 8-segment mode, `X9H` is 7-segment mode, `04H` is sleep mode, and D0 and D2 cannot both be 1). The driver exposes brightness and on/off. Sleep mode is not exposed.
- Segment mode: `et6226m::SegmentMode` is a constructor argument on `ET6226M`, defaulting to `SevenSegment`. The mode depends on how a board wires DP/KP, so it is fixed at construction instead of toggled at runtime. This demo uses the default because it has no specific display board yet. `projects/toy-microwave` passes `EightSegment`, because its board ties DP/KP to the display module's shared DP line and covers its 16 keys with SG1 to SG4 by GR1 to GR4.
- Addressing: there is no auto-increment mode. The Display Data Command table lists only the 4 fixed grid addresses, so `setGrid()` sends one transaction per grid.
- Key reads: the Key Code Command returns one key, not a full matrix scan.
- DAT pull-up: the datasheet gives a DAT input pull-up current of 150 to 400 µA, which confirms the pin is open-drain with a weak internal pull-up. Releasing DAT with `INPUT_PULLUP` adds the AVR pull-up in parallel.
- Bus timing: the Interface Timing Parameter table gives minimums of 100 to 400 ns for clock pulse width and data setup and hold. `BUS_DELAY_US = 3` (3000 ns) is above all of them.

## Known issues

- Bus timing: the column alignment of the Interface Timing Parameter table did not survive text extraction. Check the rendered table before shortening `BUS_DELAY_US`.
- SG1 to SG7 segment mapping: `encodeDigit()` assumes SG1 is segment a through SG7 is segment g. The real mapping depends on the display board and needs confirming once one is wired.
- Read-back ACK: `readKeyCode()` reads the returned byte without sending a master ACK or NACK afterward. The datasheet's "ACK/Key Back data" waveform label suggests this is correct, but the text does not state it outright.
- Simulation: Wokwi has no ET6226M part, so this example has no simulator setup.

## Status

Driver and demo are written against the full datasheet. Not yet run on real hardware.
