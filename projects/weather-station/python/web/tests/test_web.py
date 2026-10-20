import sqlite3
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.shared.db import SCHEMA_VERSION, schema_sql
from weather_station.shared.registry import TEMPERATURE
from weather_station.web.app import create_app


def create_database(path: Path) -> sqlite3.Connection:
    """Builds the database the way the sensors package would, without importing it."""
    connection = sqlite3.connect(path)
    connection.executescript(schema_sql())
    connection.execute("INSERT INTO schema_version (version) VALUES (?)", (SCHEMA_VERSION,))
    connection.commit()
    return connection


def insert_reading(connection: sqlite3.Connection, value: float) -> None:
    connection.execute(
        "INSERT OR IGNORE INTO sensors (sensor_id, sensor_type) VALUES ('room-am2302', 'temperature_humidity')"
    )
    connection.execute(
        """
        INSERT INTO measurements
            (id, created_at, observed_at, sensor_id, property_id, value, unit, quality, uncertainty)
        VALUES ('8a1f3e2c-0000-4000-8000-000000000001', '2026-10-19T12:00:00+00:00',
                '2026-10-19T12:00:00+00:00', 'room-am2302', ?, ?, ?, 'valid', NULL)
        """,
        (TEMPERATURE.property_id, value, TEMPERATURE.si_unit),
    )
    connection.commit()


class WebApiTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = TemporaryDirectory()
        self.database_path = Path(self.temporary_directory.name) / "weather.sqlite3"
        app = create_app({"TESTING": True, "DATABASE_PATH": self.database_path})
        self.client = app.test_client()

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def test_stored_reading_renders_in_display_units(self) -> None:
        connection = create_database(self.database_path)
        insert_reading(connection, 294.65)
        connection.close()

        readings = self.client.get("/api/readings")
        series = self.client.get("/api/series/room-am2302?property=air-temperature")
        dashboard = self.client.get("/")

        self.assertEqual(readings.status_code, 200)
        self.assertEqual(readings.json[0]["measurement"]["value"], 294.65)
        self.assertEqual(series.json["measurements"][0]["value"], 294.65)
        self.assertIn(b"Temperature and humidity", dashboard.data)
        self.assertIn(b"21.5", dashboard.data)

    def test_empty_dashboard_renders(self) -> None:
        create_database(self.database_path).close()

        response = self.client.get("/")

        self.assertEqual(response.status_code, 200)
        self.assertIn(b"Waiting for a sensor", response.data)

    def test_missing_database_returns_service_unavailable(self) -> None:
        response = self.client.get("/api/readings")

        self.assertEqual(response.status_code, 503)
        self.assertEqual(response.json["error"]["code"], "database_unavailable")

    def test_unsupported_schema_version_returns_service_unavailable(self) -> None:
        connection = create_database(self.database_path)
        connection.execute("UPDATE schema_version SET version = ?", (SCHEMA_VERSION + 1,))
        connection.commit()
        connection.close()

        response = self.client.get("/api/readings")

        self.assertEqual(response.status_code, 503)
        self.assertEqual(response.json["error"]["code"], "schema_version_mismatch")

    def test_web_exposes_no_write_endpoints(self) -> None:
        create_database(self.database_path).close()

        self.assertEqual(self.client.post("/api/readings", json={}).status_code, 405)
        self.assertEqual(self.client.post("/api/sensors", json={}).status_code, 404)

    def test_unknown_sensor_series_returns_not_found(self) -> None:
        create_database(self.database_path).close()

        response = self.client.get("/api/series/not-registered?property=air-temperature")

        self.assertEqual(response.status_code, 404)


if __name__ == "__main__":
    unittest.main()
