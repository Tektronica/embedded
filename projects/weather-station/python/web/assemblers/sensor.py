from typing import Any

from pydantic import BaseModel, ConfigDict, Field, ValidationError

from domain.errors import RequestValidationError
from domain.registry import SENSOR_DEFINITIONS


class SensorRegistrationRequest(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    sensor_id: str = Field(min_length=1)
    sensor_type: str = Field(min_length=1)


class SensorAssembler:
    @staticmethod
    def registration_from_request(value: object) -> SensorRegistrationRequest:
        try:
            return SensorRegistrationRequest.model_validate(value)
        except ValidationError as error:
            raise RequestValidationError(error.errors(include_input=False)) from error

    @staticmethod
    def registration_to_json(sensor_id: str, sensor_type: str) -> dict[str, str]:
        return {"sensor_id": sensor_id, "sensor_type": sensor_type}

    @staticmethod
    def catalog_to_json() -> list[dict[str, Any]]:
        return [
            {
                "sensor_type": definition.key,
                "label": definition.label,
                "properties": [
                    {
                        "key": prop.key,
                        "property_id": prop.property_id,
                        "label": prop.label,
                        "quantity_kind_uri": prop.quantity_kind_uri,
                        "unit": prop.si_unit,
                        "display_unit": prop.display_unit,
                    }
                    for prop in definition.properties
                ],
            }
            for definition in SENSOR_DEFINITIONS.values()
        ]
