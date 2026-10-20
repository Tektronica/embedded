# Set Up Raspberry Pi

## Requirements

- Raspberry Pi running Raspberry Pi OS
- Python 3.12 or newer and the `venv` module
- Network access for package installation and SMTP delivery
- An AM2302/DHT22 wired as described in [Wiring](../../hardware-configuration/wiring/README.md)
- A login user with GPIO access

Check Python before installing the application:

```bash
python3 --version
python3 -m venv --help
```

If either check fails, install a supported Python and venv package using the instructions for the installed Raspberry Pi OS release. The sensors use Adafruit Blinka and GPIO libraries; follow the current [Adafruit DHT Raspberry Pi setup guide](https://docs.circuitpython.org/projects/dht/en/latest/) for OS-level GPIO requirements and permissions. Don't run the service with `sudo` merely to work around missing GPIO access.

Install the packages that build `lgpio`. PyPI has no `lgpio` wheel for the Pi, so pip builds it from source:

```bash
sudo apt install python3-dev python3-venv swig liblgpio-dev
```

Continue with [Install on Raspberry Pi](../installing-on-rpi/README.md).
