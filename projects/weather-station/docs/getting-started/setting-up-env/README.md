# Set Up Python Environments

Run commands from `projects/weather-station/python`. The packages are installed separately so hardware dependencies stay out of the web and notification environments.

`init.sh` rebuilds the environments and creates `.env` and `config.json` from their templates when they're missing. It uses `uv` when installed, which honors `.python-version`, and falls back to `python3 -m venv` with Python 3.12 or newer. Each run deletes and recreates the named environments.

```bash
./init.sh services                  # .venv-sensors, .venv-web, .venv-notifications
./init.sh services web              # one service only
./init.sh dev                       # .venv, every package editable plus test dependencies
```

To set up manually instead:

```bash
python3 -m venv .venv-sensors
python3 -m venv .venv-web
python3 -m venv .venv-notifications

.venv-sensors/bin/python -m pip install ./shared ./sensors
.venv-web/bin/python -m pip install ./shared ./web
.venv-notifications/bin/python -m pip install ./shared ./notifications
```

For development, use one environment with the packages editable and test dependencies installed:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements-dev.txt
```

Copy the templates only when there isn't already a local file:

```bash
cp -n .env.example .env
cp -n config.example.json config.json
```

`.env` holds only `SMTP_USER` and `SMTP_PASSWORD`. Edit it in a local editor. Keep SMTP credentials in `.env`, not in `config.json` or source control. `python-dotenv` loads `.env` from the service working directory; `.env.local` is not loaded automatically. See [the existing-installation upgrade guide](../upgrading-existing-installation/README.md) before replacing or modifying a populated `.env`.

`config.json` holds everything else. It is read from the working directory and ignored by Git:

| Key | Read by | Default |
| --- | --- | --- |
| `storage.database_path` | all three services | `instance/weather.sqlite3` |
| `sensors.interval_seconds` | sensors | 60 |
| `sensors.retention_days` | sensors | 30 |
| `sensors.devices` | sensors | empty list |
| `status.stale_after_minutes` | web | 5 |
| `email.smtp_host`, `email.recipients` | notifications | required |
| `email.smtp_port` | notifications | 465 |

Unknown sections and keys stop the service at startup, so a typo is reported instead of ignored. `storage.database_path` is relative to the shared working directory. systemd units set that directory to the Python project root so all three services use the same SQLite file.
