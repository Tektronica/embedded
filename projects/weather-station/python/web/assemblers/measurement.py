from typing import Any

from pydantic import ValidationError

from domain.errors import RequestValidationError
from domain.measurements import Measurement, StoredMeasurement


class MeasurementAssembler:
    @staticmethod
    def from_request(value: object) -> Measurement:
        try:
            return Measurement.model_validate(value)
        except ValidationError as error:
            raise RequestValidationError(error.errors(include_input=False)) from error

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
