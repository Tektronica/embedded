import os
from dataclasses import dataclass
from pathlib import Path

from weather_station.shared.settings import database_path, load_env


@dataclass(frozen=True)
class CollectorSettings:
    database_path: Path
    sensors_config_path: Path
    interval_seconds: int
    retention_days: int

    @classmethod
    def from_env(cls) -> "CollectorSettings":
        load_env()
        return cls(
            database_path=database_path(),
            sensors_config_path=Path(os.getenv("SENSORS_CONFIG", "sensors.toml")),
            interval_seconds=int(os.getenv("INTERVAL_SECONDS", "60")),
            retention_days=int(os.getenv("RETENTION_DAYS", "30")),
        )
