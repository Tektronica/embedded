import logging
import time

from weather_station.sensors.contracts import Sensor
from weather_station.sensors.errors import InvalidCollectorConfiguration
from weather_station.sensors.recording import RecordingService
from weather_station.shared.measurements import Measurement


def collect_once(sensor: Sensor, recorder: RecordingService) -> list[Measurement]:
    measurements = sensor.read()
    recorder.record_many(measurements)
    return measurements


def run(sensor: Sensor, recorder: RecordingService, interval_seconds: int) -> None:
    if interval_seconds <= 0:
        sensor.close()
        raise InvalidCollectorConfiguration("interval_seconds must be positive")
    try:
        recorder.register_sensor(sensor.sensor_id, sensor.sensor_type)
        while True:
            try:
                measurements = collect_once(sensor, recorder)
                logging.info("Stored %d measurements for %s", len(measurements), sensor.sensor_id)
            except Exception:
                logging.exception("Collection cycle failed")
            time.sleep(interval_seconds)
    finally:
        sensor.close()
