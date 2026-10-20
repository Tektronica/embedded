# Set Up Web Server

The web package is read-only. It serves the dashboard and API from the SQLite file written by the sensors service.

## Run manually

From `projects/weather-station/python`, with `.venv-web` installed:

```bash
.venv-web/bin/waitress-serve --call --host=127.0.0.1 --port=5000 weather_station.web.app:create_app
```

Open `http://127.0.0.1:5000`. The service binds to loopback, not the LAN. To access it from a phone, use Tailscale Serve and restrict access with tailnet ACLs; don't change the service to a public listener.

## Install as a service

Copy `deploy/weather-web.service` to `/etc/systemd/system/`, update `User`, `WorkingDirectory`, and `ExecStart` if the checkout isn't at `/home/pi/embedded`, then reload and start it:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now weather-web.service
sudo systemctl status weather-web.service
```

The sensors service should be started first so it can create the database. The web service returns a structured 503 if the database doesn't exist or has an unsupported schema version. See [Troubleshooting](../../operations/troubleshooting/README.md).
