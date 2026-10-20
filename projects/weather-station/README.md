# Weather Station

A Raspberry Pi weather station with a sensor collector, SQLite history, a local Flask dashboard, and a daily email digest. Python services run as separate packages and systemd units. Start with the guide for the task you need:

## Getting started

- [Set up Raspberry Pi](docs/getting-started/setting-up-rpi/README.md)
- [Install on Raspberry Pi](docs/getting-started/installing-on-rpi/README.md)
- [Set up Python environments](docs/getting-started/setting-up-env/README.md)
- [Set up the web server](docs/getting-started/setting-up-web-server/README.md)
- [Upgrade an existing installation](docs/getting-started/upgrading-existing-installation/README.md)

## Hardware configuration

- [Choose sensors](docs/hardware-configuration/sensor-selection/README.md)
- [Configure sensors](docs/hardware-configuration/sensor-configuration/README.md)
- [Wire the AM2302](docs/hardware-configuration/wiring/README.md)

## Operations

- [Back up and restore](docs/operations/backup-and-restore/README.md)
- [Troubleshoot services and sensors](docs/operations/troubleshooting/README.md)

The AM2302 reference datasheet is in [docs/datasheets](docs/datasheets/AM2302-EN.pdf). The shipped `sensors.toml` enables the AM2302; other adapters need their hardware connected and configured before use.
