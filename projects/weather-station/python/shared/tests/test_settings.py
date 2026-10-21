import json
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.shared.errors import (
    ConfigFileError,
    InvalidDisplayConfiguration,
    InvalidStatusConfiguration,
    InvalidStorageConfiguration,
)
from weather_station.shared.settings import database_path, load_config, read_stale_after_minutes, read_unit_system
from weather_station.shared.units import CELSIUS, FAHRENHEIT


class ConfigTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "config.json"

    def tearDown(self) -> None:
        self.directory.cleanup()


class LoadConfigTests(ConfigTestCase):
    def test_returns_the_parsed_document(self) -> None:
        self.path.write_text(json.dumps({"status": {"stale_after_minutes": 9}}))

        self.assertEqual(load_config(self.path), {"status": {"stale_after_minutes": 9}})

    def test_missing_file_names_the_file_and_the_example(self) -> None:
        with self.assertRaises(ConfigFileError) as raised:
            load_config(self.path)

        self.assertIn(str(self.path), str(raised.exception))
        self.assertIn("config.example.json", str(raised.exception))

    def test_invalid_json_names_the_file(self) -> None:
        self.path.write_text("{ not json")

        with self.assertRaisesRegex(ConfigFileError, f"Invalid JSON in {self.path}"):
            load_config(self.path)

    def test_top_level_must_be_an_object(self) -> None:
        self.path.write_text("[]")

        with self.assertRaisesRegex(ConfigFileError, "JSON object"):
            load_config(self.path)

    def test_unknown_top_level_section_is_rejected(self) -> None:
        self.path.write_text(json.dumps({"sensor": {}}))

        with self.assertRaisesRegex(ConfigFileError, "unknown sections: sensor"):
            load_config(self.path)


class DatabasePathTests(ConfigTestCase):
    def test_reads_the_storage_section(self) -> None:
        self.path.write_text(json.dumps({"storage": {"database_path": "data/w.sqlite3"}}))

        self.assertEqual(database_path(self.path), Path("data/w.sqlite3"))

    def test_defaults_when_the_key_is_absent(self) -> None:
        self.path.write_text("{}")

        self.assertEqual(database_path(self.path), Path("instance/weather.sqlite3"))

    def test_rejects_a_non_string_path(self) -> None:
        self.path.write_text(json.dumps({"storage": {"database_path": 5}}))

        with self.assertRaisesRegex(InvalidStorageConfiguration, "non-empty string"):
            database_path(self.path)

    def test_rejects_an_unknown_storage_key(self) -> None:
        self.path.write_text(json.dumps({"storage": {"path": "x"}}))

        with self.assertRaisesRegex(InvalidStorageConfiguration, "unknown keys: path"):
            database_path(self.path)

    def test_storage_section_must_be_an_object(self) -> None:
        self.path.write_text(json.dumps({"storage": "x"}))

        with self.assertRaisesRegex(InvalidStorageConfiguration, "must be an object"):
            database_path(self.path)


    def test_alarms_is_an_allowed_top_level_section(self) -> None:
        self.path.write_text(json.dumps({"alarms": []}))

        self.assertEqual(load_config(self.path), {"alarms": []})


class StaleAfterMinutesTests(unittest.TestCase):
    def test_defaults_to_five(self) -> None:
        self.assertEqual(read_stale_after_minutes({}), 5)
        self.assertEqual(read_stale_after_minutes({"status": {}}), 5)

    def test_reads_the_status_section(self) -> None:
        self.assertEqual(read_stale_after_minutes({"status": {"stale_after_minutes": 12}}), 12)

    def test_rejects_wrong_type_and_bool(self) -> None:
        for value in ("5", 2.5, True):
            with self.subTest(value=value):
                with self.assertRaisesRegex(InvalidStatusConfiguration, "integer"):
                    read_stale_after_minutes({"status": {"stale_after_minutes": value}})

    def test_must_be_positive(self) -> None:
        with self.assertRaisesRegex(InvalidStatusConfiguration, "positive"):
            read_stale_after_minutes({"status": {"stale_after_minutes": 0}})

    def test_rejects_unknown_keys_and_non_objects(self) -> None:
        with self.assertRaisesRegex(InvalidStatusConfiguration, "unknown keys"):
            read_stale_after_minutes({"status": {"stale_after_minute": 5}})
        with self.assertRaisesRegex(InvalidStatusConfiguration, "must be an object"):
            read_stale_after_minutes({"status": 5})


class ReadUnitSystemTests(unittest.TestCase):
    def test_defaults_to_celsius(self) -> None:
        self.assertIs(read_unit_system({}), CELSIUS)

    def test_reads_fahrenheit(self) -> None:
        self.assertIs(read_unit_system({"display": {"temperature_unit": "fahrenheit"}}), FAHRENHEIT)

    def test_rejects_unknown_values_and_types(self) -> None:
        for value in ("kelvin", "F", 5, None):
            with self.subTest(value=value):
                with self.assertRaisesRegex(InvalidDisplayConfiguration, "temperature_unit"):
                    read_unit_system({"display": {"temperature_unit": value}})

    def test_rejects_unknown_keys_and_non_objects(self) -> None:
        with self.assertRaisesRegex(InvalidDisplayConfiguration, "unknown keys"):
            read_unit_system({"display": {"unit": "celsius"}})
        with self.assertRaisesRegex(InvalidDisplayConfiguration, "must be an object"):
            read_unit_system({"display": "celsius"})

    def test_load_config_accepts_the_display_section(self) -> None:
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / "config.json"
        path.write_text(json.dumps({"display": {"temperature_unit": "celsius"}}))

        self.assertIs(read_unit_system(load_config(path)), CELSIUS)


if __name__ == "__main__":
    unittest.main()
