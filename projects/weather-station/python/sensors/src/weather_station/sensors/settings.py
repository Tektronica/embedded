import os
from dataclasses import dataclass
from pathlib import Path

from weather_station.shared.settings import database_path, load_env


@dataclass(frozen=True)
class CollectorSettings:
    database_path: Path
    sensor_id: str
    gpio_pin: int
    interval_seconds: int

    @classmethod
    def from_env(cls) -> "CollectorSettings":
        load_env()
        return cls(
            database_path=database_path(),
            sensor_id=os.getenv("SENSOR_ID", "room-am2302"),
            gpio_pin=int(os.getenv("GPIO_PIN", "4")),
            interval_seconds=int(os.getenv("INTERVAL_SECONDS", "900")),
        )
