import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from domain.registry import TEMPERATURE
from web.app import create_app


class WebApiTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary_directory = TemporaryDirectory()
        database_path = Path(self.temporary_directory.name) / "weather.sqlite3"
        app = create_app({"TESTING": True, "DATABASE_PATH": database_path})
        self.client = app.test_client()

    def tearDown(self) -> None:
        self.temporary_directory.cleanup()

    def test_ingest_validates_measurement_and_renders_display_units(self) -> None:
        registration = self.client.post(
            "/api/sensors",
            json={"sensor_id": "room-am2302", "sensor_type": "temperature_humidity"},
        )
        self.assertEqual(registration.status_code, 201)

        response = self.client.post(
            "/api/readings",
            json={
                "sensor_id": "room-am2302",
                "observed_at": "2026-10-19T12:00:00+00:00",
                "property_id": TEMPERATURE.property_id,
                "value": 294.65,
                "unit": TEMPERATURE.si_unit,
            },
        )

        self.assertEqual(response.status_code, 201)
        self.assertTrue(response.json["id"])

        series = self.client.get("/api/series/room-am2302?property=air-temperature")
        self.assertEqual(series.status_code, 200)
        self.assertEqual(series.json["measurements"][0]["value"], 294.65)

        dashboard = self.client.get("/")
        self.assertEqual(dashboard.status_code, 200)
        self.assertIn(b"Temperature and humidity", dashboard.data)
        self.assertIn(b"21.5", dashboard.data)

    def test_wrong_unit_and_unknown_sensor_are_rejected_before_persistence(self) -> None:
        self.client.post(
            "/api/sensors",
            json={"sensor_id": "room-am2302", "sensor_type": "temperature_humidity"},
        )
        invalid = self.client.post(
            "/api/readings",
            json={
                "sensor_id": "room-am2302",
                "observed_at": "2026-10-19T12:00:00+00:00",
                "property_id": TEMPERATURE.property_id,
                "value": 21.5,
                "unit": "http://qudt.org/vocab/unit/DEG_C",
            },
        )
        unknown = self.client.post(
            "/api/readings",
            json={
                "sensor_id": "not-registered",
                "observed_at": "2026-10-19T12:00:00+00:00",
                "property_id": TEMPERATURE.property_id,
                "value": 294.65,
                "unit": TEMPERATURE.si_unit,
            },
        )

        self.assertEqual(invalid.status_code, 422)
        self.assertEqual(unknown.status_code, 404)
        self.assertEqual(invalid.json["error"]["code"], "invalid_measurement_unit")
        self.assertEqual(unknown.json["error"]["code"], "sensor_not_registered")
        self.assertEqual(self.client.get("/api/readings").json, [])

    def test_sensor_registration_is_idempotent_and_type_is_immutable(self) -> None:
        registration = {
            "sensor_id": "room-am2302",
            "sensor_type": "temperature_humidity",
        }

        first = self.client.post("/api/sensors", json=registration)
        duplicate = self.client.post("/api/sensors", json=registration)
        conflicting = self.client.post(
            "/api/sensors",
            json={"sensor_id": "room-am2302", "sensor_type": "pressure"},
        )

        self.assertEqual(first.status_code, 201)
        self.assertEqual(duplicate.status_code, 201)
        self.assertEqual(conflicting.status_code, 409)
        self.assertEqual(conflicting.json["error"]["code"], "sensor_registration_conflict")

    def test_unknown_sensor_type_is_rejected(self) -> None:
        response = self.client.post(
            "/api/sensors",
            json={"sensor_id": "sensor-1", "sensor_type": "made-up"},
        )

        self.assertEqual(response.status_code, 400)
        self.assertEqual(response.json["error"]["code"], "unknown_sensor_type")

    def test_invalid_request_uses_structured_validation_error(self) -> None:
        response = self.client.post("/api/readings", json={"unexpected": True})

        self.assertEqual(response.status_code, 422)
        self.assertEqual(response.json["error"]["code"], "request_validation_error")
        self.assertTrue(response.json["error"]["details"])

    def test_empty_dashboard_renders(self) -> None:
        response = self.client.get("/")

        self.assertEqual(response.status_code, 200)
        self.assertIn(b"Waiting for a sensor", response.data)


if __name__ == "__main__":
    unittest.main()