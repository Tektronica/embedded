# lib/WallClock

Hardware-free time-keeping. Header-only, no dependencies beyond `stdint.h`.

- `wallclock::Clock`: time of day, advanced by `tick()` once per second. A software clock, so it
  resets to 0:00 on power loss.
- `wallclock::Timer`: a countdown that knows nothing about what happens at zero.
- `shiftEnteredDigit`, `splitDigits`, `decodeEnteredMinutes`, `isValidTime`, `to12Hour`: keypad
  digit entry and validation for HH:MM / MM:SS.

Namespace `wallclock`, not `clock`, to avoid shadowing C's `clock()`.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/WallClock
```

## Test

```bash
pio test -d lib/WallClock
```
