from datetime import datetime, timezone
from domain.measurements import Measurement, make_measurement
from domain.registry import LIGHTNING_COUNT, LIGHTNING_DISTANCE


class AS3935Sensor:
    """Basic event counter. Configure an IRQ pin and event handling for lightning details."""

    sensor_type = "lightning"

    def __init__(self, sensor_id: str = "lightning", address: int = 0x03):
        import adafruit_as3935
        import board

        self.sensor_id = sensor_id
        self.device = adafruit_as3935.AS3935(board.I2C(), address=address)
        self.interrupt_count = 0
        self.last_distance_km = None
        self.last_event = None

    def read(self) -> list[Measurement]:
        timestamp = datetime.now(timezone.utc)
        measurements = [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                LIGHTNING_COUNT.property_id,
                float(self.interrupt_count),
                LIGHTNING_COUNT.si_unit,
                timestamp,
            )
        ]
        if self.last_distance_km is not None:
            measurements.append(
                make_measurement(
                    self.sensor_id,
                    self.sensor_type,
                    LIGHTNING_DISTANCE.property_id,
                    float(self.last_distance_km) * 1000,
                    LIGHTNING_DISTANCE.si_unit,
                    timestamp,
                )
            )
        return measurements

    def close(self) -> None:
        pass
