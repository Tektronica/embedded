# PWM Pot Demo

A potentiometer sets the duty cycle (0 to 100%) of a PWM output on D9. The example demonstrates
two shared libraries, `lib/arduino/PotInput` and `lib/arduino/PwmDuty`, and compares two PWM
implementations: `analogWrite()` and direct Timer1 register control.

## Platform

| Item | Value |
| --- | --- |
| Board | Arduino Nano (`nanoatmega328`), ATmega328P, 5 V, 16 MHz |
| Firmware | C++ on the Arduino framework, built with PlatformIO |
| Shared libraries | `lib/arduino/PotInput`, `lib/arduino/PwmDuty` |
| Other libraries | None |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI.
- Unity tests live with the libraries, not in this example.
- Wokwi simulator (`diagram.json`, `wokwi.toml`).

## Getting started

If `pio` is not on your shell `PATH` (VS Code extension only installs), add it:

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

Run from the repo root:

1. Build: `pio run -d examples/arduino-nano/pwm-pot-demo`
2. Build and flash the Nano: `pio run -d examples/arduino-nano/pwm-pot-demo -t upload`
3. Run the library tests: `pio test -d lib/arduino/PotInput` and `pio test -d lib/arduino/PwmDuty`
4. Open the serial monitor (only prints when `DEBUG_TRACE_ENABLED` is `true` in `src/main.cpp`):
   `pio device monitor -d examples/arduino-nano/pwm-pot-demo`

## Hardware

- Arduino Nano
- Potentiometer
- LED with a ~330 Ω series resistor

| Signal | Nano pin | Notes |
| --- | --- | --- |
| Potentiometer wiper | A0 | Outer terminals to 5 V and GND |
| PWM output | D9 | Through a ~330 Ω resistor when driving an LED |

Both PWM strategies use the same wiring. Only the register setup differs.

## How it works

Each loop reads A0, smooths the reading with `potinput::emaStep()` (`EMA_SHIFT` 3), applies
`potinput::Deadzone`, and converts it to a percent with `potinput::toPercent()`. The percent goes
to whichever PWM strategy is selected.

### PWM pins on the ATmega328P

| Timer | Pins | Default `analogWrite()` frequency |
| --- | --- | --- |
| Timer0 | D5, D6 | ~980 Hz (also drives `millis()` and `delay()`) |
| Timer1 | D9, D10 | ~490 Hz |
| Timer2 | D3, D11 | ~490 Hz |

This example uses D9, which is OC1A on Timer1.

### PWM strategy

`PWM_MODE` in `src/main.cpp` selects the strategy at compile time. The default is
`PwmMode::AnalogWrite`.

- `PwmMode::AnalogWrite`: `renderAnalogWritePwm()` calls `analogWrite()` with
  `pwmduty::dutyToPwm8()` (0 to 255). The frequency is fixed at Timer1's default, ~490 Hz. An LED
  looks fine at that rate.
- `PwmMode::DirectTimer`: `setupDirectTimerPwm()` configures Timer1 Fast PWM mode 14, with `ICR1`
  as TOP and non-inverting output on OC1A. `pwmduty::computeTimerConfig()` picks the prescaler and
  TOP for `PWM_FREQUENCY_HZ` (1000 Hz by default), and `renderDirectTimerPwm()` writes
  `pwmduty::dutyToOcr()` to `OCR1A`. Use it when the frequency matters, for motors, audio, or
  avoiding interference. It works on real hardware only. See Known issues.

### Firmware layout

- `lib/arduino/PotInput`: `emaStep()`, `Deadzone`, and `toPercent()` turn a raw reading into a
  steady 0 to 100.
- `lib/arduino/PwmDuty`: hardware-free duty math for both strategies. `dutyToPwm8()`,
  `computeTimerConfig()`, `dutyToOcr()`, and `gammaCorrect()`.
- `src/main.cpp`: pins, both strategies, the Timer1 register writes, and the loop.

`pwmduty::gammaCorrect()` applies a square-law correction for how the eye perceives LED
brightness. The firmware does not call it, because Wokwi's LED already applies its own brightness
curve and the two together suppressed the bottom half of the pot range. Wrap the duty with it,
`pwmduty::gammaCorrect(potinput::toPercent(...))`, when driving a real LED.

`DEBUG_TRACE_ENABLED` (off by default) prints a Teleplot trace of `raw` and `duty` every 200 ms at
115200 baud.

## Simulate

Build with the default `PwmMode::AnalogWrite`, then run **F1 > Wokwi: Start Simulator** in VS
Code. Drag the potentiometer to change the LED brightness.

## Known issues

- `PwmMode::DirectTimer` does not work in Wokwi. The avr8js core does not emulate Timer1 modes
  that read TOP from a register (`ICR1` or `OCR1A`), so the pin stays high regardless of the
  compare value. Fast PWM mode 14 (`ICR1` as TOP) and mode 15 (`OCR1A` as TOP) both failed the
  same way. See [avr8js#122](https://github.com/wokwi/avr8js/issues/122) and the
  [simavr equivalent](https://github.com/buserror/simavr/issues/162). `analogWrite()` uses
  Phase Correct mode with a fixed TOP of `0x00FF`, which simulates correctly.
- Auto-reset is unreliable on this CH340 Nano. Tap RESET as the upload starts.

## Status

Builds and simulates in Wokwi with `PwmMode::AnalogWrite`. `PwmMode::DirectTimer` is kept as a
reference implementation for real hardware.
