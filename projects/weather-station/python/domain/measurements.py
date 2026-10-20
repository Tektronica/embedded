from dataclasses import dataclass
from datetime import datetime, timezone
from enum import StrEnum
from uuid import UUID, uuid4

from pydantic import BaseModel, ConfigDict, Field, field_validator
from pydantic_core import PydanticCustomError


class MeasurementQuality(StrEnum):
    VALID = "valid"
    SUSPECT = "suspect"
    INVALID = "invalid"


class Measurement(BaseModel):
    model_config = ConfigDict(extra="forbid", frozen=True)

    id: UUID = Field(default_factory=uuid4)
    created_at: datetime = Field(default_factory=lambda: datetime.now(timezone.utc))
    observed_at: datetime
    sensor_id: str = Field(min_length=1)
    property_id: str = Field(min_length=1)
    value: float = Field(allow_inf_nan=False)
    unit: str = Field(min_length=1)
    quality: MeasurementQuality = MeasurementQuality.VALID
    uncertainty: float | None = Field(default=None, ge=0, allow_inf_nan=False)

    @field_validator("created_at", "observed_at")
    @classmethod
    def require_timezone(cls, value: datetime) -> datetime:
        if value.tzinfo is None or value.utcoffset() is None:
            raise PydanticCustomError(
                "timezone_required",
                "measurement timestamps must include a timezone",
            )
        return value


@dataclass(frozen=True)
class StoredMeasurement:
    sensor_type: str
    measurement: Measurement


def make_measurement(
    sensor_id: str,
    sensor_type: str,
    property_id: str,
    value: float,
    unit: str,
    observed_at: datetime | None = None,
    uncertainty: float | None = None,
    quality: MeasurementQuality = MeasurementQuality.VALID,
) -> Measurement:
    from domain.registry import validate_measurement

    measurement = Measurement(
        sensor_id=sensor_id,
        observed_at=observed_at or datetime.now(timezone.utc),
        property_id=property_id,
        value=value,
        unit=unit,
        uncertainty=uncertainty,
        quality=quality,
    )
    return validate_measurement(sensor_type, measurement)
