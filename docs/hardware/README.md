# Hardware

BOM, power budget, wiring rules, and pin assignments.

## BOM (current)

- 1 × Arduino Nano (ATmega328, 5 V)
- 4 × WS2812 5050 RGB LED rings, 35 LEDs each (96 mm OD / 78 mm ID, ~9 W rated)
- 4 × potentiometers (dimmers), one per hob
- 1 × 5 V DC power supply (sizing below)
- 1 × ~330–470 Ω resistor (data line), 1 × ~1000 µF capacitor (5 V rail)

## Power budget (verified)

Per-LED full-white draw is ~60 mA @ 5 V (3 channels × ~20 mA) = 0.3 W.

| Scope | LEDs | Full-white current | Power |
|---|---|---|---|
| 1 ring | 35 | 2.1 A | 10.5 W |
| 4 rings | 140 | **8.4 A** | **~42 W** |

- The "≤9 W per ring" rating ≈ ~51 mA/LED — a typical/conservative figure, **not** worst case.
  Your math (4 × 9 W = 36 W → 7.2 A) is correct *for the rated number*; true full-white worst
  case is ~8.4 A / 42 W.
- **Realistically** the hobs render reds and oranges (blue channel off, green low, rarely all-on
  full bright), so actual draw is ~1–3 A total.

### PSU — pick one

- **5 V / 10 A (50 W):** full headroom, never think about it again. *or*
- **5 V / 6 A + firmware current cap:** FastLED `setMaxPowerInVoltsAndMilliamps(5, 5500)`
  auto-dims to stay under budget.

## Wiring rules

- **Power injection:** do NOT feed 7–8 A through the daisy chain. Run 5 V + GND in **parallel**
  to each ring directly from the PSU rail; chain only the **DATA** line in series.
- **Common ground** between the PSU and the Nano — mandatory, or data is noise.
- **~330–470 Ω resistor** in series on the data line at the first LED's DIN.
- **~1000 µF cap** across the 5 V rail near the LEDs (inrush protection).
- **Never** power the rings from the Nano's 5V pin — external PSU only.

## Pin assignments

> Pins are assigned **centrally** (see [architecture](../architecture/)). No software module
> hardcodes a pin; pins are passed in at init. This table is the single source of truth.

| Signal | Nano pin | Notes |
|---|---|---|
| LED data out | D6 | one line, drives all 140 LEDs (set in `include/Config.h`) |
| Hob 1 dimmer | A0 | analog in |
| Hob 2 dimmer | A1 | analog in |
| Hob 3 dimmer | A2 | analog in |
| Hob 4 dimmer | A3 | analog in |

Use A0–A5 for analog (avoid A6/A7 — analog-only, no internal pull-ups / digital use).
