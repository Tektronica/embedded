# arduino-nano/led-dimmer-4ch

A 4-channel potentiometer-controlled LED dimmer board: Arduino Nano + 4× PT4115 constant-current
buck LED drivers, powered from 12V (Nano runs off 5V via an onboard LM2596 buck regulator). Each
channel is one pot (A0–A3) → one PWM output (D11, D10, D9, D3) → one PT4115's DIM pin.

## PWM pins (ATmega328P)

The Nano's 6 PWM-capable pins split across 3 timers, each with its own default `analogWrite()`
frequency:

| Timer | Pins | Frequency |
|---|---|---|
| Timer0 | D5, D6 | ~980 Hz (also drives `millis()`/`delay()`) |
| Timer1 | D9, D10 | ~490 Hz |
| Timer2 | D3, D11 | ~490 Hz |

This board uses D11, D10, D9, D3 (Timer1 + Timer2), skipping D5/D6 so all 4 channels share one
common ~490 Hz frequency with zero timer register configuration.

## Folders

- **`KiCad/`** — schematic + PCB (KiCad 7+). Run ERC before fabricating — the LM2596's `ON/OFF`
  pin wiring is worth double-checking there.
- **`datasheets/`** — reference PDFs for the key ICs (PT4115 LED driver, LM2596 buck regulator,
  connector).
- **`fw/`** — firmware, a self-contained PlatformIO project. Includes a Wokwi diagram substituting
  plain LED+resistor pairs for the PT4115 modules, to verify the 4-channel PWM/pot correspondence
  without real hardware.
