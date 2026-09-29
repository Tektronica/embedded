# lib/PotInput

Hardware-free conditioning for a raw 10-bit potentiometer reading. Header-only, no dependencies
beyond `stdint.h`.

- `potinput::emaStep(smoothed, raw, shift)`: one integer EMA step that converges exactly to the
  rails. The app picks `shift`.
- `potinput::Deadzone(low, high, hysteresis)`: pins readings at or beyond `low` / `high` to
  `0` / `ADC_MAX`, releasing only `hysteresis` counts past the threshold. Defaults are 20 / 1000
  / 10. One instance per pot.
- `potinput::toPercent(raw)`: raw reading -> 0..100.

Typical use, per pot, per loop: `smoothed = emaStep(smoothed, analogRead(pin), shift)`, then
`deadzone.apply(smoothed)`, then `toPercent()` or the app's own scaling (level, speed).

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/PotInput
```

Add it to every env that includes `PotInput.h`, including `native`.

## Test

```bash
pio test -d lib/PotInput
```
