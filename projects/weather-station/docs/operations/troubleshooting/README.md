# Troubleshoot

## Sensor not detected

Check `sensors.toml` model/options, wiring, power, I²C address, GPIO permissions, and Raspberry Pi interfaces such as 1-Wire. Sensor configuration errors stop startup; a hardware read failure logs `Not detected` and skips that sensor. Inspect logs with:

```bash
sudo journalctl -u weather-sensors.service -b --no-pager
```

After changing hardware or `sensors.toml`, restart the collector:

```bash
sudo systemctl restart weather-sensors.service
```

## Web shows no data or returns 503

Check that the sensors service started and initialized the database, then verify all services use the same `DATABASE_PATH` and `WorkingDirectory`:

```bash
sudo systemctl status weather-sensors.service weather-web.service
sudo journalctl -u weather-web.service -b --no-pager
```

The web and digest packages open SQLite read-only and reject missing or unsupported schema versions. Start the sensors writer first.

## Digest email fails

Verify SMTP host, port, sender, app password, and recipient in the local `.env`, then inspect:

```bash
sudo systemctl start weather-digest.service
sudo journalctl -u weather-digest.service -b --no-pager
```

Don't print or paste SMTP secrets into logs or issue reports.
