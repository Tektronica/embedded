import json
import re
import sqlite3
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tempfile import TemporaryDirectory
from uuid import uuid4

from flask.testing import FlaskClient

from test_web import create_database
from weather_station.shared.registry import PRESSURE, PropertyDefinition, RELATIVE_HUMIDITY, TEMPERATURE
from weather_station.shared.units import CELSIUS, FAHRENHEIT, UnitSystem
from weather_station.web.app import create_app
from weather_station.web.assemblers.dashboard import RETIRED_AFTER, DashboardView

ROOM = "room-am2302"
KELVIN_AT_20_C = 293.15


def add_reading(
    connection: sqlite3.Connection,
    sensor_id: str,
    sensor_type: str,
    prop: PropertyDefinition,
    value: float,
    age: timedelta,
) -> None:
    observed_at = (datetime.now(timezone.utc) - age).isoformat()
    connection.execute("INSERT OR IGNORE INTO sensors (sensor_id, sensor_type) VALUES (?, ?)", (sensor_id, sensor_type))
    connection.execute(
        """
        INSERT INTO measurements
            (id, created_at, observed_at, sensor_id, property_id, value, unit, quality, uncertainty)
        VALUES (?, ?, ?, ?, ?, ?, ?, 'valid', NULL)
        """,
        (str(uuid4()), observed_at, observed_at, sensor_id, prop.property_id, value, prop.si_unit),
    )
    connection.commit()


def register_empty(connection: sqlite3.Connection, sensor_id: str, sensor_type: str) -> None:
    connection.execute("INSERT INTO sensors (sensor_id, sensor_type) VALUES (?, ?)", (sensor_id, sensor_type))
    connection.commit()


class DashboardTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.database_path = Path(self.directory.name) / "weather.sqlite3"
        self.connection = create_database(self.database_path)

    def tearDown(self) -> None:
        self.connection.close()
        self.directory.cleanup()

    def client(self, display: dict[str, str] | None = None) -> FlaskClient:
        config = {"storage": {"database_path": str(self.database_path)}}
        if display is not None:
            config["display"] = display
        path = Path(self.directory.name) / "config.json"
        path.write_text(json.dumps(config))
        self.app = create_app({"TESTING": True}, path)
        return self.app.test_client()

    def assemble(self, sensor_id: str | None = None, property_key: str | None = None) -> DashboardView:
        self.client()
        return self.app.extensions["dashboard_assembler"].assemble(sensor_id, property_key, CELSIUS)

    def add_room(self, kelvin: float = KELVIN_AT_20_C, age: timedelta = timedelta(minutes=1)) -> None:
        add_reading(self.connection, ROOM, "temperature_humidity", TEMPERATURE, kelvin, age)


class UnitPrecedenceTests(DashboardTestCase):
    def setUp(self) -> None:
        super().setUp()
        self.add_room()

    def test_config_default_applies_without_query_or_cookie(self) -> None:
        response = self.client({"temperature_unit": "fahrenheit"}).get("/")

        self.assertIn(b"68.0", response.data)
        self.assertIn("Temperature in °F".encode(), response.data)

    def test_default_is_celsius(self) -> None:
        response = self.client().get("/")

        self.assertIn(b"20.0", response.data)
        self.assertIn("Temperature in °C".encode(), response.data)

    def test_cookie_beats_config(self) -> None:
        client = self.client({"temperature_unit": "celsius"})
        client.set_cookie("unit", "f")

        self.assertIn("Temperature in °F".encode(), client.get("/").data)

    def test_query_beats_cookie(self) -> None:
        client = self.client({"temperature_unit": "fahrenheit"})
        client.set_cookie("unit", "f")

        self.assertIn("Temperature in °C".encode(), client.get("/?unit=c").data)

    def test_invalid_query_falls_back_to_cookie_without_setting_one(self) -> None:
        client = self.client()
        client.set_cookie("unit", "f")

        response = client.get("/?unit=kelvin")

        self.assertEqual(response.status_code, 200)
        self.assertIn("Temperature in °F".encode(), response.data)
        self.assertNotIn("Set-Cookie", response.headers)

    def test_invalid_query_falls_back_to_config_default(self) -> None:
        response = self.client({"temperature_unit": "fahrenheit"}).get("/?unit=")

        self.assertEqual(response.status_code, 200)
        self.assertIn("Temperature in °F".encode(), response.data)

    def test_valid_query_sets_a_year_long_lax_cookie_and_varies_on_cookie(self) -> None:
        response = self.client().get("/?unit=f")

        cookie = response.headers["Set-Cookie"]
        self.assertIn("unit=f", cookie)
        self.assertIn("SameSite=Lax", cookie)
        self.assertIn(f"Max-Age={365 * 24 * 3600}", cookie)
        self.assertIn("Cookie", response.headers["Vary"])

    def test_no_query_sets_no_cookie(self) -> None:
        self.assertNotIn("Set-Cookie", self.client().get("/").headers)

    def test_api_series_stays_si_under_fahrenheit(self) -> None:
        client = self.client({"temperature_unit": "fahrenheit"})
        client.set_cookie("unit", "f")

        response = client.get(f"/api/series/{ROOM}?property=air-temperature")

        self.assertEqual(response.json["unit"], TEMPERATURE.si_unit)
        self.assertAlmostEqual(response.json["measurements"][0]["value"], KELVIN_AT_20_C)

    def test_api_readings_stay_si_under_fahrenheit(self) -> None:
        client = self.client({"temperature_unit": "fahrenheit"})

        response = client.get("/api/readings")

        self.assertAlmostEqual(response.json[0]["measurement"]["value"], KELVIN_AT_20_C)


class ConvertedPresentationTests(DashboardTestCase):
    def test_tiles_chart_ticks_and_entries_convert_in_fahrenheit(self) -> None:
        self.add_room(293.15, timedelta(hours=2))
        self.add_room(295.15, timedelta(minutes=1))
        add_reading(self.connection, "bme280", "pressure", PRESSURE, 101300.0, timedelta(minutes=1))
        add_reading(self.connection, "bme280", "pressure", TEMPERATURE, 273.15, timedelta(minutes=1))

        view = self.app_view(FAHRENHEIT, ROOM)

        self.assertAlmostEqual(view.current_value or 0.0, 71.6)
        self.assertAlmostEqual(view.chart.minimum or 0.0, 68.0)
        self.assertAlmostEqual(view.chart.maximum or 0.0, 71.6)
        self.assertEqual(view.chart.unit, "°F")
        self.assertEqual(view.display_unit, "°F")
        self.assertGreaterEqual(float(view.chart.y_ticks[0].label), 67.0)
        entries = {entry.sensor_id: entry for entry in view.entries}
        self.assertAlmostEqual(entries[ROOM].value or 0.0, 71.6)
        self.assertEqual(entries[ROOM].unit, "°F")

    def test_non_temperature_properties_keep_their_display_unit(self) -> None:
        add_reading(self.connection, ROOM, "temperature_humidity", RELATIVE_HUMIDITY, 0.4, timedelta(minutes=1))
        self.add_room()

        view = self.app_view(FAHRENHEIT, ROOM, "relative-humidity")

        self.assertAlmostEqual(view.current_value or 0.0, 40.0)
        self.assertEqual(view.display_unit, "%")

    def app_view(self, units: UnitSystem, sensor_id: str, property_key: str | None = None) -> DashboardView:
        self.client()
        view = self.app.extensions["dashboard_assembler"].assemble(sensor_id, property_key, units)
        assert isinstance(view, DashboardView)
        return view


class StatusBadgeTests(DashboardTestCase):
    def test_healthy_shows_no_badge_and_keeps_updated_text(self) -> None:
        self.add_room()

        view = self.assemble()
        page = self.client().get("/").data

        self.assertIsNone(view.status_badge)
        self.assertIn(b"Updated", page)
        self.assertNotIn(b"Live", page)
        self.assertNotIn(b"stale</span>", page)

    def test_stale_badge_counts_stale_sensors_among_reporting_ones(self) -> None:
        self.add_room()
        add_reading(self.connection, "bme280", "pressure", PRESSURE, 101300.0, timedelta(hours=3))
        add_reading(self.connection, "probe", "probe_temperature", TEMPERATURE, 290.0, timedelta(hours=4))

        badge = self.assemble().status_badge

        assert badge is not None
        self.assertEqual((badge.tone, badge.label), ("stale", "2 of 3 stale"))
        self.assertIn(b"2 of 3 stale", self.client().get("/").data)

    def test_retired_sensor_stays_stale_in_the_list_but_not_in_the_badge(self) -> None:
        self.add_room()
        add_reading(
            self.connection, "old", "probe_temperature", TEMPERATURE, 290.0, RETIRED_AFTER + timedelta(hours=1)
        )

        view = self.assemble()

        self.assertIsNone(view.status_badge)
        retired = next(item for item in view.sensors if item.sensor_id == "old")
        self.assertTrue(retired.is_stale)
        self.assertTrue(next(entry for entry in view.entries if entry.sensor_id == "old").is_stale)

    def test_no_data_when_no_sensor_has_a_reading(self) -> None:
        register_empty(self.connection, ROOM, "temperature_humidity")

        badge = self.assemble().status_badge

        assert badge is not None
        self.assertEqual((badge.tone, badge.label), ("no-data", "No data"))

    def test_no_data_with_no_sensors(self) -> None:
        badge = self.assemble().status_badge

        assert badge is not None
        self.assertEqual(badge.tone, "no-data")


class SensorLayoutTests(DashboardTestCase):
    def test_sensors_sort_by_label_then_id(self) -> None:
        add_reading(self.connection, "b-probe", "probe_temperature", TEMPERATURE, 290.0, timedelta(minutes=1))
        add_reading(self.connection, "a-probe", "probe_temperature", TEMPERATURE, 290.0, timedelta(minutes=1))
        add_reading(self.connection, "bme280", "pressure", PRESSURE, 101300.0, timedelta(minutes=1))
        self.add_room()

        view = self.assemble()

        self.assertEqual([item.sensor_id for item in view.sensors], ["bme280", "a-probe", "b-probe", ROOM])
        self.assertEqual([entry.sensor_id for entry in view.entries], ["bme280", "a-probe", "b-probe", ROOM])

    def test_entries_show_the_declared_primary_property(self) -> None:
        add_reading(self.connection, "bme280", "pressure", TEMPERATURE, 293.15, timedelta(minutes=1))
        add_reading(self.connection, "bme280", "pressure", PRESSURE, 101300.0, timedelta(minutes=1))

        entry = self.assemble().entries[0]

        self.assertEqual((entry.property_label, entry.unit), ("Pressure", "hPa"))
        self.assertAlmostEqual(entry.value or 0.0, 1013.0)

    def test_one_sensor_hides_navigation(self) -> None:
        self.add_room()

        page = self.client().get("/").data

        self.assertNotIn(b'aria-label="Sensors"', page)
        self.assertNotIn(b"sensor-select", page)

    def test_many_sensors_render_sidebar_and_a_select(self) -> None:
        self.add_room()
        add_reading(self.connection, "bme280", "pressure", PRESSURE, 101300.0, timedelta(minutes=1))

        page = self.client().get("/").data.decode()

        self.assertIn('aria-label="Sensors"', page)
        self.assertIn('id="sensor-select"', page)
        self.assertEqual(page.count('aria-current="page"'), 2)
        self.assertEqual(page.count('href="/?sensor_id=bme280"'), 1)

    def test_empty_sensor_is_marked_no_data(self) -> None:
        self.add_room()
        register_empty(self.connection, "empty-uv", "uv")

        view = self.assemble()
        page = self.client().get("/").data.decode()

        entry = next(item for item in view.entries if item.sensor_id == "empty-uv")
        self.assertIsNone(entry.value)
        self.assertIn("No data", page)
        self.assertIn("(empty-uv) - no data", page)

    def test_property_tabs_mark_the_selected_one(self) -> None:
        self.add_room()

        page = self.client().get(f"/?sensor_id={ROOM}&property=relative-humidity").data.decode()

        tabs = re.search(r'<nav[^>]*aria-label="Measurements".*?</nav>', page, re.S)
        assert tabs is not None
        current = re.findall(r'aria-current="page">([^<]+)</a>', tabs.group(0))
        self.assertEqual(current, ["Relative humidity"])
        self.assertNotIn("flex-wrap", tabs.group(0))


class UnitToggleTests(DashboardTestCase):
    def test_toggle_marks_the_active_unit_and_preserves_the_selection(self) -> None:
        self.add_room()

        page = self.client().get(f"/?sensor_id={ROOM}&property=relative-humidity&unit=f").data.decode()

        form = re.search(r'<form[^>]*aria-label="Temperature unit".*?</form>', page, re.S)
        assert form is not None
        html = form.group(0)
        self.assertIn('role="group"', html)
        self.assertIn(f'name="sensor_id" value="{ROOM}"', html)
        self.assertIn('name="property" value="relative-humidity"', html)
        self.assertRegex(html, r'value="c" aria-pressed="false"')
        self.assertRegex(html, r'value="f" aria-pressed="true"')


if __name__ == "__main__":
    unittest.main()
