import logging

from weather_station.sensors.adapters.am2302 import AM2302Sensor
from weather_station.sensors.collector import run
from weather_station.sensors.db.sqlite_writer import SQLiteMeasurementWriter
from weather_station.sensors.recording import RecordingService
from weather_station.sensors.settings import CollectorSettings


def main() -> None:
    settings = CollectorSettings.from_env()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    sensor = AM2302Sensor(sensor_id=settings.sensor_id, gpio_pin=settings.gpio_pin)
    recorder = RecordingService(SQLiteMeasurementWriter(settings.database_path))
    run(sensor, recorder, interval_seconds=settings.interval_seconds)


if __name__ == "__main__":
    main()
