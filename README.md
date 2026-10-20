# embedded

Personal embedded projects in one repository: microcontroller firmware, shared libraries, and the
KiCad design packages for the custom boards.

## Layout

| Folder                 | Contents                                                                                                       |
| ---------------------- | -------------------------------------------------------------------------------------------------------------- |
| `projects/`            | Things built to be used. Each has `fw/` (firmware), and `hw/` (KiCad design package) when it has a custom PCB. |
| `examples/<platform>/` | Single-concept demos, one self-contained firmware project each                                                 |
| `lib/<platform>/`      | Shared libraries, used by projects and examples through PlatformIO `lib_deps`                                  |
| `hw/`                  | Shared hardware: KiCad symbol library, part catalog and selection policy, release tools, review template       |
| `docs/`                | Cross-project notes                                                                                            |

## Projects

| Project                                          | Platform              | Custom PCB | Description                                                                                                         |
| ------------------------------------------------ | --------------------- | ---------- | ------------------------------------------------------------------------------------------------------------------- |
| [led-dimmer-4ch](projects/led-dimmer-4ch/)       | Arduino Nano          | Yes        | Four pot-controlled LED channels driven by PT4115 buck LED drivers on 12 V                                          |
| [toy-microwave](projects/toy-microwave/)         | Arduino Nano          | Yes        | Toy microwave control panel: countdown, clock, kitchen timer, 16-key keypad, and 4-digit display through an ET6226M |
| [led-dimmer-ws2812](projects/led-dimmer-ws2812/) | Arduino Nano          | No         | N-channel pot dimmer driving a WS2812 LED strip                                                                     |
| [robot-buoy](projects/robot-buoy/)               | Arduino Nano          | No         | Vertical-profiling float that dives and surfaces with a syringe ballast                                             |
| [game-dino-run](projects/game-dino-run/)         | Arduino Nano          | No         | T-Rex-Run-style game on an OLED, with the score on a 7-segment display                                              |
| [weather-station](projects/weather-station/)     | Raspberry Pi / Python | No         | Sensor monitoring service with optional weather-sensor adapters                                                     |

## Examples

| Example                                                                   | Platform     | Demonstrates                                                                           |
| ------------------------------------------------------------------------- | ------------ | -------------------------------------------------------------------------------------- |
| [buzzer-simple](examples/arduino-nano/buzzer-simple/)                     | Arduino Nano | A passive buzzer on a button press                                                     |
| [buzzer-patterns](examples/arduino-nano/buzzer-patterns/)                 | Arduino Nano | Three buttons, each playing a different non-blocking tone pattern                      |
| [buzzer-song](examples/arduino-nano/buzzer-song/)                         | Arduino Nano | Song playback from note tables                                                         |
| [keypad](examples/arduino-nano/keypad/)                                   | Arduino Nano | A debounced 4x4 matrix keypad reader                                                   |
| [seven-segment](examples/arduino-nano/seven-segment/)                     | Arduino Nano | A 4-digit 7-segment display with static, flashing, and rolling modes                   |
| [display-keyscan-et6226m](examples/arduino-nano/display-keyscan-et6226m/) | Arduino Nano | The ET6226M display and key-scan driver on its own                                     |
| [pwm-pot-demo](examples/arduino-nano/pwm-pot-demo/)                       | Arduino Nano | A potentiometer setting PWM duty                                                       |
| [stepper](examples/arduino-nano/stepper/)                                 | Arduino Nano | A potentiometer setting stepper motor speed                                            |
| [toy-microwave-tm1637](examples/arduino-nano/toy-microwave-tm1637/)       | Arduino Nano | The earlier toy microwave on a TM1637 display and membrane keypad, kept as a reference |

## Tooling

- VS Code with the PlatformIO extension, or the `pio` CLI. No Arduino IDE.
- KiCad 10 for the custom boards.
- Wokwi for simulating examples that include a `diagram.json`.

## Getting started

1. Install PlatformIO. For a VS Code-only install, put the CLI on your path:
   `export PATH="$HOME/.platformio/penv/bin:$PATH"`
2. Build any firmware from the repo root: `pio run -d projects/toy-microwave/fw`
3. Run its native unit tests: `pio test -d projects/toy-microwave/fw -e native`
4. Run every shared library's tests: `for l in lib/arduino/*/; do pio test -d "$l"; done`
5. Regenerate a board's BOM, fab, and review outputs: `hw/tools/release.sh projects/toy-microwave/hw`

Each project and example README covers its own hardware, pins, and behavior.
