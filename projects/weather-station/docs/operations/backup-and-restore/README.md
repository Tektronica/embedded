# Back Up and Restore

Stop the writer before copying the SQLite file so the backup is consistent. From the repository root:

```bash
sudo systemctl stop weather-digest.timer weather-alarm.timer weather-web.service weather-sensors.service
cd projects/weather-station/python
cp .env .env.backup
cp config.json config.json.backup
cp instance/weather.sqlite3 instance/weather.sqlite3.backup
```

Copy the backups off the Pi. `.env` contains credentials, so store it privately and never commit it. The SQLite database can contain months of readings.

To restore, stop all three services, copy the saved files back to their original locations, and start `weather-sensors.service` before `weather-web.service`, `weather-digest.timer`, and `weather-alarm.timer`.
