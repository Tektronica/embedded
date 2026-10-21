# Alarms

`weather-alarm.timer` runs `weather-alarm.service` every minute. Each run reads the latest reading of every sensor named in `alarms`, compares it with the rules in `config.json`, and sends one email when something changed. The timer runs every minute and journald logs each run.

An alarm checks two things:

- A threshold: the latest value of one property is above or below a limit.
- Staleness: a sensor has stopped reporting.

## Rules

Each entry in the top-level `alarms` list is one rule for one sensor and property:

```json
"alarms": [
  { "sensor": "room-am2302", "property": "air-temperature", "above": 35.0, "below": 0.0, "hysteresis": 1.0 }
]
```

| Key | Meaning | Default |
| --- | --- | --- |
| `sensor` | Sensor id from `sensors.devices` | required |
| `property` | Property key, such as `air-temperature` or `relative-humidity` | required |
| `above` | Alarm when the value passes this limit upward | none |
| `below` | Alarm when the value passes this limit downward | none |
| `hysteresis` | Distance back past a limit before the alarm clears | 0 |

Set `above`, `below`, or both. Limits and `hysteresis` use the default display units: °C for temperature, hPa for pressure. They stay in °C when `display.temperature_unit` is `"fahrenheit"`. Each sensor and property pair can appear once. When both limits are set, `above` minus `below` must exceed twice `hysteresis`. An invalid rule stops the run and the unit shows `failed`.

## Hysteresis

The alarm triggers when the value passes a limit and clears only after the value moves `hysteresis` back. In between, the alarm keeps its state. This stops a reading that hovers around a limit from sending an email every minute.

With `above` at 35.0 and `hysteresis` at 1.0:

| Reading | State | Email |
| --- | --- | --- |
| 34.9 | normal | none |
| 35.1 | above | "above 35.0" |
| 34.5 | above | none |
| 34.0 | above | none |
| 33.9 | normal | "back below 34.0" |

`below` works the same way in reverse. With `below` at 0.0 and `hysteresis` at 1.0, the alarm triggers under 0.0 and clears over 1.0. A first run that finds a value already past a limit triggers.

## Stale sensors

Every sensor named in a rule is also checked for staleness. A sensor is stale when its newest reading is older than `status.stale_after_minutes`, or when the database has no readings for it. The dashboard uses the same setting. A sensor id that the database doesn't know counts as stale.

The email goes out once when the sensor turns stale and once when it reports again. While the sensor is stale, its threshold alarms keep their state and are not checked against old data.

## Emails

One run sends at most one email. It lists every change found in that run, one line per change, and sends nothing when nothing changed. The subject is `Weather station alarm`, with the event count added when there is more than one, such as `Weather station alarm: 2 events`. Times use the Pi's local time zone.

```
room-am2302 temperature above 35.0 °C: now 36.2 °C (2026-10-21 14:03 EST)
room-am2302 stale: no reading since 2026-10-21 13:40 EST
```

Alarm and digest emails follow `display.temperature_unit` from `config.json` and ignore the dashboard toggle. With `"fahrenheit"`, the numbers and units in the email are converted. Comparisons against the limits still run in °C, so the email shows the limit converted too:

```
room-am2302 temperature above 95.0 °F: now 97.2 °F (2026-10-21 14:03 EST)
```

Emails go to `email.recipients` through the same account as the [digest](../email-digest/README.md). If sending fails, the run logs the error and exits with a failure. It keeps the old state, so the next run finds the same changes and tries again.

A run with nothing to report needs no SMTP settings. `SMTP_USER`, `SMTP_PASSWORD`, `email.smtp_host`, and `email.recipients` are read only when an email must go out. A fresh install with no `alarms` rules runs cleanly every minute.

## State

The alarm state is saved in `alarm-state.json`, in the same folder as the database (`instance/` by default). It records which limits are active and which sensors are stale. Deleting the file resets the alarms, and the next run triggers again for anything still past a limit. A missing or unreadable file is treated as empty and logged as a warning. State for rules removed from `config.json` is dropped on the next run.

## Turn alarms off or on

systemd decides whether alarms run. The notifications package has no on/off setting of its own.

```bash
sudo systemctl disable --now weather-alarm.timer
sudo systemctl enable --now weather-alarm.timer
```

`deploy.sh` keeps the timer in whichever state it finds. A first install turns it on.

## Run one check now

```bash
sudo systemctl start weather-alarm.service
sudo journalctl -u weather-alarm.service -n 20 --no-pager
```
