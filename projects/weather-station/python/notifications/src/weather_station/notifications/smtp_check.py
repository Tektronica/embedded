import argparse
import smtplib
import socket
import ssl
from collections.abc import Sequence
from pathlib import Path

from weather_station.notifications.contracts import Notification
from weather_station.notifications.email import EmailNotifier
from weather_station.notifications.settings import SmtpSettings, load_smtp_settings
from weather_station.shared.errors import WeatherStationError
from weather_station.shared.settings import DEFAULT_CONFIG_PATH


def main(argv: Sequence[str] | None = None, config_path: Path = DEFAULT_CONFIG_PATH) -> int:
    args = _parse_args(argv)
    try:
        settings = load_smtp_settings(config_path)
    except WeatherStationError as error:
        print(f"Configuration error: {error}")
        return 1

    try:
        if args.send:
            _send(settings)
        else:
            _check(settings)
    except smtplib.SMTPAuthenticationError:
        print("Authentication failed. Check SMTP_USER and SMTP_PASSWORD.")
        return 1
    except ssl.SSLError:
        print(f"TLS error with {settings.host}:{settings.port}")
        return 1
    except OSError:
        print(f"Could not connect or timed out: {settings.host}:{settings.port}")
        return 1
    except smtplib.SMTPException as error:
        print(f"SMTP error: {error}")
        return 1
    return 0


def _parse_args(argv: Sequence[str] | None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check the SMTP settings. Sends nothing unless --send is given.")
    parser.add_argument("--send", action="store_true", help="also send a test email to every recipient")
    return parser.parse_args(argv)


def _check(settings: SmtpSettings) -> None:
    with smtplib.SMTP_SSL(settings.host, settings.port, timeout=30) as smtp:
        smtp.login(settings.username, settings.password)
    print(f"Logged in to {settings.host}:{settings.port} as {settings.username}")


def _send(settings: SmtpSettings) -> None:
    notification = Notification(
        subject="Weather station test email",
        body=f"Sent by smtp_check from {socket.gethostname()}.\n",
    )
    EmailNotifier(settings).send(notification)
    count = len(settings.recipients)
    print(f"Sent a test email to {count} recipient{'' if count == 1 else 's'}")


if __name__ == "__main__":
    raise SystemExit(main())
