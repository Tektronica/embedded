# lib/PwmDuty

Hardware-free duty-cycle math. Header-only, no dependencies beyond `stdint.h`. Takes a 0..100 duty
percentage and knows nothing about where it came from.

- `pwmduty::dutyToPwm8(percent)`: duty -> `analogWrite()` value, 0..255.
- `pwmduty::gammaCorrect(percent)`: square-law perceptual curve for driving LEDs.
- `pwmduty::computeTimerConfig(cpuHz, targetHz)`: target frequency -> ATmega328 Timer1 prescaler
  and TOP. The app picks the frequency and writes the registers.
- `pwmduty::dutyToOcr(percent, top)`: duty -> `OCR1A` compare value for that TOP.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/PwmDuty
```

## Test

```bash
pio test -d lib/PwmDuty
```
