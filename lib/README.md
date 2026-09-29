# lib

Shared, header-only libraries used by the projects under `arduino-nano/`. Rules for adding one are
in the root `CLAUDE.md`. No library includes another, except `ET6226M`'s driver and codec, which
are one package.

| Library | What it is | Used by |
| --- | --- | --- |
| `PotInput` | Pot smoothing, rail deadzone, percent scaling | pwm-pot-demo, led-dimmer-4ch, led-dimmer-ws2812, stepper |
| `PwmDuty` | Duty percent -> `analogWrite()` / Timer1 values, gamma | pwm-pot-demo, led-dimmer-4ch |
| `SevenSeg` | 7-segment font, text, blink phase, MM:SS digits | both toy microwaves, display-keyscan-et6226m, seven-segment |
| `ET6226M` | ET6226M display/key-scan driver and codec | toy-microwave-et6226m, display-keyscan-et6226m |
| `WallClock` | Time-of-day clock, countdown timer, digit entry | both toy microwaves |
| `Debounce` | `Debouncer<T>` and `Button` | buzzer-patterns, buzzer-song, seven-segment, stepper, led-dimmer-ws2812, keypad, both toy microwaves |
| `KeyMatrix` | Row/column matrix scan driver | keypad, toy-microwave-tm1637 |
| `ToneSequence` | Non-blocking `{frequency, duration}` tone tables | both toy microwaves, buzzer-patterns, game-dino-run, buzzer-song (`ToneState` only) |

Run every library's tests from the repo root:

```bash
for l in lib/*/; do [ -d "$l/test" ] && pio test -d "$l"; done
```
