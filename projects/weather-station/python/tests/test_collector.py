import unittest
from datetime import datetime, timezone
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from collector.service import collect_once, run
from dal.sqlite_readings import SQLiteMeasurementRepository
from domain.errors import InvalidCollectorConfiguration
from domain.measurements import Measurement
from domain.registry import RELATIVE_HUMIDITY, TEMPERATURE
from services.measurement_service import MeasurementService


class SensorStub:
    sensor_id = "room-am2302"
    sensor_type = "temperature_humidity"

    def __init__(self, measurements: list[Measurement]) -> None:
        self.closed = False
        self.measurements = measurements
        self.read_count = 0

    def read(self) -> list[Measurement]:
        self.read_count += 1
        return self.measurements

    def close(self) -> None:
        self.closed = True


def sample_measurements() -> list[Measurement]:
    timestamp = datetime.now(timezone.utc)
    return [
        Measurement(
            sensor_id="room-am2302",
            observed_at=timestamp,
            property_id=TEMPERATURE.property_id,
            value=294.65,
            unit=TEMPERATURE.si_unit,
        ),
        Measurement(
            sensor_id="room-am2302",
            observed_at=timestamp,
            property_id=RELATIVE_HUMIDITY.property_id,
            value=0.44,
            unit=RELATIVE_HUMIDITY.si_unit,
        ),
    ]


class CollectorTests(unittest.TestCase):
    def test_collect_once_persists_a_measurement_batch(self) -> None:
        sensor = SensorStub(sample_measurements())

        with TemporaryDirectory() as directory:
            repository = SQLiteMeasurementRepository(Path(directory) / "readings.sqlite3")
            service = MeasurementService(repository)
            service.register_sensor(sensor.sensor_id, sensor.sensor_type)
            measurements = collect_once(sensor, service)
            latest = repository.latest()

        self.assertEqual(sensor.read_count, 1)
        self.assertEqual(len(measurements), 2)
        self.assertEqual(
            {item.measurement.property_id for item in latest},
            {TEMPERATURE.property_id, RELATIVE_HUMIDITY.property_id},
        )

    def test_invalid_interval_closes_sensor_without_reading(self) -> None:
        sensor = SensorStub(sample_measurements())
        with TemporaryDirectory() as directory:
            repository = SQLiteMeasurementRepository(Path(directory) / "readings.sqlite3")
            service = MeasurementService(repository)
            with self.assertRaisesRegex(InvalidCollectorConfiguration, "must be positive"):
                run(sensor, service, 0)

        self.assertTrue(sensor.closed)
        self.assertEqual(sensor.read_count, 0)

    def test_run_collects_then_closes_on_interrupt(self) -> None:
        sensor = SensorStub(sample_measurements())
        with TemporaryDirectory() as directory:
            repository = SQLiteMeasurementRepository(Path(directory) / "readings.sqlite3")
            service = MeasurementService(repository)
            with patch("collector.service.time.sleep", side_effect=KeyboardInterrupt):
                with self.assertRaises(KeyboardInterrupt):
                    run(sensor, service, 1)
            registered_type = repository.sensor_type(sensor.sensor_id)

        self.assertTrue(sensor.closed)
        self.assertEqual(sensor.read_count, 1)
        self.assertEqual(registered_type, sensor.sensor_type)


if __name__ == "__main__":
    unittest.main()