import json
import sqlite3
import unittest
from datetime import timedelta
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any

from flask import Flask

from weather_station.shared.db import SCHEMA_VERSION, schema_sql
from weather_station.shared.errors import InvalidStatusConfiguration
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
        config_path = Path(self.temporary_directory.name) / "config.json"
        config_path.write_text(json.dumps({"storage": {"database_path": str(self.database_path)}}))
        app = create_app({"TESTING": True}, config_path)
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
        self.assertIn('aria-label="Temperature in °C'.encode(), dashboard.data)

    def test_sensor_without_recent_readings_is_marked_stale(self) -> None:
        connection = create_database(self.database_path)
        insert_reading(connection, 294.65)
        connection.close()

        dashboard = self.client.get("/")

        self.assertIn(b"Stale", dashboard.data)
        self.assertIn(b"(stale)", dashboard.data)

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


class StatusConfigTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "config.json"

    def tearDown(self) -> None:
        self.directory.cleanup()

    def _create(self, document: Any) -> Flask:
        self.path.write_text(json.dumps(document))
        return create_app({"TESTING": True}, self.path)

    def test_stale_after_defaults_to_five_minutes(self) -> None:
        app = self._create({})

        self.assertEqual(app.config["STALE_AFTER"], timedelta(minutes=5))

    def test_stale_after_reads_the_status_section(self) -> None:
        app = self._create({"status": {"stale_after_minutes": 12}})

        self.assertEqual(app.config["STALE_AFTER"], timedelta(minutes=12))

    def test_database_path_reads_the_storage_section(self) -> None:
        app = self._create({"storage": {"database_path": "data/weather.sqlite3"}})

        self.assertEqual(app.config["DATABASE_PATH"], Path("data/weather.sqlite3"))

    def test_stale_after_rejects_wrong_type_and_bool(self) -> None:
        for value in ("5", 2.5, True):
            with self.subTest(value=value):
                with self.assertRaisesRegex(InvalidStatusConfiguration, "integer"):
                    self._create({"status": {"stale_after_minutes": value}})

    def test_stale_after_must_be_positive(self) -> None:
        with self.assertRaisesRegex(InvalidStatusConfiguration, "positive"):
            self._create({"status": {"stale_after_minutes": 0}})

    def test_unknown_status_key_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidStatusConfiguration, "stale_after_minute"):
            self._create({"status": {"stale_after_minute": 5}})

    def test_status_section_must_be_an_object(self) -> None:
        with self.assertRaisesRegex(InvalidStatusConfiguration, "must be an object"):
            self._create({"status": 5})


if __name__ == "__main__":
    unittest.main()
