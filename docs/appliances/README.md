# Appliances

Per-appliance specs and the plugin contract. Cooktop is the only appliance today; the contract
is extracted properly once a second one is real.

## Plugin contract

Each appliance implements `IAppliance` (see [architecture](../architecture/)):

```cpp
struct IAppliance {
    virtual void update() = 0;   // read inputs, advance state
    virtual void render() = 0;   // draw to outputs
};
```

The app loop calls `update()` then `render()` each frame. Appliances are selected at compile
time per build, not at runtime.

## Cooktop (current)

- **Hobs:** 4, each a 35-LED WS2812 ring; all four data-chained as one 140-LED strip.
- **Input:** 1 dimmer (potentiometer) per hob → analog in (A0–A3).
- **Mapping:** dimmer position → heat level → color/brightness ramp
  (off → deep red → bright orange; never yellow/white). HSV via FastLED.
- **Ring layout:** ring N owns LED indices `[N*35, N*35+34]`.

## Microwave (future)

Placeholder. When real, it implements the same `IAppliance` contract and gets its own build
environment. Document its inputs/outputs and LED/segment layout here at that point.
