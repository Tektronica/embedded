# Hardware

BOM, power budget, wiring rules, and pin assignments for the generic controller. (Physical
form-factor — e.g. bending an LED strip into a ring — is application-specific; see
[docs/application](../application/).)

## BOM (default 4-channel build)

| Qty | Component              | Spec / notes                                                                |
| --- | ---------------------- | --------------------------------------------------------------------------- |
| 1   | Arduino Nano           | ATmega328, 5 V                                                              |
| 4   | WS2812 LED strip       | 35 LEDs each (`CHANNEL_COUNT` × `LED_STRIP_LENGTH`)                         |
| 4   | Potentiometer (dimmer) | standard single-turn rotary panel pot, 10 kΩ linear (B10K); one per channel |
| 2   | Momentary push switch  | optional — cycle mode + color                                               |
| 1   | 5 V DC power supply    | sizing below                                                                |
| 1   | Resistor               | ~330–470 Ω, data line                                                       |
| 1   | Capacitor              | ~1000 µF, 5 V rail                                                          |

## Power budget (verified)

Per-LED full-white draw is ~60 mA @ 5 V (3 channels × ~20 mA) = 0.3 W.

| Scope        | LEDs | Full-white current | Power     |
| ------------ | ---- | ------------------ | --------- |
| 1 LED strip  | 35   | 2.1 A              | 10.5 W    |
| 4 LED strips | 140  | **8.4 A**          | **~42 W** |

- Full-white is worst case. With the default `levelColor` curve (reds/oranges — blue off, green
  low, rarely full bright), actual draw is ~1–3 A total.

### PSU — 5 V directly

WS2812 are **5 V** parts, so the LEDs need a 5 V rail at the full LED current regardless of source —
power them from a 5 V supply directly. Stepping down from 12 V/24 V buys nothing here: the buck would
have to carry the entire LED current (an LM2596 can't) and only adds a lossy stage. Reserve a higher
bus voltage for when you actually have 12 V/24 V loads.

Pick a current rating:

- **5 V / 6 A + firmware current cap (default):** FastLED `setMaxPowerInVoltsAndMilliamps(5, 5500)`
  auto-dims to stay under budget; 6 A covers the 5.5 A cap with margin. _or_
- **5 V / 10 A (50 W):** full-white headroom, never think about the cap again.

## Wiring rules

- **Power injection:** do NOT feed 7–8 A through the daisy chain. Run 5 V + GND in **parallel** to
  each LED strip directly from the PSU rail; chain only the **DATA** line in series.
- **Dimmer pots as voltage dividers:** outer terminals to 5 V and GND, wiper to the analog pin
  (A0-A3) — not as a 2-terminal rheostat, which leaves the input floating. 10 kΩ keeps the ADC
  source impedance in spec (≤ ~10 kΩ); much higher values read noisy/laggy.
- **Common ground** between the PSU and the Nano — mandatory, or data is noise.
- **~330–470 Ω resistor** in series on the data line at the first LED's DIN.
- **~1000 µF cap** across the 5 V rail near the LEDs (inrush protection).
- **Never** power the LED strips from the Nano's 5V pin — external PSU only.

## Pin assignments

> Pins live at the top of `src/main.cpp` (`PIN_DATA_LED`, `PIN_DIMMER`). This table is the single
> source of truth.

| Signal                                      | Nano pin | Notes                                              |
| ------------------------------------------- | -------- | -------------------------------------------------- |
| LED data out (`PIN_DATA_LED`)               | D6       | one line; drives all 140 LEDs (chained LED strips) |
| Channel 0 dimmer                            | A0       | analog in                                          |
| Channel 1 dimmer                            | A1       | analog in                                          |
| Channel 2 dimmer                            | A2       | analog in                                          |
| Channel 3 dimmer                            | A3       | analog in                                          |
| Mode switch (`PIN_SWITCH_MODE`, optional)   | D8       | `INPUT_PULLUP` → GND; cycles Mode                  |
| Color switch (`PIN_SWITCH_COLOR`, optional) | D9       | `INPUT_PULLUP` → GND; cycles Palette               |

Use A0–A5 for analog (avoid A6/A7 — analog-only, no internal pull-ups / digital use).
