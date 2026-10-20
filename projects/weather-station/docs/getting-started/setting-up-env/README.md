# Set Up Python Environments

Run commands from `projects/weather-station/python`. The packages are installed separately so hardware dependencies stay out of the web and notification environments.

`init.sh` rebuilds the environments and creates `.env` from the template when it's missing. It uses `uv` when installed, which honors `.python-version`, and falls back to `python3 -m venv` with Python 3.12 or newer. Each run deletes and recreates the named environments.

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

Copy the template only when there isn't already a local file:

```bash
cp -n .env.example .env
```

Edit `.env` in a local editor. Keep SMTP credentials in `.env`, not in `sensors.toml` or source control. `python-dotenv` loads `.env` from the service working directory; `.env.local` is not loaded automatically. See [the existing-installation upgrade guide](../upgrading-existing-installation/README.md) before replacing or modifying a populated `.env`.

`DATABASE_PATH` is relative to the shared working directory. systemd units set that directory to the Python project root so all three services use the same SQLite file.
