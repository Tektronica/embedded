# arduino-nano/led-dimmer-4ch

A 4-channel potentiometer-controlled LED dimmer board: Arduino Nano + 4× PT4115 constant-current
buck LED drivers, powered from 12V (Nano runs off 5V via an onboard LM2596 buck regulator). Each
channel is one pot (A0–A3) → one PWM output (D9, D10, D3, D11) → one PT4115's DIM pin.

## Folders

- **`KiCad/`** — schematic + PCB (KiCad 7+). Run ERC before fabricating — the LM2596's `ON/OFF`
  pin wiring is worth double-checking there.
- **`datasheets/`** — reference PDFs for the key ICs (PT4115 LED driver, LM2596 buck regulator,
  connector).
- **`fw/`** — firmware, a self-contained PlatformIO project. `analogWrite()` on all 4 channels
  (D9/D10 share Timer1, D3/D11 share Timer2) gives one common ~490 Hz frequency with no timer
  register configuration. Includes a Wokwi diagram substituting plain LED+resistor pairs for the
  PT4115 modules, to verify the 4-channel PWM/pot correspondence without real hardware.
