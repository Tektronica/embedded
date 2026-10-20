from datetime import datetime
from typing import Protocol

from weather_station.shared.measurements import StoredMeasurement


class MeasurementReader(Protocol):
    def registered_sensors(self) -> list[tuple[str, str]]: ...
    def sensor_type(self, sensor_id: str) -> str | None: ...
    def latest(self) -> list[StoredMeasurement]: ...
    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]: ...
    def series(
        self,
        sensor_id: str,
        property_id: str,
        since: datetime,
        limit: int = 3000,
    ) -> list[StoredMeasurement]: ...
