import os
from dataclasses import dataclass
from pathlib import Path

from weather_station.shared.settings import database_path, load_env


@dataclass(frozen=True)
class SmtpSettings:
    host: str
    port: int
    username: str
    password: str
    recipient: str


@dataclass(frozen=True)
class NotificationSettings:
    database_path: Path
    smtp: SmtpSettings

    @classmethod
    def from_env(cls) -> "NotificationSettings":
        load_env()
        return cls(
            database_path=database_path(),
            smtp=SmtpSettings(
                host=os.environ["SMTP_HOST"],
                port=int(os.getenv("SMTP_PORT", "465")),
                username=os.environ["SMTP_USER"],
                password=os.environ["SMTP_PASSWORD"],
                recipient=os.environ["EMAIL_TO"],
            ),
        )
