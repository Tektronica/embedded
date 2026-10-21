import logging

from weather_station.sensors.collector import Collector
from weather_station.sensors.db.sqlite_writer import SQLiteMeasurementWriter
from weather_station.sensors.detection import SensorDetector
from weather_station.sensors.factory import SensorFactory
from weather_station.sensors.recording import RecordingService
from weather_station.sensors.settings import CollectorSettings


def main() -> None:
    settings = CollectorSettings.from_config()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    detected = SensorDetector(SensorFactory()).detect(settings.sensors)
    recorder = RecordingService(SQLiteMeasurementWriter(settings.database_path))
    Collector(detected, recorder, settings.interval_seconds, settings.retention_days).run()


if __name__ == "__main__":
    main()
