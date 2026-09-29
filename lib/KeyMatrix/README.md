# lib/KeyMatrix

Row/column key matrix scan driver. Needs the Arduino core, not unit-tested.

- `keymatrix::MatrixScanner(rowPins, rowCount, colPins, colCount)`: `begin()` sets pin modes,
  `scan()` drives each row low in turn and returns `row * colCount + col` for the first active key,
  or `keymatrix::NO_KEY`.

Debouncing (`lib/Debounce`) and the key layout belong to the app.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/KeyMatrix
```
