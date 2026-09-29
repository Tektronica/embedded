# lib/ET6226M

Driver for the UMW ET6226M (LED display plus key scan over a two-wire bus), and its codec.

- `ET6226M.h`: the driver class. Bit-banged CLK/DAT, needs the Arduino core, not unit-tested.
  Takes raw segment bytes, so the font comes from wherever the app chooses (`lib/SevenSeg` here).
- `ET6226MCodec.h`: hardware-free. `encodeDisplayControl()` (brightness, on/off, segment mode) and
  `decodeKeyCode()` (key code -> grid/segment `KeyPosition`, which supports `==`).

The driver includes its own codec. That's the one allowed dependency, since the two are one
package.

## Use from a project

```ini
lib_deps = symlink://<path to repo root>/lib/ET6226M
```

## Test

```bash
pio test -d lib/ET6226M      # codec only; the driver needs real hardware
```
