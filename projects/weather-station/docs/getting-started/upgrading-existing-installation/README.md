# Upgrade Existing Installation

Use this guide when the Pi already has `.env`, SQLite data, or systemd services. Don't replace `.env` with the example: it may contain SMTP credentials.

## 1. Stop services and back up state

From the repository root, stop the timer and services before copying the database:

```bash
sudo systemctl stop weather-digest.timer weather-web.service weather-sensors.service
cd projects/weather-station/python
cp .env .env.backup
cp instance/weather.sqlite3 instance/weather.sqlite3.backup
cp sensors.toml sensors.toml.backup
```

The `.env` and database backups stay local and are ignored by Git. Keep a copy off the Pi as well if the database matters.

## 2. Update configuration

Compare the current `.env` with `.env.example` manually. Preserve `SMTP_HOST`, `SMTP_PORT`, `SMTP_USER`, `SMTP_PASSWORD`, and `EMAIL_TO`. Remove the obsolete `GPIO_PIN` and `SENSOR_ID` keys; sensor pins and IDs now live in `sensors.toml`. Add `SENSORS_CONFIG=sensors.toml` if it is missing. `cp -n .env.example .env` will not update an existing file.

Review the current `sensors.toml`; it is now the source of enabled sensor IDs, models, and options. Don't overwrite local calibrations without comparing them first.

## 3. Install the split packages

Run from `projects/weather-station/python`:

```bash
python3 -m venv .venv-sensors
python3 -m venv .venv-web
python3 -m venv .venv-notifications
.venv-sensors/bin/python -m pip install ./shared ./sensors
.venv-web/bin/python -m pip install ./shared ./web
.venv-notifications/bin/python -m pip install ./shared ./notifications
```

These are the current unit paths. If the matching environments already exist, back them up or rename them before rebuilding; don't install all hardware dependencies into the web/notifier environments.

## 4. Replace units and start in order

Return to the repository root before copying the units:

```bash
cd /home/pi/embedded
sudo cp projects/weather-station/deploy/weather-sensors.service /etc/systemd/system/
sudo cp projects/weather-station/deploy/weather-web.service /etc/systemd/system/
sudo cp projects/weather-station/deploy/weather-digest.service /etc/systemd/system/
sudo cp projects/weather-station/deploy/weather-digest.timer /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now weather-sensors.service
sudo systemctl status weather-sensors.service
sudo systemctl enable --now weather-web.service weather-digest.timer
```

Starting sensors first lets the writer initialize `schema_version` before the read-only services start.

## Database compatibility

The previous committed SQLite `sensors` and `measurements` columns match the current writer schema. The new writer creates `schema_version` and initializes it to the supported version when absent; it does not rebuild measurement rows. Keep the backup until the dashboard and digest have been checked.

## Roll back

Stop the services, restore `.env`, `sensors.toml`, and the SQLite backup, then restore the previous package environments and unit files. Run `systemctl daemon-reload` before starting the old services.
