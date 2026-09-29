# arduino-nano/led-dimmer-4ch

A 4-channel potentiometer-controlled LED dimmer board: Arduino Nano + 4× PT4115 constant-current
buck LED drivers, powered from 12V (Nano runs off 5V via an onboard AP63205 buck regulator, or
from its own USB port). Each channel is one off-board pot (A0–A3, wired in through `J4`/`J5`) →
one PWM output (D9, D10, D3, D11) → one PT4115's DIM pin. USB alone powers the Nano only: the
PT4115s need at least 6V on VIN.

## PWM pins (ATmega328P)

The Nano's 6 PWM-capable pins split across 3 timers, each with its own default `analogWrite()`
frequency:

| Timer | Pins | Frequency |
|---|---|---|
| Timer0 | D5, D6 | ~980 Hz (also drives `millis()`/`delay()`) |
| Timer1 | D9, D10 | ~490 Hz |
| Timer2 | D3, D11 | ~490 Hz |

This board uses D9, D10, D3, D11 (Timer1 + Timer2), skipping D5/D6 so all 4 channels share one
common ~490 Hz frequency with zero timer register configuration.

## Folders

- **`KiCad/`** — schematic + PCB (KiCad 7+). The 5V buck block (`U5`, `C5`-`C8`, `D5`, `L5`) is
  the same layout as `toy-microwave-et6226m`'s, mirrored to the front side.
- **`datasheets/`** — reference PDFs for the key ICs (PT4115 LED driver, connector). The LM2596
  PDFs are from the board's earlier buck regulator. The AP63205's lives in
  `toy-microwave-et6226m/docs/buck-converter-5v/`.
- **`fw/`** — firmware, a self-contained PlatformIO project. Includes a Wokwi diagram substituting
  plain LED+resistor pairs for the PT4115 modules, to verify the 4-channel PWM/pot correspondence
  without real hardware.
