# nano/microwave

Arduino Nano firmware for a toy/prop microwave oven control panel: a countdown timer on a 4-digit
display, a turntable motor, a done/alert buzzer, a cooling fan and buzzer-tone layer for the
running "hum," an interior light, and keypad input for setting time and start/stop.

## BOM

| Qty | Role (generic) | Part chosen | Notes |
|---|---|---|---|
| 1 | 4-digit 7-segment display (countdown timer) | WWZMDiB 4-Digit 7-Segment LED Display Board for Arduino | Sold in a 5-pack. TM1637 driver chip — 2-wire CLK/DIO protocol, only 2 Nano pins needed. |
| 1 | Buzzer (done/alert tone, button feedback, running hum) | 12085 buzzer, 12×8.5mm, 42Ω, 3–12V | Sold in a 20-pack. Passive — driven via `tone()`/PWM; also layers a quiet low-frequency tone during Running, alongside the fan and motor, for the ambient hum. |
| 1 | Turntable motor | TYC-50 synchronous motor, 12V DC, 5–6 RPM, CW/CCW, 4W | Matches a real microwave turntable's rotation speed; its own running sound also contributes to the hum. |
| 1 | Cooling fan (running hum) | Not yet chosen — a small 12V sleeve-bearing fan (30–40mm) | Match the 12V rail already used by the motor/light rather than introducing a separate 5V domain. Needs a transistor/MOSFET driver, same as the motor — not a direct GPIO connection. |
| 1 | Interior/status light | 80mm white COB LED ring ("angel eye" halo ring) | Sold in a 2-pack. 12V/24V rated — confirm current draw and how it's switched (relay/MOSFET vs. direct GPIO) before wiring. |
| — | Keypad (time entry, start/stop) | Not yet chosen | Likely mechanical keyswitches (Cherry MX or similar) for tactile feedback; matrix vs. individually-wired switches is still open — see the firmware note below. |

## Firmware

Self-contained PlatformIO project (`platformio.ini`, `include/`, `src/`, `test/`), scaffolded but
not yet implemented:

- **`SevenSegment.h`** — countdown-time formatting (seconds → digit values) for the TM1637 display; the TM1637's own encoding/multiplexing is handled by a library, not this header.
- **`Buzzer.h`** — beep-pattern sequencing (key press, done, error tones) for the passive buzzer, plus the quiet continuous low-frequency tone it plays during Running as part of the hum.
- **`KeyInput.h`** — debounced key events; matrix-scan vs. plain per-key debounce depends on whether the keypad ends up wired as a matrix or individually-wired switches (still open).
- **`Microwave.h`** — the Idle → Setting → Running → Done state machine tying the above together.
- Motor, fan, and light have no dedicated header — all three are plain on/off hardware glue in `main.cpp`.

## Status

BOM mostly set (keypad still unchosen); no hardware design yet. Firmware is scaffolded (see
above) but unimplemented.
