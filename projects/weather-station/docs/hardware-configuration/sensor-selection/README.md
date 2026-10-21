# Choose Sensors

Pick a sensor by measured quantity, environment, interface, and required calibration. Check the exact breakout board's voltage and driver API before ordering; a model name alone doesn't guarantee that two boards have the same pinout or address.

| Model in `config.json` | Measures                                 | Interface / notes                                                 |
| ----------------------- | ---------------------------------------- | ----------------------------------------------------------------- |
| `am2302`                | Temperature, relative humidity           | One-wire timing on a GPIO; enabled by default                     |
| `bme280`                | Temperature, relative humidity, pressure | I²C; address configurable                                         |
| `sgp30`                 | TVOC, equivalent CO₂                     | I²C; requires regular sampling and warm-up/baseline handling      |
| `veml6075`              | UV index                                 | I²C; raw UVA/UVB counts aren't stored as SI measurements          |
| `bh1750`                | Illuminance                              | I²C                                                               |
| `ds18b20`               | Probe temperature                        | 1-Wire; enable the bus in Raspberry Pi OS                         |
| `as3935`                | Lightning count/distance                 | I²C scaffold; configure interrupt handling for the exact breakout |
| `anemometer`            | Wind speed/gust                          | GPIO pulses; calibrate the speed factor                           |
| `rain-gauge`            | Rain accumulation/rate                   | GPIO tip counter; calibrate `mm_per_tip`                          |
| `wind-vane`             | Wind direction                           | ADS1115 analog input; calibrate voltage-to-angle mapping          |
| `soil-moisture`         | Soil moisture                            | ADS1115 analog input; calibrate dry/wet voltages                  |

Wind vane and soil moisture can share one ADS1115 object when configured with the same `adc_address`. GPIO pulse sensors need appropriate pull-ups, debounce, surge protection, and a hardware counter if the pulse rate exceeds reliable software capture.

Only AM2302 is enabled in the shipped configuration. See [Sensor configuration](../sensor-configuration/README.md).
