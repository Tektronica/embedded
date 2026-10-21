# Startup email

`weather-boot.service` sends one email, "Weather station started", each time the Pi boots. It tells you the power came back and how long the station was offline.

## What it sends

The email lists the start time, the newest reading stored before that start, and the gap between them:

```
The weather station started at 2026-10-21 14:00 EST.
Last reading before the start: 2026-10-21 10:48 EST (room-am2302).
Offline for about 3 h 12 min.
```

If the database holds no earlier reading, or the database file does not exist yet, the email says `No earlier reading was found.` Times use the Pi's local time zone. Emails go to `email.recipients` through the same account as the [digest](../email-digest/README.md).

## When it sends

The unit runs once per boot, after the network and the clock sync are up. `deploy.sh` enables the unit but does not start it, so a deploy never sends a false "started" email.

## How the gap is computed

The start time is the current time minus the system uptime from `/proc/uptime`. The last reading is the newest `observed_at` below that start time across all sensors. The gap is the difference, rounded to the minute and shown as minutes, hours, or days.

## Limits

- The Pi has no real-time clock. After a power outage the clock starts from the last saved time until NTP syncs.
- `weather-sensors.service` and `weather-boot.service` wait for `time-sync.target`. `systemd-time-wait-sync.service` holds that target until the clock syncs.
- The wait is capped at 2 minutes. A Pi with no network starts collecting after that, with an unsynced clock.
- An offline boot can still stamp readings and the start time with wrong times until the clock syncs. An RTC module removes the problem.
- The gap is only as correct as the clock at the time the email is built.

## Retries

If sending fails, the run logs the error and exits with a failure. systemd retries every 30 seconds for about 10 minutes (20 starts in 15 minutes). After that the unit stays `failed` until the next boot.

```bash
sudo journalctl -u weather-boot.service -b --no-pager
```

## Turn it off or on

systemd decides whether the email sends. The notifications package has no on/off setting of its own.

```bash
sudo systemctl disable weather-boot.service
sudo systemctl enable weather-boot.service
```

`deploy.sh` keeps the unit in whichever state it finds. A first install turns it on.

## Send one now

```bash
sudo systemctl start weather-boot.service
```

The email reports the real start time of the current boot.
