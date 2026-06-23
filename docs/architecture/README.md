# Architecture

Software design: MVC, the plugin (Strategy) model, layering, and pin ownership — adapted for
an 8-bit MCU.

## MCU constraints (the ground rules)

ATmega328: 2 KB SRAM, 32 KB flash. The LED framebuffer alone is 140 × 3 = 420 B (~20% of SRAM).

- **No dynamic allocation** in the hot path — avoid `new`, `String`, `std::vector`,
  `std::function`, RTTI/`dynamic_cast`. Static allocation only.
- **Shallow inheritance.** A vtable + a few virtuals is fine; deep hierarchies and
  factories-that-`new` are not.
- Use design *principles* (separation of concerns, dependency inversion); skip the pattern zoo.

## Why repo-per-board

This is the **Nano** repo. The board is the project boundary, so `nano` is in the name. A
different chip (ESP32) is a *separate* repo that reuses these patterns, not this code. "One
codebase → many deploys" still applies, but scoped to **multiple Nano units** (a cooktop Nano, a
microwave Nano) via PlatformIO environments — not across chip families.

## MVC split

- **Model** — pure state + logic, zero hardware (e.g. `hobLevel[4]`, heating state). Testable
  off-device.
- **View** — `LedRenderer`: maps level → ring colors, owns the FastLED framebuffer and the
  index layout (which of the 140 LEDs belong to ring N).
- **Controller** — `InputController`: reads/smooths the 4 dimmers, writes the Model.
- **App loop** — orchestrates: `read inputs → update model → render`.

## Plugin model (Strategy)

Appliances implement a common interface so the app layer drives them uniformly:

```cpp
struct IAppliance {
    virtual void update() = 0;   // read inputs, advance state
    virtual void render() = 0;   // draw to outputs
};
```

`CooktopAppliance` now; `MicrowaveAppliance` later. Selected at **compile time** per build
(build flags / `src_filter`), not loaded at runtime. See [appliances](../appliances/).

## Pin ownership

- Pins are assigned in **one** central config header — no module hardcodes a pin.
- Modules receive their pins via `init()`/constructor params (**dependency injection**). This
  keeps Model/Controller testable off-hardware and means no layer "owns" the pins.
- The authoritative pin table lives in [hardware](../hardware/).

## Patterns: use vs avoid

- **Use (lightly):** Strategy (plugin), dependency injection, central config. State machine
  per hob only if animations need it.
- **Avoid on Nano:** heap-based Observer/factories, Singletons with lazy heap init, deep
  inheritance, anything pulling in `String`/STL containers/RTTI.
