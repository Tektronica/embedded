import json
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from weather_station.shared.errors import ConfigFileError, InvalidStorageConfiguration
from weather_station.shared.settings import database_path, load_config


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


if __name__ == "__main__":
    unittest.main()
