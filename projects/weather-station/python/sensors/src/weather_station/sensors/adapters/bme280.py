from datetime import datetime, timezone
import board
import adafruit_bme280.basic as adafruit_bme280
from weather_station.shared.measurements import Measurement, make_measurement
from weather_station.shared.registry import PRESSURE, RELATIVE_HUMIDITY, TEMPERATURE


class BME280Sensor:
    sensor_type = "pressure"

    def __init__(self, sensor_id: str = "bme280", address: int = 0x76):
        self.sensor_id = sensor_id
        self.i2c = board.I2C()
        self.device = adafruit_bme280.Adafruit_BME280_I2C(self.i2c, address=address)

    def read(self) -> list[Measurement]:
        timestamp = datetime.now(timezone.utc)
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                TEMPERATURE.property_id,
                float(self.device.temperature) + 273.15,
                TEMPERATURE.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                RELATIVE_HUMIDITY.property_id,
                float(self.device.relative_humidity) / 100,
                RELATIVE_HUMIDITY.si_unit,
                timestamp,
            ),
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                PRESSURE.property_id,
                float(self.device.pressure) * 100,
                PRESSURE.si_unit,
                timestamp,
            ),
        ]

    def close(self) -> None:
        pass
