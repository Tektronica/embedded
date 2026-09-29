# lib/SevenSeg

Hardware-free 7-segment encoding. Header-only, no dependencies beyond `stdint.h`. Segment bytes use
bit0=A .. bit6=G, bit7=DP, which TM1637Display and the ET6226M both take as-is.

- `sevenseg::encodeChar(c)`: one font for the whole repo. Digits and the letters that are legible
  on seven segments, case-sensitive where the shapes differ (H/h, U/u). Illegible letters return
  `DASH`, space returns `BLANK`.
- `sevenseg::encodeDigit(d)` and `sevenseg::encodeText(text, out, count)`: built on `encodeChar`.
- `sevenseg::blinkOn(frame, period)`: on for the first half of each period.
- `sevenseg::secondsToDigits(seconds)`: MM:SS digits, clamped to 99:59.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/SevenSeg
```

## Test

```bash
pio test -d lib/SevenSeg
```
