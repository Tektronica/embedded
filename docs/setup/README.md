# Setup

Development environment, building, and flashing the Nano.

## Toolchain — VSCode + PlatformIO

You do **not** need the Arduino IDE. Use the **PlatformIO IDE** extension in VSCode:

- Real IntelliSense/autocomplete, integrated debugging, dependency management.
- Uses the Arduino framework under the hood, so Arduino libraries work unchanged.
- Handles the toolchain and upload itself.

Arduino CLI (`brew install arduino-cli`) is a lighter alternative but you wire up editor
support yourself. Keep the Arduino IDE around only for quick throwaway sketches or the Serial
Plotter.

## platformio.ini (starting point)

```ini
[env:nano]
platform = atmelavr
board = nanoatmega328        ; use nanoatmega328old if uploads time out (old bootloader)
framework = arduino
monitor_speed = 115200
lib_deps =
    fastled/FastLED
```

If multiple Nano-based appliances ship from this repo, add one `[env:...]` per unit (e.g.
`[env:cooktop]`, `[env:microwave]`) selecting the right feature module via build flags.

## Flashing the Nano — gotchas

- **Bootloader:** older Nanos use the *old* bootloader. If uploads fail/time out, switch
  `board` to `nanoatmega328old`.
- **USB-serial chip:** clone Nanos usually use the **CH340**. macOS often works out of the box;
  some clones need the CH340 driver.
- **Port:** PlatformIO auto-detects; if needed, set `upload_port = /dev/cu.usbserial-XXXX`.
- **Logic level:** Nano is 5 V and drives WS2812 data directly — **no level shifter needed**
  (unlike 3.3 V boards such as the ESP32/ESP8266).

## Build / upload

```bash
pio run                 # build
pio run -t upload       # build + flash
pio device monitor      # serial monitor
```
