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
