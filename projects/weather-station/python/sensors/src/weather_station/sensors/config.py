from dataclasses import dataclass, field
from typing import Any

from weather_station.sensors.errors import InvalidSensorConfiguration


@dataclass(frozen=True)
class SensorConfig:
    """One entry of sensors.devices in config.json: an id, an adapter model, and that model's options."""

    sensor_id: str
    model: str
    options: dict[str, Any] = field(default_factory=dict)


def parse_sensor_configs(devices: Any) -> list[SensorConfig]:
    if not isinstance(devices, list):
        raise InvalidSensorConfiguration("Config key 'sensors.devices' must be an array of objects")

    configs: list[SensorConfig] = []
    for device in devices:
        if not isinstance(device, dict):
            raise InvalidSensorConfiguration("Each entry in 'sensors.devices' must be a JSON object")
        options = dict(device)
        sensor_id = options.pop("id", None)
        model = options.pop("model", None)
        if not isinstance(sensor_id, str) or not sensor_id or sensor_id != sensor_id.strip():
            raise InvalidSensorConfiguration(f"Sensor entry {device!r} needs a non-empty id")
        if not isinstance(model, str):
            raise InvalidSensorConfiguration(f"Sensor {sensor_id!r} needs a model")
        configs.append(SensorConfig(sensor_id, model, options))
    ids = [config.sensor_id for config in configs]
    if len(ids) != len(set(ids)):
        raise InvalidSensorConfiguration("Sensor ids in the configuration must be unique")
    return configs
