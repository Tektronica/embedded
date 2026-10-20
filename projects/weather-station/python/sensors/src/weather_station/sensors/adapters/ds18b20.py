from datetime import datetime, timezone
from w1thermsensor import W1ThermSensor
from weather_station.sensors.errors import SensorUnavailable
from weather_station.shared.measurements import Measurement, make_measurement
from weather_station.shared.registry import TEMPERATURE


class DS18B20Sensor:
    sensor_type = "probe_temperature"

    def __init__(self, sensor_id: str = "probe-temperature", sensor_index: int = 0):
        self.sensor_id = sensor_id
        sensors = W1ThermSensor.get_available_sensors()
        if not sensors or sensor_index >= len(sensors):
            raise SensorUnavailable("No DS18B20 detected; enable 1-Wire and check wiring")
        self.device = sensors[sensor_index]

    def read(self) -> list[Measurement]:
        return [
            make_measurement(
                self.sensor_id,
                self.sensor_type,
                TEMPERATURE.property_id,
                float(self.device.get_temperature()) + 273.15,
                TEMPERATURE.si_unit,
                datetime.now(timezone.utc),
            )
        ]

    def close(self) -> None:
        pass
