import time
from datetime import datetime, timezone
import board
import adafruit_sgp30
from weather_station.shared.measurements import Measurement, make_measurement
from weather_station.shared.registry import EQUIVALENT_CO2, TVOC


class SGP30Sensor:
    sensor_type = "air_quality"

    def __init__(self, sensor_id: str = "sgp30"):
        self.sensor_id = sensor_id
        self.device = adafruit_sgp30.Adafruit_SGP30(board.I2C())
        self.device.iaq_init()
        self.device.set_iaq_baseline(0x8973, 0x8AAE) if False else None
        self.started = time.monotonic()

    def read(self) -> list[Measurement]:
        # SGP30 requires periodic measurements; allow its baseline algorithm to warm up.
        tvoc, eco2 = self.device.iaq_measure()
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                TVOC.property_id,
                int(tvoc) / 1_000_000_000,
                TVOC.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                EQUIVALENT_CO2.property_id,
                int(eco2) / 1_000_000,
                EQUIVALENT_CO2.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        pass
