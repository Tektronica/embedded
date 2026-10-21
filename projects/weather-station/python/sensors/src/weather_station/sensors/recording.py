from datetime import datetime

from weather_station.sensors.contracts import MeasurementWriter
from weather_station.sensors.errors import InvalidSensorId, SensorNotRegistered
from weather_station.shared.errors import UnknownSensorType
from weather_station.shared.measurements import Measurement
from weather_station.shared.registry import get_sensor_definition, validate_measurement


class RecordingService:
    """Registers sensors and validates readings against the catalog before they reach the writer."""

    def __init__(self, writer: MeasurementWriter) -> None:
        self._writer = writer

    def register_sensor(self, sensor_id: str, sensor_type: str) -> None:
        if not sensor_id or sensor_id != sensor_id.strip():
            raise InvalidSensorId("sensor_id must be non-empty and have no surrounding whitespace")
        get_sensor_definition(sensor_type)
        self._writer.register_sensor(sensor_id, sensor_type)

    def record_many(self, measurements: list[Measurement]) -> None:
        validated = [
            validate_measurement(self._sensor_type(item.sensor_id), item) for item in measurements
        ]
        self._writer.save_many(validated)

    def prune_observed_before(self, cutoff: datetime) -> int:
        return self._writer.delete_observed_before(cutoff)

    def _sensor_type(self, sensor_id: str) -> str:
        sensor_type = self._writer.sensor_type(sensor_id)
        if sensor_type is None:
            raise SensorNotRegistered(f"Sensor {sensor_id!r} is not registered")
        try:
            get_sensor_definition(sensor_type)
        except UnknownSensorType as error:
            raise SensorNotRegistered(f"Sensor {sensor_id!r} has an unknown type") from error
        return sensor_type
