import time
from datetime import datetime
from datetime import timezone

from domain.errors import SensorReadError
from domain.measurements import Measurement, make_measurement
from domain.registry import RELATIVE_HUMIDITY, TEMPERATURE


class AM2302Sensor:
    sensor_type = "temperature_humidity"

    def __init__(self, sensor_id: str = "am2302", gpio_pin: int = 4):
        import adafruit_dht
        import board

        self.sensor_id = sensor_id
        self.device = adafruit_dht.DHT22(
            getattr(board, f"D{gpio_pin}"), use_pulseio=False
        )

    def read(self) -> list[Measurement]:
        last_error = None
        for attempt in range(3):
            try:
                timestamp = datetime.now(timezone.utc)
                temperature_c = float(self.device.temperature)
                humidity_percent = float(self.device.humidity)
                return [
                    make_measurement(
                        self.sensor_id,
                        self.sensor_type,
                        TEMPERATURE.property_id,
                        temperature_c + 273.15,
                        TEMPERATURE.si_unit,
                        timestamp,
                    ),
                    make_measurement(
                        self.sensor_id,
                        self.sensor_type,
                        RELATIVE_HUMIDITY.property_id,
                        humidity_percent / 100,
                        RELATIVE_HUMIDITY.si_unit,
                        timestamp,
                    ),
                ]
            except RuntimeError as exc:
                last_error = exc
                if attempt < 2:
                    time.sleep(2)
        raise SensorReadError("AM2302 read failed after 3 attempts") from last_error

    def close(self) -> None:
        self.device.exit()
