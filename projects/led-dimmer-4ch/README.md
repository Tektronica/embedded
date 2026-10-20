# LED Dimmer 4-Channel

A 4-channel LED dimmer board. An Arduino Nano reads 4 off-board potentiometers and drives 4 PT4115
constant-current buck LED drivers with PWM, one pot per channel. The board runs from 12 V, and an
onboard AP63205 buck regulator supplies the Nano's 5 V.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano, ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/PotInput`, `lib/arduino/PwmDuty` |
| Other libraries | None |
| Custom PCB | `hw/` in KiCad 10 |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Wokwi simulator, through `fw/diagram.json` and `fw/wokwi.toml`.
- KiCad 10 for the board, and `tools/release.sh` to regenerate its outputs.

## Getting started

Run these from the repo root. If `pio` isn't on your shell `PATH` (VS Code-only installs), add it
first:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

1. Build: `pio run -d projects/led-dimmer-4ch/fw`
2. Flash the Nano: `pio run -d projects/led-dimmer-4ch/fw -t upload`
3. Test the pot and duty logic in the shared libraries: `pio test -d lib/arduino/PotInput` and
   `pio test -d lib/arduino/PwmDuty`
4. Regenerate BOM, fab, and review outputs: `tools/release.sh projects/led-dimmer-4ch/hw`

The firmware has no native test env of its own. All tested logic lives in the two libraries.

## Hardware

### BOM

From `hw/bom/led-dimmer-4ch-bom.csv`. The Nano module is not in the exported BOM.

| Reference | Qty | Value | Part |
| --- | --- | --- | --- |
| U1-U4 | 4 | PT4115 | UMW PT4115, SOT-89-5 |
| U5 | 1 | AP63205WU | Diodes Inc AP63205WU-7, TSOT-23-6 |
| L1-L4 | 4 | 100 uH | Coilcraft MSS1038-104MLC |
| L5 | 1 | 4.7 uH | Murata LQH55DN4R7M03L |
| R1-R4 | 4 | 0.36 Ω | Ohmite KDV08FR360ET, 0805 |
| R5-R8 | 4 | 10 kΩ | Yageo RC0805FR-0710KL, 0805 |
| C1-C4 | 4 | 4.7 uF | KEMET C1210C475K5RACAUTO7210, 1210 |
| C5 | 1 | 10 uF | Taiyo Yuden TMK325AB7106MMHPR, 1210 |
| C6, C9-C12 | 5 | 100 nF | KEMET C0805C104M5RACAUTO, 0805 |
| C7, C8 | 2 | 22 uF | Samsung CL32A226MAJNNNE, 1210 |
| D1-D5 | 5 | SS14 | Vishay SS14-E3/61T, SMA |
| J1 | 1 | 2-pin screw terminal | Phoenix Contact 1715721, 5.08 mm |
| J2-J5 | 4 | 4-pin screw terminal | Phoenix Contact 1715747, 5.08 mm |

### Pins

From `fw/src/main.cpp`.

| Channel | Pot input | PWM output | Timer |
| --- | --- | --- | --- |
| CH0 | A0 (`POT_1`) | D9 (`PWM_1`) | Timer1 |
| CH1 | A1 (`POT_2`) | D10 (`PWM_2`) | Timer1 |
| CH2 | A2 (`POT_3`) | D3 (`PWM_3`) | Timer2 |
| CH3 | A3 (`POT_4`) | D11 (`PWM_4`) | Timer2 |

The pots are off-board and connect through `J4` and `J5`. Each PWM output drives one PT4115's DIM
pin.

### Power

USB alone powers the Nano only. The PT4115s need at least 6 V on VIN. The 5 V buck block (`U5`,
`C5`-`C8`, `D5`, `L5`) uses the same layout as the one in `projects/toy-microwave`, mirrored to
the front side. The LM2596 datasheets in `hw/datasheets/` are from the board's earlier buck
regulator. The AP63205 datasheet is in `projects/toy-microwave/hw/datasheets/`.

## How it works

Each loop pass reads all 4 channels in turn. For each channel, the firmware smooths the raw ADC
reading with an exponential moving average (`EMA_SHIFT` = 3), applies a deadzone, converts it to a
percent, and writes the matching 8-bit duty with `analogWrite()`. `setup()` seeds each filter with
the pot's current position, so the outputs start at the pot setting instead of ramping up from 0.

The Nano's 6 PWM pins split across 3 timers, each with its own default `analogWrite()` frequency:

| Timer | Pins | Frequency |
| --- | --- | --- |
| Timer0 | D5, D6 | about 980 Hz (also drives `millis()` and `delay()`) |
| Timer1 | D9, D10 | about 490 Hz |
| Timer2 | D3, D11 | about 490 Hz |

The board uses D9, D10, D3, and D11 and skips D5 and D6. Thus all 4 channels share one 490 Hz
frequency with no timer register setup.

### ADC multiplexer and unused pins

The AVR's ADC shares one multiplexer and one sample-and-hold capacitor across every analog pin.
Two separate failure modes follow from that.

Crosstalk between channels: on a channel switch, the capacitor still holds the previous channel's
charge until the new source drives it to its own voltage. Sources above about 10 kΩ can leave a
residual bias in the next reading. This only applies when the firmware alternates between 2 or
more channels. The fix is to read the channel twice and discard the first result, or to add a
0.1 uF cap from the pin to GND.

Floating pins: every ADC pin is also a digital I/O pin with its input buffer active by default. A
floating pin sits at an indeterminate voltage, draws shoot-through current in that buffer, and
picks up noise, whether or not `analogRead()` ever reads it. The fixes, in order of preference:

1. `pinMode(pin, INPUT_PULLUP)`, or drive the pin as an output.
2. Set the pin's `DIDR0` bit to disable its digital input buffer.
3. Tie the pin to GND, directly or through about 10 kΩ, if its state must be defined before
   `setup()` runs.

A cap to GND alone does not define an unused pin. It has no DC path, so the pin still drifts.

## Folders

- `fw/`: firmware, a PlatformIO project with the Wokwi diagram.
- `hw/`: the board design package. See `docs/hw/README.md` for the layout. It holds
  the KiCad project, datasheets, mechanical STEP, and the generated `bom/`, `fab/`, and `review/`
  outputs.

## Simulate

`fw/diagram.json` replaces each PT4115 module with a plain LED and resistor, since Wokwi has no
PT4115 part. It checks the pot-to-PWM channel mapping, not the LED driver behavior.

1. Build: `pio run -d projects/led-dimmer-4ch/fw`
2. In VS Code, run F1 > "Wokwi: Start Simulator".
3. Turn each pot and check that its LED follows.

## Status

Rev 1 board design and its generated outputs are in `hw/`. The firmware follows the rev 1 pin
map, and Wokwi covers its channel mapping.
