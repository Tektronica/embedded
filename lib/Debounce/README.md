# lib/Debounce

Hardware-free debouncing. Header-only, no dependencies beyond `stdint.h`.

- `debounce::Debouncer<T>(initial, samples = 3)`: `update(raw)` returns true on the sample that
  commits a new value, which only happens once the same new reading repeats `samples` times in a
  row. Works for any `T` with `==`: a button level, a raw key index, a key-position struct.
- `debounce::Button(samples = 3)`: `pressed(raw)` is true once per debounced press, never on
  release or while held.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/Debounce
```

## Test

```bash
pio test -d lib/Debounce
```
