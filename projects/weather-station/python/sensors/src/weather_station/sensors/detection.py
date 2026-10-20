import logging
from dataclasses import dataclass

from weather_station.sensors.config import SensorConfig
from weather_station.sensors.contracts import Sensor
from weather_station.sensors.factory import SensorFactory
from weather_station.sensors.errors import InvalidSensorConfiguration
from weather_station.shared.measurements import Measurement


@dataclass(frozen=True)
class DetectedSensor:
    sensor: Sensor
    first_reading: list[Measurement]


class SensorDetector:
    """Builds each configured sensor and keeps only the ones that answer a first read.

    A sensor that isn't plugged in fails to build or to read, gets logged as not detected, and is
    left out entirely. A configuration mistake, such as an unknown model or option, still raises.
    """

    def __init__(self, factory: SensorFactory) -> None:
        self._factory = factory

    def detect(self, configs: list[SensorConfig]) -> list[DetectedSensor]:
        detected: list[DetectedSensor] = []
        for config in configs:
            sensor: Sensor | None = None
            try:
                sensor = self._factory.build(config)
                detected.append(DetectedSensor(sensor, sensor.read()))
                logging.info("Detected %s (%s)", config.sensor_id, config.model)
            except InvalidSensorConfiguration:
                raise
            except Exception as error:
                logging.warning("Not detected: %s (%s): %s", config.sensor_id, config.model, error)
                if sensor is not None:
                    _close_quietly(sensor)
        return detected


def _close_quietly(sensor: Sensor) -> None:
    try:
        sensor.close()
    except Exception:
        logging.exception("Closing %s failed", sensor.sensor_id)
