import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from weather_station.notifications.errors import InvalidEmailConfiguration, MissingSmtpCredential
from weather_station.shared.settings import (
    DEFAULT_CONFIG_PATH,
    database_path,
    load_config,
    load_env,
    read_int,
    read_section,
)

EMAIL_KEYS = frozenset({"smtp_host", "smtp_port", "recipients"})


@dataclass(frozen=True)
class SmtpSettings:
    host: str
    port: int
    username: str
    password: str
    recipients: list[str]


@dataclass(frozen=True)
class NotificationSettings:
    database_path: Path
    smtp: SmtpSettings

    @classmethod
    def from_config(cls, config_path: Path = DEFAULT_CONFIG_PATH) -> "NotificationSettings":
        load_env()
        email = read_section(
            load_config(config_path), "email", EMAIL_KEYS, InvalidEmailConfiguration
        )
        return cls(
            database_path=database_path(config_path),
            smtp=SmtpSettings(
                host=_read_host(email),
                port=read_int(email, "email", "smtp_port", 465, InvalidEmailConfiguration),
                username=_read_credential("SMTP_USER"),
                password=_read_credential("SMTP_PASSWORD"),
                recipients=_read_recipients(email),
            ),
        )


def _read_host(email: dict[str, Any]) -> str:
    host = email.get("smtp_host")
    if not isinstance(host, str) or not host:
        raise InvalidEmailConfiguration("Config key 'email.smtp_host' must be a non-empty string")
    return host


def _read_recipients(email: dict[str, Any]) -> list[str]:
    recipients = email.get("recipients")
    if (
        not isinstance(recipients, list)
        or not recipients
        or not all(isinstance(item, str) and item for item in recipients)
    ):
        raise InvalidEmailConfiguration(
            "Config key 'email.recipients' must be a non-empty array of strings"
        )
    return recipients


def _read_credential(name: str) -> str:
    value = os.getenv(name)
    if not value:
        raise MissingSmtpCredential(f"Environment variable {name} is not set")
    return value
