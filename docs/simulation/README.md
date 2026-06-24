# Simulation (Wokwi, VS Code)

Run the firmware in the [Wokwi](https://wokwi.com) simulator to verify the four-dimmer → four-hob
behavior with **no physical hardware** — drag each potentiometer and watch its ring ramp
off → deep red → orange.

The simulator runs the **exact binary** PlatformIO builds (`.pio/build/nano/firmware.elf`); no code
changes are needed.

## Install + license (one time)

1. Install the **Wokwi for VS Code** extension.
2. Press **F1** → **"Wokwi: Request a new License"**. Confirm opening the Wokwi site ("Open").
3. Click **"GET YOUR LICENSE"** (sign in / create a free Wokwi account if prompted).
4. Confirm sending the license back to VS Code (you may confirm twice — once in the browser, once
   in VS Code). You'll see **"License activated for [your name]"**.

## Project files (already in the repo)

- **`wokwi.toml`** — points the simulator at the built firmware (`.hex` + `.elf`).
- **`diagram.json`** — the circuit: Nano + 4 potentiometers (A0–A3) + 4 × 35-LED `wokwi-led-strip`
  parts chained on D6.

## Run it

1. Build first so the firmware exists: `pio run` (or VS Code **Build**).
2. Open `diagram.json`, press **F1** → **"Wokwi: Start Simulator"** (or click the green ▶ on the
   diagram).
3. **Drag a potentiometer** → its hob's ring changes color/brightness, independently of the others.

## What the diagram models (must match `src/main.cpp`)

| Firmware | Wokwi wiring |
|---|---|
| `LED_DATA_PIN = 6` | Nano **`6`** (not `D6`!) → ring1 `DIN`; each `DOUT`→next `DIN` (4 strips chained) |
| `DIMMER_PINS = {A0,A1,A2,A3}` | pot1–4 `SIG` → **A0–A3**; `VCC`→5V, `GND`→GND |
| `LEDS_PER_RING = 35`, 4 hobs | 4 × `wokwi-led-strip` (`pixels: 35`) = 140 px; power `VDD`→5V, `VSS`→GND |

## Notes

- **All strips black? First suspect the data-pin label.** On `wokwi-arduino-nano` the digital pin
  is **`nano:6`** (numeric) — **`nano:D6` silently does not connect**, so the strip gets no data and
  stays dark. The firmware uses pin 6, so the wire must be `nano:6` → `ring1:DIN`.
- **Also:** the potentiometer `value` defaults to `0` → level 0 → off. `diagram.json` seeds non-zero
  values (1000/650/350/120) so the four strips light in different heat colors on load; drag a pot to
  change its strip.
- **Power comes from the Nano's `5V` pin** in the sim (strips' `VDD`→`5V`, `VSS`→`GND`). No separate
  supply is needed — the external-PSU / power-injection guidance in `docs/hardware` is a *real-board*
  concern, not a simulator one.
- **`pixelSize: "2020"`** keeps each 35-LED strip compact (~9 px/LED); the default `"5050"` makes
  them ~800 px wide and unwieldy.
- **No floating pins in sim** — the pots give defined values, so rings respond cleanly (unlike a
  bare board, where unconnected analog inputs read random noise).
- **140 LEDs** is heavy for the simulator; it runs but may render below 60 fps. Fine for verifying
  behavior.
- `setMaxPowerInVoltsAndMilliamps` still applies its brightness cap in sim — expected.
- LEDs use **`wokwi-led-strip`** (`pixels: 35`, chained `DOUT`→`DIN`; power pins `VDD`/`VSS`). If you
  prefer a ring look, `wokwi-led-ring` exists but comes in fixed sizes — the strip lets us match the
  real 35-LED count. If anything fails to load, add the part from the diagram editor's **+** panel
  and re-wire per the table; the editor writes correct `diagram.json` for you.
