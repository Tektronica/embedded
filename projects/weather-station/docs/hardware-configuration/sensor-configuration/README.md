# Configure Sensors

`projects/weather-station/python/config.json` lists the enabled sensors under `sensors.devices`. Each device has an ID, a model, and that model's options such as pins and calibration values. `init.sh` copies `config.example.json` to `config.json` when the file is missing. `config.json` is ignored by Git.

```json
{
  "sensors": {
    "devices": [
      { "id": "sensor-am2302", "model": "am2302", "gpio": 4 }
    ]
  }
}
```

An ID takes the form `sensor-<model>`. A second sensor of the same model takes a numeric suffix, such as `sensor-am2302-2`. IDs must be unique, and a duplicate stops startup. The model must be in the factory catalog. Only options supported by that model are accepted. Unknown models, unknown options, and invalid option types or ranges stop startup as configuration errors. A boolean is not accepted where a number is required. A sensor that fails its first hardware read is logged as not detected and skipped. If no configured sensor responds, the service exits and systemd retries after 60 seconds.

Supported model options:

| Model           | Options                                                             | Defaults            |
| --------------- | ------------------------------------------------------------------- | ------------------- |
| `am2302`        | `gpio` (BCM pin 0–27)                                               | 4                   |
| `bme280`        | `i2c_address` (7-bit address)                                       | `118`               |
| `sgp30`         | None                                                                | fixed I²C address   |
| `veml6075`      | None                                                                | fixed I²C address   |
| `bh1750`        | None                                                                | fixed I²C address   |
| `ds18b20`       | `index` (non-negative probe index)                                  | 0                   |
| `as3935`        | `i2c_address` (7-bit address)                                       | `3`                 |
| `anemometer`    | `gpio`, `meters_per_second_per_hz` (>0), `gust_window_seconds` (>0) | 17, 2.4, 300        |
| `rain-gauge`    | `gpio`, `mm_per_tip` (>0)                                           | 27, 0.2794          |
| `wind-vane`     | `adc_address`, `channel` (0–3)                                      | `72`, 0             |
| `soil-moisture` | `adc_address`, `channel`, `dry_voltage`, `wet_voltage`              | `72`, 1, 2.8, 1.2   |

## I²C addresses in decimal

JSON has no hex literals, so write an I²C address as a decimal integer. The sensors service accepts integers from 0 to 127. A string such as `"0x76"` is rejected.

| Hex    | Decimal | Used by                      |
| ------ | ------- | ---------------------------- |
| `0x03` | 3       | `as3935` default             |
| `0x48` | 72      | `adc_address` default        |
| `0x76` | 118     | `bme280` default             |
| `0x77` | 119     | `bme280` with SDO pulled high |

## Example devices

JSON has no comments, so these are the supported sensors as copy-ready entries. Add the ones you connect to `sensors.devices`. Pins, addresses, and calibration values are examples.

```json
{ "id": "sensor-bme280", "model": "bme280", "i2c_address": 118 }
```

```json
{ "id": "sensor-anemometer", "model": "anemometer", "gpio": 17, "meters_per_second_per_hz": 2.4 }
```

```json
{ "id": "sensor-rain-gauge", "model": "rain-gauge", "gpio": 27, "mm_per_tip": 0.2794 }
```

At startup the sensors service builds every entry and keeps only the sensors that answer a first read. An unplugged sensor is left out. Restart the service after plugging one in. Entries in `sensors.devices` are separated by commas.

For analog calibration and hardware choices, see [Choose sensors](../sensor-selection/README.md).
