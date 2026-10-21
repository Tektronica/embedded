import os
from dataclasses import dataclass
from datetime import timedelta
from pathlib import Path
from typing import Any

from weather_station.notifications.alarm_rules import AlarmRule, read_alarm_rules
from weather_station.notifications.errors import InvalidEmailConfiguration, MissingSmtpCredential
from weather_station.shared.settings import (
    DEFAULT_CONFIG_PATH,
    database_path,
    load_config,
    load_env,
    read_int,
    read_section,
    read_stale_after_minutes,
    read_unit_system,
)
from weather_station.shared.units import UnitSystem

EMAIL_KEYS = frozenset({"smtp_host", "smtp_port", "recipients"})
ALARM_STATE_FILENAME = "alarm-state.json"


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
    unit_system: UnitSystem

    @classmethod
    def from_config(cls, config_path: Path = DEFAULT_CONFIG_PATH) -> "NotificationSettings":
        return cls(
            database_path=database_path(config_path),
            smtp=load_smtp_settings(config_path),
            unit_system=read_unit_system(load_config(config_path)),
        )


@dataclass(frozen=True)
class AlarmSettings:
    database_path: Path
    state_path: Path
    stale_after: timedelta
    rules: list[AlarmRule]
    unit_system: UnitSystem

    @classmethod
    def from_config(cls, config_path: Path = DEFAULT_CONFIG_PATH) -> "AlarmSettings":
        document = load_config(config_path)
        path = database_path(config_path)
        return cls(
            database_path=path,
            state_path=path.parent / ALARM_STATE_FILENAME,
            stale_after=timedelta(minutes=read_stale_after_minutes(document)),
            rules=read_alarm_rules(document),
            unit_system=read_unit_system(document),
        )


def load_smtp_settings(config_path: Path = DEFAULT_CONFIG_PATH) -> SmtpSettings:
    load_env()
    email = read_section(load_config(config_path), "email", EMAIL_KEYS, InvalidEmailConfiguration)
    return SmtpSettings(
        host=_read_host(email),
        port=read_int(email, "email", "smtp_port", 465, InvalidEmailConfiguration),
        username=_read_credential("SMTP_USER"),
        password=_read_credential("SMTP_PASSWORD"),
        recipients=_read_recipients(email),
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
