# Configure Sensors

`projects/weather-station/python/sensors.toml` is the source of enabled sensor IDs, models, pins, and calibration values. Each sensor has one `[[sensor]]` block:

```toml
[[sensor]]
id = "room-am2302"
model = "am2302"
gpio = 4
```

Use unique IDs. The model must be in the factory catalog. Only options supported by that model are accepted; unknown models/options or invalid option types/ranges stop startup as configuration errors. A sensor that fails its first hardware read is logged as not detected and skipped. If no configured sensor responds, the service exits and systemd retries after 60 seconds.

Supported model options:

| Model           | Options                                                             | Defaults            |
| --------------- | ------------------------------------------------------------------- | ------------------- |
| `am2302`        | `gpio` (BCM pin 0–27)                                               | 4                   |
| `bme280`        | `i2c_address` (7-bit address)                                       | `0x76`              |
| `sgp30`         | None                                                                | fixed I²C address   |
| `veml6075`      | None                                                                | fixed I²C address   |
| `bh1750`        | None                                                                | fixed I²C address   |
| `ds18b20`       | `index` (non-negative probe index)                                  | 0                   |
| `as3935`        | `i2c_address` (7-bit address)                                       | `0x03`              |
| `anemometer`    | `gpio`, `meters_per_second_per_hz` (>0), `gust_window_seconds` (>0) | 17, 2.4, 300        |
| `rain-gauge`    | `gpio`, `mm_per_tip` (>0)                                           | 27, 0.2794          |
| `wind-vane`     | `adc_address`, `channel` (0–3)                                      | `0x48`, 0           |
| `soil-moisture` | `adc_address`, `channel`, `dry_voltage`, `wet_voltage`              | `0x48`, 1, 2.8, 1.2 |

For analog calibration and hardware choices, see [Choose sensors](../sensor-selection/README.md).
