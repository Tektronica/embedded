import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from weather_station.sensors.collector import Collector
from weather_station.sensors.db.sqlite_writer import SQLiteMeasurementWriter
from weather_station.sensors.detection import DetectedSensor
from weather_station.sensors.errors import InvalidCollectorConfiguration, NoSensorsDetected
from weather_station.sensors.recording import RecordingService
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.measurements import Measurement
from weather_station.shared.registry import RELATIVE_HUMIDITY, TEMPERATURE


def sample_measurements(sensor_id: str) -> list[Measurement]:
    timestamp = datetime.now(timezone.utc)
    return [
        Measurement(sensor_id=sensor_id, observed_at=timestamp, property_id=TEMPERATURE.property_id,
                    value=294.65, unit=TEMPERATURE.si_unit),
        Measurement(sensor_id=sensor_id, observed_at=timestamp, property_id=RELATIVE_HUMIDITY.property_id,
                    value=0.44, unit=RELATIVE_HUMIDITY.si_unit),
    ]


class SensorStub:
    sensor_type = "temperature_humidity"

    def __init__(self, sensor_id: str, fails: bool = False) -> None:
        self.sensor_id = sensor_id
        self.fails = fails
        self.closed = False
        self.read_count = 0

    def read(self) -> list[Measurement]:
        self.read_count += 1
        if self.fails:
            raise RuntimeError("sensor unplugged")
        return sample_measurements(self.sensor_id)

    def close(self) -> None:
        self.closed = True


class FakeClock:
    def __init__(self) -> None:
        self.current = datetime(2026, 1, 1, tzinfo=timezone.utc)

    def __call__(self) -> datetime:
        return self.current

    def advance(self, delta: timedelta) -> None:
        self.current += delta


class RecorderSpy:
    def __init__(self, fails: bool = False) -> None:
        self.cutoffs: list[datetime] = []
        self.fails = fails

    def register_sensor(self, sensor_id: str, sensor_type: str) -> None:
        pass

    def record_many(self, measurements: list[Measurement]) -> None:
        pass

    def prune_observed_before(self, cutoff: datetime) -> int:
        self.cutoffs.append(cutoff)
        if self.fails:
            raise RuntimeError("database locked")
        return 3


class CollectorTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "readings.sqlite3"
        self.recorder = RecordingService(SQLiteMeasurementWriter(self.path))

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_registers_detected_sensors_and_stores_their_first_reading(self) -> None:
        sensor = SensorStub("room")
        Collector([DetectedSensor(sensor, sensor.read())], self.recorder, 60, 30)

        latest = SQLiteMeasurementReader(self.path).latest()

        self.assertEqual({item.measurement.sensor_id for item in latest}, {"room"})
        self.assertEqual(len(latest), 2)

    def test_one_failing_sensor_does_not_stop_the_others(self) -> None:
        good, flaky = SensorStub("good"), SensorStub("flaky")
        collector = Collector(
            [DetectedSensor(good, good.read()), DetectedSensor(flaky, flaky.read())],
            self.recorder,
            60,
            30,
        )
        flaky.fails = True

        with self.assertLogs(level="ERROR"):
            stored = collector.collect_once()

        self.assertEqual(stored, 2)

    def test_run_closes_every_sensor_on_interrupt(self) -> None:
        sensors = [SensorStub("a"), SensorStub("b")]
        collector = Collector([DetectedSensor(s, s.read()) for s in sensors], self.recorder, 1, 30)

        with patch("weather_station.sensors.collector.time.sleep", side_effect=KeyboardInterrupt):
            with self.assertRaises(KeyboardInterrupt):
                collector.run()

        self.assertTrue(all(sensor.closed for sensor in sensors))

    def test_rejects_a_non_positive_interval(self) -> None:
        sensor = SensorStub("room")
        with self.assertRaisesRegex(InvalidCollectorConfiguration, "must be positive"):
            Collector([DetectedSensor(sensor, [])], self.recorder, 0, 30)

    def test_rejects_a_non_positive_retention(self) -> None:
        sensor = SensorStub("room")
        with self.assertRaisesRegex(InvalidCollectorConfiguration, "retention_days must be positive"):
            Collector([DetectedSensor(sensor, [])], self.recorder, 60, 0)

    def run_with_sleeps(self, collector: Collector, clock: FakeClock, sleeps: list[timedelta]) -> None:
        remaining = iter(sleeps)

        def fake_sleep(_: float) -> None:
            try:
                clock.advance(next(remaining))
            except StopIteration:
                raise KeyboardInterrupt

        with patch("weather_station.sensors.collector.time.sleep", side_effect=fake_sleep):
            with self.assertRaises(KeyboardInterrupt):
                collector.run()

    def test_prunes_at_startup_then_at_most_once_per_hour(self) -> None:
        sensor = SensorStub("room")
        clock, spy = FakeClock(), RecorderSpy()
        collector = Collector([DetectedSensor(sensor, [])], spy, 60, 7, clock)  # type: ignore[arg-type]
        start = clock.current
        retention = timedelta(days=7)

        with self.assertLogs(level="INFO") as logs:
            self.run_with_sleeps(
                collector, clock, [timedelta(minutes=30), timedelta(minutes=29), timedelta(minutes=2)]
            )

        self.assertEqual(spy.cutoffs, [start - retention, start + timedelta(minutes=61) - retention])
        self.assertIn("Pruned 3 measurements older than", logs.output[0])

    def test_pruning_failure_is_logged_and_collection_continues(self) -> None:
        sensor = SensorStub("room")
        clock, spy = FakeClock(), RecorderSpy(fails=True)
        collector = Collector([DetectedSensor(sensor, [])], spy, 60, 7, clock)  # type: ignore[arg-type]

        with self.assertLogs(level="ERROR"):
            self.run_with_sleeps(collector, clock, [timedelta(hours=2)])

        self.assertEqual(sensor.read_count, 1)
        self.assertEqual(len(spy.cutoffs), 2)

    def test_refuses_to_start_with_no_detected_sensors(self) -> None:
        with self.assertRaises(NoSensorsDetected):
            Collector([], self.recorder, 60, 30)


if __name__ == "__main__":
    unittest.main()
