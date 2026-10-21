# Set Up Web Server

The web package is read-only. It serves the dashboard and API from the SQLite file written by the sensors service.

## Run manually

From `projects/weather-station/python`, with `.venv-web` installed:

```bash
.venv-web/bin/waitress-serve --call --host=0.0.0.0 --port=5000 weather_station.web.app:create_app
```

Open `http://127.0.0.1:5000` on the Pi, or see [Access on the local network](#access-on-the-local-network) for other devices.

## Install as a service

`deploy.sh` installs this unit with the others. See [Install on Raspberry Pi](../installing-on-rpi/README.md). To install it alone, copy `deploy/weather-web.service` to `/etc/systemd/system/`, update `User`, `WorkingDirectory`, and `ExecStart` if the checkout isn't at `/home/pi/embedded`, then reload and start it:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now weather-web.service
sudo systemctl status weather-web.service
```

The sensors service should be started first so it can create the database. The web service returns a structured 503 if the database doesn't exist or has an unsupported schema version. See [Troubleshooting](../../operations/troubleshooting/README.md).

## Access on the local network

The dashboard listens on port 5000 on every interface, so any device on the same network as the Pi can open it:

```
http://<hostname>.local:5000
```

Some Android browsers don't resolve `.local` names. Use the Pi's IP address instead, which `hostname -I` prints on the Pi. Reserve that address for the Pi in the router's DHCP settings so the URL doesn't change.

The dashboard has no login, so every device on the network can read it. Away from the local network, rely on the [email digest](../../operations/email-digest/README.md). Never forward port 5000 on the router: that publishes the dashboard to the internet without authentication or TLS.
