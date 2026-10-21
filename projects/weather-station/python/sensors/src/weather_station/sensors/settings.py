from dataclasses import dataclass
from pathlib import Path

from weather_station.sensors.config import SensorConfig, parse_sensor_configs
from weather_station.sensors.errors import InvalidCollectorConfiguration
from weather_station.shared.settings import (
    DEFAULT_CONFIG_PATH,
    database_path,
    load_config,
    read_positive_int,
    read_section,
)

SENSORS_KEYS = frozenset({"interval_seconds", "retention_days", "devices"})


@dataclass(frozen=True)
class CollectorSettings:
    database_path: Path
    interval_seconds: int
    retention_days: int
    sensors: list[SensorConfig]

    @classmethod
    def from_config(cls, config_path: Path = DEFAULT_CONFIG_PATH) -> "CollectorSettings":
        section = read_section(
            load_config(config_path), "sensors", SENSORS_KEYS, InvalidCollectorConfiguration
        )
        return cls(
            database_path=database_path(config_path),
            interval_seconds=read_positive_int(
                section, "sensors", "interval_seconds", 60, InvalidCollectorConfiguration
            ),
            retention_days=read_positive_int(
                section, "sensors", "retention_days", 30, InvalidCollectorConfiguration
            ),
            sensors=parse_sensor_configs(section.get("devices", [])),
        )
