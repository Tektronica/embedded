import sqlite3
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.sensors.db.sqlite_writer import SQLiteMeasurementWriter
from weather_station.shared.measurements import Measurement
from weather_station.shared.registry import TEMPERATURE


def measurement(observed_at: datetime) -> Measurement:
    return Measurement(
        sensor_id="sensor-am2302",
        observed_at=observed_at,
        property_id=TEMPERATURE.property_id,
        value=294.65,
        unit=TEMPERATURE.si_unit,
    )


class DeleteObservedBeforeTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "readings.sqlite3"
        self.writer = SQLiteMeasurementWriter(self.path)
        self.writer.register_sensor("sensor-am2302", "temperature_humidity")

    def tearDown(self) -> None:
        self.directory.cleanup()

    def stored_observed_at(self) -> list[str]:
        with sqlite3.connect(self.path) as connection:
            rows = connection.execute("SELECT observed_at FROM measurements ORDER BY observed_at")
            return [row[0] for row in rows]

    def test_deletes_only_rows_older_than_the_cutoff_and_returns_the_count(self) -> None:
        cutoff = datetime(2026, 1, 10, 12, 0, 0, tzinfo=timezone.utc)
        self.writer.save_many(
            [
                measurement(cutoff - timedelta(days=5)),
                measurement(cutoff - timedelta(microseconds=1)),
                measurement(cutoff),
                measurement(cutoff + timedelta(microseconds=1)),
            ]
        )

        deleted = self.writer.delete_observed_before(cutoff)

        self.assertEqual(deleted, 2)
        self.assertEqual(
            self.stored_observed_at(),
            [cutoff.isoformat(), (cutoff + timedelta(microseconds=1)).isoformat()],
        )

    def test_compares_in_utc_when_the_cutoff_has_another_offset(self) -> None:
        cutoff = datetime(2026, 1, 10, 12, 0, tzinfo=timezone.utc)
        local = timezone(timedelta(hours=-5))
        self.writer.save_many([measurement(cutoff - timedelta(hours=1)), measurement(cutoff + timedelta(hours=1))])

        deleted = self.writer.delete_observed_before(cutoff.astimezone(local))

        self.assertEqual(deleted, 1)


if __name__ == "__main__":
    unittest.main()
