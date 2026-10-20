# Weather Station

A Raspberry Pi weather station. A sensor collector samples an AM2302/DHT22 every 15 minutes and
stores each reading in SQLite. A local Flask dashboard shows the latest values and 30 days of
history, and a daily email summarizes every sensor. Adapters for ten more sensor types exist but
aren't enabled in the collector yet.

## Platform

| Item | Value |
| --- | --- |
| Board | Raspberry Pi running Raspberry Pi OS |
| Language | Python 3.12 or newer |
| Packages | `shared`, `sensors`, `web`, and `notifications`, all under the `weather_station` namespace |
| Storage | One SQLite file, written only by `sensors` |
| Custom PCB | None |

## Packages

The code is four installable packages. Each runs in its own process with its own dependencies, and
none of them imports another except `shared`.

| Package | Import | Job | Runs as | Database access |
| --- | --- | --- | --- | --- |
| `python/shared/` | `weather_station.shared` | Measurement model, sensor catalog, read contract, SQLite schema and read-only reader | Library | Defines the schema and the reader |
| `python/sensors/` | `weather_station.sensors` | Sensor adapters, sampling loop, sensor registration | `weather-sensors.service` | Writer, the only one |
| `python/web/` | `weather_station.web` | Dashboard and JSON API | `weather-web.service` | Read-only |
| `python/notifications/` | `weather_station.notifications` | Daily digest email | `weather-digest.timer`, 07:00 | Read-only |

The SQLite schema in `shared/src/weather_station/shared/db/schema.sql` is the contract between the
writer and its readers. The reader opens the file in SQLite's read-only mode and refuses to run
against a schema version it doesn't support. The hardware libraries are dependencies of `sensors`
alone, so they aren't installed alongside the web server or the notifier.

## Tooling

- Python 3.12 or newer with `venv`, or `uv`
- `unittest` for tests and `mypy --strict` for type checks
- systemd for the three services and the digest timer, with units in `deploy/`
- Tailwind CSS v4 for the dashboard styles, built once into a committed `static/dashboard.css`. The Pi needs no Node.

## Getting started

### Development (any machine)

Run from `projects/weather-station/python`:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-dev.txt
cp -n .env.example .env
```

Run the tests and the type check:

```bash
for d in shared/tests sensors/tests web/tests notifications/tests tests; do python -m unittest discover -s "$d"; done
mypy shared/src sensors/src web/src notifications/src
```

The tests stub the sensor hardware and use temporary SQLite files, so they don't need a Pi. The
root `tests/` folder holds the boundary test that keeps each package importing only itself and
`shared`.

### On the Pi

1. Wire the AM2302/DHT22 as listed under Hardware.
2. Give each service its own environment with only the packages it needs. Run from
   `projects/weather-station/python`:

   ```bash
   python3 -m venv .venv-sensors && .venv-sensors/bin/pip install ./shared ./sensors
   python3 -m venv .venv-web && .venv-web/bin/pip install ./shared ./web
   python3 -m venv .venv-notifications && .venv-notifications/bin/pip install ./shared ./notifications
   cp -n .env.example .env
   ```

3. Set the SMTP values in `.env`. For Gmail, use an app password, not the account password.
4. Copy the units from `deploy/` to `/etc/systemd/system/`. Edit `User` and the paths if the repo
   isn't at `/home/pi/embedded`.
5. Start everything:

   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable --now weather-sensors.service weather-web.service weather-digest.timer
   ```

6. Open `http://127.0.0.1:5000` on the Pi. To reach it from a phone, expose it with Tailscale Serve
   and restrict access with tailnet ACLs. No public listener is needed.

To run a service by hand instead, activate its environment and run `python -m weather_station.sensors`,
`waitress-serve --call --host=127.0.0.1 --port=5000 weather_station.web.app:create_app`, or
`python -m weather_station.notifications`. Each one reads `.env` from the working directory.

## Configuration

| Variable | Used by | Default |
| --- | --- | --- |
| `DATABASE_PATH` | All | `instance/weather.sqlite3`, relative to the working directory |
| `STALE_AFTER_MINUTES` | web | `45`. A sensor with no reading in this window shows as stale. |
| `SENSOR_ID` | sensors | `room-am2302` |
| `GPIO_PIN` | sensors | `4` (BCM) |
| `INTERVAL_SECONDS` | sensors | `900` |
| `SMTP_HOST`, `SMTP_PORT`, `SMTP_USER`, `SMTP_PASSWORD`, `EMAIL_TO` | notifications | Port `465`, the others required |

## Hardware

| AM2302/DHT22 pin | Raspberry Pi |
| --- | --- |
| Data | BCM GPIO 4, physical pin 7 |
| VCC | 3.3 V |
| GND | Ground |

Add a 4.7 kΩ pull-up between data and 3.3 V if the sensor board doesn't have one. Never connect a
5 V data signal to a Pi GPIO. The account running the sensors service needs GPIO access. The
AM2302 datasheet is in `docs/datasheets/AM2302-EN.pdf`.

## How it works

The sensors service registers its sensor at startup, then reads it every `INTERVAL_SECONDS`. Each
adapter returns validated `Measurement` records in SI units with QUDT unit URIs. `RecordingService`
checks each record against the sensor catalog before the writer stores it, and a failed cycle is
logged without stopping the loop.

The web service reads the same file through the read-only reader. The dashboard uses the
shadcn/ui neutral palette, refreshes every 5 minutes, shows times in the Pi's local time zone,
and marks a sensor stale when it has no reading within `STALE_AFTER_MINUTES`.

| Route | Returns |
| --- | --- |
| `GET /` | Dashboard: latest values and a 30-day trend chart per property |
| `GET /api/readings` | Latest reading for every sensor and property |
| `GET /api/series/<sensor_id>?property=<key>` | 30 days of one property |
| `GET /api/sensor-types` | The sensor and property catalog |

Errors use one JSON envelope with `error.code`, `error.message`, and optional `error.details`. A
missing database or an unsupported schema version returns 503.

At 07:00 the digest timer runs the notifications package once. It sends one email listing every
sensor's latest value and its 24-hour minimum and maximum, converted to display units (°C, %, hPa).
Delivery goes through a `Notifier` interface, so another channel, such as ntfy, is one more class.

## Sensor adapters

| Adapter | Measures | Notes |
| --- | --- | --- |
| AM2302/DHT22 | Temperature, humidity | Used by the collector |
| BME280 | Temperature, humidity, pressure | |
| SGP30 | TVOC, equivalent CO₂ | Needs its required sampling cadence, warm-up, and baseline handling |
| Pulse anemometer | Wind speed, rolling gust | Calibrate the speed factor |
| Analog wind vane | Wind direction | ADS1115 input. Calibrate the voltage-to-direction map. |
| Tipping-bucket rain gauge | Accumulated rainfall, rate | Calibrate `mm_per_tip` |
| VEML6075 | UV index | |
| BH1750 | Illuminance | |
| DS18B20 | Probe temperature | Enable 1-Wire in Raspberry Pi OS first |
| Analog soil moisture | Soil moisture | ADS1115 input. Calibrate dry and wet. |
| AS3935 | Lightning count, distance | Scaffold only. Configure the interrupt for the exact breakout. |

Before enabling more adapters, verify each breakout's voltage, I²C address, and library API. Share
one ADS1115 instance between the wind vane and soil moisture adapters. GPIO pulse inputs need
pull-ups, debounce, and outdoor surge protection, and a hardware counter if pulse rates outrun
Python callbacks.

To change the dashboard styles, edit `web/tailwind/input.css` or the template, then rebuild the CSS
from `python/web` with `npm install` and `npm run build:css`. Commit the rebuilt
`static/dashboard.css`.

## Folders

- `python/`: the four packages, `requirements-dev.txt`, `mypy.ini`, `.env.example`, and the
  boundary test in `tests/`
- `deploy/`: systemd units for the sensors and web services and the digest timer
- `docs/datasheets/`: sensor datasheets

## Status

The collector supports one AM2302, and the dashboard and digest read its readings. All 23 tests
and the strict type check pass off-device. The other adapters are written but not yet run on
hardware.
