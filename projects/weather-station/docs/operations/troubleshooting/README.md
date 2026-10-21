# Troubleshoot

## Sensor not detected

Check the `sensors.devices` model and options in `config.json`, wiring, power, I²C address, GPIO permissions, and Raspberry Pi interfaces such as 1-Wire. Sensor configuration errors stop startup; a hardware read failure logs `Not detected` and skips that sensor. Inspect logs with:

```bash
sudo journalctl -u weather-sensors.service -b --no-pager
```

After changing hardware or `config.json`, restart the collector:

```bash
sudo systemctl restart weather-sensors.service
```

## Web shows no data or returns 503

Check that the sensors service started and initialized the database, then verify all services use the same `storage.database_path` in `config.json` and the same `WorkingDirectory`:

```bash
sudo systemctl status weather-sensors.service weather-web.service
sudo journalctl -u weather-web.service -b --no-pager
```

The web and digest packages open SQLite read-only and reject missing or unsupported schema versions. Start the sensors writer first.

## Digest email fails

Verify `email.smtp_host`, `email.smtp_port`, and `email.recipients` in `config.json`, and `SMTP_USER` and `SMTP_PASSWORD` in `.env`, then inspect:

```bash
sudo systemctl start weather-digest.service
sudo journalctl -u weather-digest.service -b --no-pager
```

Don't print or paste SMTP secrets into logs or issue reports.

## Alarm email fails

Alarms use the same email settings as the digest. A failed send leaves the alarm state unchanged, so the next run tries again. Inspect:

```bash
sudo systemctl start weather-alarm.service
sudo journalctl -u weather-alarm.service -b --no-pager
```

See [Alarms](../alarms/README.md) for the rule format and state file.
