import tomllib
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from weather_station.sensors.errors import InvalidSensorConfiguration


@dataclass(frozen=True)
class SensorConfig:
    """One [[sensor]] block from sensors.toml: an id, an adapter model, and that model's options."""

    sensor_id: str
    model: str
    options: dict[str, Any] = field(default_factory=dict)


def load_sensor_configs(path: Path) -> list[SensorConfig]:
    if not path.exists():
        raise InvalidSensorConfiguration(f"No sensor configuration at {path}")
    with path.open("rb") as file:
        document = tomllib.load(file)
    configs: list[SensorConfig] = []
    for block in document.get("sensor", []):
        options = dict(block)
        sensor_id = options.pop("id", None)
        model = options.pop("model", None)
        if not isinstance(sensor_id, str) or not sensor_id or sensor_id != sensor_id.strip():
            raise InvalidSensorConfiguration(f"Sensor block {block!r} needs a non-empty id")
        if not isinstance(model, str):
            raise InvalidSensorConfiguration(f"Sensor {sensor_id!r} needs a model")
        configs.append(SensorConfig(sensor_id, model, options))
    ids = [config.sensor_id for config in configs]
    if len(ids) != len(set(ids)):
        raise InvalidSensorConfiguration("Sensor ids in the configuration must be unique")
    return configs
