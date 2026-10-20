# Email Digest

`weather-digest.timer` runs `weather-digest.service` every day at 07:00. The service sends one email with each sensor's latest reading and its range over the last 24 hours, then exits. It reads SMTP settings from `.env` in `projects/weather-station/python`.

## Turn the digest off or on

systemd decides whether the digest runs. The notifications package has no on/off setting of its own.

```bash
sudo systemctl disable --now weather-digest.timer
sudo systemctl enable --now weather-digest.timer
```

`deploy.sh` keeps the timer in whichever state it finds. A first install turns it on.

## Send one now

```bash
sudo systemctl start weather-digest.service
sudo journalctl -u weather-digest.service -n 20 --no-pager
```

## Without SMTP settings

The digest needs `SMTP_HOST`, `SMTP_USER`, `SMTP_PASSWORD`, and `EMAIL_TO`. If they're missing or still hold the `.env.example` placeholders, each run fails and the unit shows `failed`. The sensors and web services don't read SMTP settings and keep running. Turn the timer off until the settings are filled in. See [Troubleshooting](../troubleshooting/README.md#digest-email-fails).
