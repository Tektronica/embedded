import logging

from weather_station.sensors.collector import Collector
from weather_station.sensors.config import load_sensor_configs
from weather_station.sensors.db.sqlite_writer import SQLiteMeasurementWriter
from weather_station.sensors.detection import SensorDetector
from weather_station.sensors.factory import SensorFactory
from weather_station.sensors.recording import RecordingService
from weather_station.sensors.settings import CollectorSettings


def main() -> None:
    settings = CollectorSettings.from_env()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    detected = SensorDetector(SensorFactory()).detect(load_sensor_configs(settings.sensors_config_path))
    recorder = RecordingService(SQLiteMeasurementWriter(settings.database_path))
    Collector(detected, recorder, settings.interval_seconds).run()


if __name__ == "__main__":
    main()
