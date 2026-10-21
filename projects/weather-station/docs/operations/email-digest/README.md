# Email Digest

`weather-digest.timer` runs `weather-digest.service` every day at 07:00. The service sends one email, then exits. For each property it lists the latest reading, the 24-hour min, mean, and max, the change since the same time yesterday, and a 7-day daily high and low. It reads `email` settings from `config.json` and `SMTP_USER` and `SMTP_PASSWORD` from `.env`, both in `projects/weather-station/python`.

## Set up Gmail

The digest connects to Gmail with implicit TLS on port 465 and signs in with an app password. An app password is a 16-character secret Google generates for one device. It isn't the account password and can be revoked on its own, but it grants access to the whole mailbox. Use a Gmail account dedicated to the station so the Pi holds no access to a personal mailbox.

1. Turn on 2-Step Verification for the account at `https://myaccount.google.com/security`. Google doesn't offer app passwords until it's on.
2. Open `https://myaccount.google.com/apppasswords`, enter an app name such as the Pi's hostname, and create the password. Google shows it once.
3. On the Pi, set the credentials in `.env`, with the app password's spaces removed:

   ```
   SMTP_USER=<gmail address>
   SMTP_PASSWORD=<app password>
   ```

4. Set the server and recipients in `config.json`:

   ```json
   "email": {
     "smtp_host": "smtp.gmail.com",
     "smtp_port": 465,
     "recipients": ["<recipient>"]
   }
   ```

5. Restrict the file to its owner:

   ```bash
   chmod 600 .env
   ```

`email.recipients` takes any address on any provider. List several as separate strings. The digest goes to every recipient and each sees the others. To revoke the Pi's access, delete its app password on the same page.

Providers that accept only STARTTLS on port 587 don't work, because the digest uses implicit TLS.

## Check the SMTP settings

Run these from `projects/weather-station/python`. The default check logs in and quits. It sends nothing.

```bash
.venv-notifications/bin/python -m weather_station.notifications.smtp_check
```

Each run ends with one line and exits 0 on success or 1 on failure:

- `Logged in to <host>:<port> as <user>`: the server accepted `SMTP_USER` and `SMTP_PASSWORD`.
- `Authentication failed. Check SMTP_USER and SMTP_PASSWORD.`: the address or app password is wrong, or the app password was revoked.
- `Could not connect or timed out: <host>:<port>`: the Pi can't reach the server, or the host or port is wrong.
- `TLS error with <host>:<port>`: the server doesn't accept implicit TLS on that port. Use port 465.
- `Configuration error: <detail>`: a key in `config.json` or a variable in `.env` is missing or invalid. The detail names it.
- `SMTP error: <detail>`: the server refused or ended the session.

Add `--send` to deliver one test email to every entry in `email.recipients`:

```bash
.venv-notifications/bin/python -m weather_station.notifications.smtp_check --send
```

On success the command prints `Sent a test email to <n> recipients`, or `1 recipient` for one address. The subject is `Weather station test email`. The body names the Pi's hostname.

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

The digest needs `email.smtp_host` and `email.recipients` in `config.json`, and `SMTP_USER` and `SMTP_PASSWORD` in `.env`. If they're missing or still hold the example placeholders, each run fails and the unit shows `failed`. The sensors and web services don't read SMTP settings and keep running. Turn the timer off until the settings are filled in. See [Troubleshooting](../troubleshooting/README.md#digest-email-fails).
