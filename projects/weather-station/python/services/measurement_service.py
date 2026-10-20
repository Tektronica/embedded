from datetime import datetime
from uuid import UUID

from domain.contracts import MeasurementRepository
from domain.errors import InvalidSensorId, SensorNotRegistered, UnknownSensorType
from domain.measurements import Measurement, StoredMeasurement
from domain.registry import get_sensor_definition, validate_measurement


class MeasurementService:
    def __init__(self, repository: MeasurementRepository) -> None:
        self._repository = repository

    def register_sensor(self, sensor_id: str, sensor_type: str) -> None:
        if not sensor_id or sensor_id != sensor_id.strip():
            raise InvalidSensorId(
                "sensor_id must be non-empty and have no surrounding whitespace"
            )
        get_sensor_definition(sensor_type)
        self._repository.register_sensor(sensor_id, sensor_type)

    def record(self, measurement: Measurement) -> UUID:
        sensor_type = self._sensor_type(measurement.sensor_id)
        validate_measurement(sensor_type, measurement)
        return self._repository.save(measurement)

    def record_many(self, measurements: list[Measurement]) -> None:
        validated = []
        for measurement in measurements:
            sensor_type = self._sensor_type(measurement.sensor_id)
            validated.append(validate_measurement(sensor_type, measurement))
        self._repository.save_many(validated)

    def latest(self) -> list[StoredMeasurement]:
        return self._repository.latest()

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return self._repository.latest_for_sensor(sensor_id)

    def history(
        self,
        sensor_id: str,
        property_id: str,
        since: datetime,
    ) -> list[StoredMeasurement]:
        return self._repository.series(sensor_id, property_id, since)

    def registered_sensors(self) -> list[tuple[str, str]]:
        return self._repository.registered_sensors()

    def sensor_type(self, sensor_id: str) -> str | None:
        return self._repository.sensor_type(sensor_id)

    def _sensor_type(self, sensor_id: str) -> str:
        sensor_type = self._repository.sensor_type(sensor_id)
        if sensor_type is None:
            raise SensorNotRegistered(f"Sensor {sensor_id!r} is not registered")
        try:
            get_sensor_definition(sensor_type)
        except UnknownSensorType as error:
            raise SensorNotRegistered(
                f"Sensor {sensor_id!r} has an unknown type"
            ) from error
        return sensor_type