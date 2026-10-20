import logging
import time

from weather_station.sensors.contracts import Sensor
from weather_station.sensors.detection import DetectedSensor
from weather_station.sensors.errors import InvalidCollectorConfiguration, NoSensorsDetected
from weather_station.sensors.recording import RecordingService


class Collector:
    """Polls every detected sensor on one interval. One failing sensor never stops the others."""

    def __init__(
        self,
        detected: list[DetectedSensor],
        recorder: RecordingService,
        interval_seconds: int,
    ) -> None:
        if interval_seconds <= 0:
            raise InvalidCollectorConfiguration("interval_seconds must be positive")
        if not detected:
            raise NoSensorsDetected("No configured sensor answered at startup")
        self._sensors: list[Sensor] = [item.sensor for item in detected]
        self._recorder = recorder
        self._interval_seconds = interval_seconds
        for item in detected:
            recorder.register_sensor(item.sensor.sensor_id, item.sensor.sensor_type)
            recorder.record_many(item.first_reading)

    def collect_once(self) -> int:
        stored = 0
        for sensor in self._sensors:
            try:
                measurements = sensor.read()
                self._recorder.record_many(measurements)
                stored += len(measurements)
            except Exception:
                logging.exception("Reading %s failed", sensor.sensor_id)
        return stored

    def run(self) -> None:
        try:
            while True:
                time.sleep(self._interval_seconds)
                logging.info("Stored %d measurements", self.collect_once())
        finally:
            self.close()

    def close(self) -> None:
        for sensor in self._sensors:
            try:
                sensor.close()
            except Exception:
                logging.exception("Closing %s failed", sensor.sensor_id)
