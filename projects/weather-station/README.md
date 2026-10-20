# Weather Station

A Raspberry Pi weather station with a headless sensor collector, SQLite history, a local Flask dashboard, and optional sensor adapters. Each adapter emits the same validated `Measurement` record; sensor type and property catalogs describe allowed relationships and display metadata. The current collector polls an AM2302/DHT22 sensor; the other adapters are not yet enabled by the collector.

## Platform

- Raspberry Pi OS
- Python 3.12+
- AM2302/DHT22 sensor for the included runner
- SMTP account with an app password or equivalent credentials

The AM2302 datasheet is in [docs/datasheets](docs/datasheets/AM2302-EN.pdf).

## Setup

Connect the AM2302/DHT22 data pin to Raspberry Pi BCM GPIO 4 (physical pin 7), VCC to 3.3 V, and GND to a ground pin. Add a 4.7 kΩ pull-up between data and 3.3 V if the sensor board does not already include one. Do not connect a 5 V data signal to a Pi GPIO.

Use Raspberry Pi OS with Python 3.12 or newer. Check `python3 --version` first; install a supported Python version and the venv package using the instructions for your OS release if needed. The account running the collector must have GPIO access; follow the current Adafruit DHT setup guidance for Raspberry Pi OS permissions and GPIO support.

Run these commands from the repository root:

```bash
cd projects/weather-station/python
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
cp -n .env.example .env
```

Set `SMTP_HOST`, `SMTP_PORT`, `SMTP_USER`, `SMTP_PASSWORD`, and `EMAIL_TO` in `.env`. For Gmail, use an app password rather than your account password. `GPIO_PIN` defaults to BCM GPIO 4, `SENSOR_ID` to `room-am2302`, `INTERVAL_SECONDS` to 900, and `DATABASE_PATH` to `instance/weather.sqlite3`. Keep local environment files out of version control.

## Run

The collector and web server are separate processes. Start each from the `python/` directory in its own terminal, with the virtual environment activated.

```bash
python -m collector.run
```

Start the web dashboard in the second terminal:

```bash
waitress-serve --call --host=127.0.0.1 --port=5000 web.app:create_app
```

Open `http://127.0.0.1:5000`. The dashboard is bound to loopback by default. To access it from a phone on your tailnet, expose the local service with Tailscale Serve and restrict access with your tailnet ACLs. No public listener is required.

The collector logs each saved reading and emails it using the SMTP settings. Press Ctrl+C in its terminal to stop it; the sensor is closed on exit. The dashboard shows the latest stored reading and up to 30 days of history. Sensors register through `POST /api/sensors` with `sensor_id` and `sensor_type`; repeating a registration is safe, while rebinding an ID to another type returns a conflict. `POST /api/readings` accepts the common measurement fields (`observed_at`, `sensor_id`, `property_id`, `value`, `unit`, and optional `quality`/`uncertainty`). The service checks the registered sensor type and canonical QUDT unit before persistence. API failures use a stable JSON envelope with `error.code`, `error.message`, and optional `error.details`.

## Check

Run from the `python/` directory with the virtual environment activated:

```bash
python -m unittest discover -s tests -v
mypy --strict domain dal collector services web sensors transports
```

The tests mock sensor hardware and exercise the Flask API against temporary SQLite databases, so they do not need a Pi or attached sensors. No systemd unit is included yet.

## Sensor Adapters

Each adapter returns one or more validated `Measurement` records and exposes `close()` for cleanup. `domain/` owns the common record and read-only sensor/property catalogs; `services/` owns registration, validation, and use cases; `collector/` owns polling; `dal/` owns SQLite persistence; and `web/` owns thin Flask routes and assemblers for API/dashboard shapes.

- AM2302/DHT22: temperature and humidity; used by the current runner
- BME280: temperature, humidity, pressure
- SGP30: TVOC and equivalent CO₂
- Generic pulse anemometer: wind speed and rolling gust maximum
- Analog wind vane: ADS1115 input, requires sensor-specific voltage-to-direction calibration
- Tipping-bucket rain gauge: accumulated rainfall and interval-derived rate
- VEML6075: UVA, UVB, UV index
- BH1750: illuminance
- DS18B20: probe temperature
- Analog soil moisture: ADS1115 input, requires dry/wet calibration
- AS3935: lightning sensor readout scaffold

These adapters are building blocks, not a sensor discovery/registry or a complete scheduler. Before deployment:

- Verify breakout-board models, voltage compatibility, I²C addresses, and library APIs.
- Share one ADS1115 instance between the analog wind-vane and soil-moisture adapters.
- Calibrate the wind-vane voltage mapping, wind-speed factor, and rain `mm_per_tip` for the actual hardware.
- Provide suitable pull-ups, debounce, and outdoor surge protection for GPIO pulse inputs. Use a hardware counter if pulse rates exceed what Python callbacks can reliably capture.
- Configure the AS3935 interrupt and event decoding for the exact breakout/library version; its adapter is currently a scaffold.
- Run SGP30 measurements at the required cadence and handle warm-up/baseline persistence for production use.
- Enable 1-Wire in Raspberry Pi OS before using a DS18B20.
