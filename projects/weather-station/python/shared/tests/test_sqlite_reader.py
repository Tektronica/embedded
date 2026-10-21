import sqlite3
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory
from uuid import uuid4

from weather_station.shared.db import SCHEMA_VERSION, schema_sql
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.errors import (
    DatabaseUnavailable,
    MeasurementPersistenceError,
    SchemaVersionMismatch,
)
from weather_station.shared.registry import TEMPERATURE


class SQLiteMeasurementReaderTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "weather.sqlite3"

    def tearDown(self) -> None:
        self.directory.cleanup()

    def _create(self, version: int = SCHEMA_VERSION) -> None:
        with sqlite3.connect(self.path) as connection:
            connection.executescript(schema_sql())
            connection.execute("INSERT INTO schema_version (version) VALUES (?)", (version,))

    def test_missing_database_is_reported_and_not_created(self) -> None:
        with self.assertRaises(DatabaseUnavailable):
            SQLiteMeasurementReader(self.path).latest()
        self.assertFalse(self.path.exists())

    def test_reads_registered_sensors(self) -> None:
        self._create()
        with sqlite3.connect(self.path) as connection:
            connection.execute("INSERT INTO sensors VALUES ('room-am2302', 'temperature_humidity')")

        sensors = SQLiteMeasurementReader(self.path).registered_sensors()

        self.assertEqual(sensors, [("room-am2302", "temperature_humidity")])

    def test_rejects_an_unsupported_schema_version(self) -> None:
        self._create(version=SCHEMA_VERSION + 1)

        with self.assertRaises(SchemaVersionMismatch):
            SQLiteMeasurementReader(self.path).latest()

    def test_series_keeps_the_newest_rows_in_ascending_order(self) -> None:
        self._create()
        start = datetime(2026, 10, 1, tzinfo=timezone.utc)
        with sqlite3.connect(self.path) as connection:
            connection.execute("INSERT INTO sensors VALUES ('room-am2302', 'temperature_humidity')")
            for minute in range(5):
                observed_at = (start + timedelta(minutes=minute)).isoformat()
                connection.execute(
                    "INSERT INTO measurements VALUES (?, ?, ?, 'room-am2302', ?, ?, ?, 'valid', NULL)",
                    (str(uuid4()), observed_at, observed_at, TEMPERATURE.property_id, 290.0 + minute, TEMPERATURE.si_unit),
                )

        history = SQLiteMeasurementReader(self.path).series(
            "room-am2302", TEMPERATURE.property_id, start, limit=3
        )

        self.assertEqual([item.measurement.value for item in history], [292.0, 293.0, 294.0])

    def test_connection_is_read_only(self) -> None:
        self._create()
        reader = SQLiteMeasurementReader(self.path)

        with self.assertRaises(MeasurementPersistenceError):
            with reader._connection() as connection:
                connection.execute("INSERT INTO sensors VALUES ('x', 'uv')")


if __name__ == "__main__":
    unittest.main()
