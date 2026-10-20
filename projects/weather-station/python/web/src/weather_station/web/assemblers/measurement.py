from typing import Any

from weather_station.shared.measurements import Measurement, StoredMeasurement


class MeasurementAssembler:
    @staticmethod
    def to_json(measurement: Measurement) -> dict[str, Any]:
        result = measurement.model_dump(mode="json")
        result["id"] = str(measurement.id)
        return result

    @classmethod
    def stored_to_json(cls, stored: StoredMeasurement) -> dict[str, Any]:
        return {
            "sensor_type": stored.sensor_type,
            "measurement": cls.to_json(stored.measurement),
        }
