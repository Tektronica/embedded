from datetime import datetime, timezone
import board
import adafruit_veml6075
import adafruit_bh1750
from domain.measurements import Measurement, make_measurement
from domain.registry import ILLUMINANCE, UV_INDEX


class VEML6075Sensor:
    sensor_type = "uv"

    def __init__(self, sensor_id: str = "uv"):
        self.sensor_id = sensor_id
        self.device = adafruit_veml6075.VEML6075(board.I2C())

    def read(self) -> list[Measurement]:
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                UV_INDEX.property_id,
                float(self.device.uv_index),
                UV_INDEX.si_unit,
                datetime.now(timezone.utc),
            )
        ]

    def close(self) -> None:
        pass


class BH1750Sensor:
    sensor_type = "light"

    def __init__(self, sensor_id: str = "ambient-light"):
        self.sensor_id = sensor_id
        self.device = adafruit_bh1750.BH1750(board.I2C())

    def read(self) -> list[Measurement]:
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                ILLUMINANCE.property_id,
                float(self.device.lux),
                ILLUMINANCE.si_unit,
                datetime.now(timezone.utc),
            )
        ]

    def close(self) -> None:
        pass
