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

Compare the current `.env` with `.env.example` manually. Keep `SMTP_USER` and `SMTP_PASSWORD`. Every other key moves to `config.json`, described below. `cp -n .env.example .env` will not update an existing file.

### Move to config.json

Settings that were in `.env` and `sensors.toml` now live in `config.json`. Do this once on an existing Pi:

1. Create the file from the template: `cp -n config.example.json config.json`.
2. Replace the example entry in `sensors.devices` with the `[[sensor]]` blocks from `sensors.toml`. Each block becomes one JSON object with the same `id`, `model`, and options. Write I²C addresses in decimal, so `0x76` becomes `118`. See [Configure sensors](../../hardware-configuration/sensor-configuration/README.md).
3. Move `SMTP_HOST` and `SMTP_PORT` to `email.smtp_host` and `email.smtp_port`. Move `EMAIL_TO` to `email.recipients` as a list, one string per address.
4. Move `DATABASE_PATH`, `INTERVAL_SECONDS`, `RETENTION_DAYS`, and `STALE_AFTER_MINUTES` to `storage.database_path`, `sensors.interval_seconds`, `sensors.retention_days`, and `status.stale_after_minutes`. Skip any key that still has its default.
5. Delete the moved keys, `SENSORS_CONFIG`, and the obsolete `GPIO_PIN` and `SENSOR_ID` from `.env`. Only `SMTP_USER` and `SMTP_PASSWORD` stay.
6. Delete `sensors.toml` after checking that no local calibration was missed. Don't overwrite local calibrations without comparing them first.

## 3. Deploy

Run from `projects/weather-station/python`, as the user the services run as:

```bash
./deploy.sh
```

`deploy.sh` pulls, deletes and rebuilds `.venv-sensors`, `.venv-web`, and `.venv-notifications`, installs the units with `User` and paths set to the current user and checkout, and starts them. See [Install on Raspberry Pi](../installing-on-rpi/README.md#4-deploy) for the full sequence. Rename existing environments first if a rollback needs them.

`deploy.sh` starts sensors first, so the writer initializes `schema_version` before the read-only services start.

## Database compatibility

The previous committed SQLite `sensors` and `measurements` columns match the current writer schema. The new writer creates `schema_version` and initializes it to the supported version when absent; it does not rebuild measurement rows. Keep the backup until the dashboard and digest have been checked.

## Roll back

Stop the services, restore `.env`, `sensors.toml`, and the SQLite backup. The old services need `sensors.toml` and the old `.env` keys, so keep both backups until the upgrade is verified, then restore the previous package environments and unit files. Run `systemctl daemon-reload` before starting the old services.
