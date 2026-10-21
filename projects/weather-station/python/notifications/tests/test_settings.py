import json
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Any
from unittest.mock import patch

from weather_station.notifications.errors import InvalidEmailConfiguration, MissingSmtpCredential
from weather_station.notifications.settings import AlarmSettings, NotificationSettings

CREDENTIALS = {"SMTP_USER": "station@example.com", "SMTP_PASSWORD": "app-password"}
EMAIL = {"smtp_host": "smtp.example.com", "recipients": ["a@example.com", "b@example.com"]}


class NotificationSettingsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "config.json"
        load_env = patch("weather_station.notifications.settings.load_env")
        load_env.start()
        self.addCleanup(load_env.stop)

    def tearDown(self) -> None:
        self.directory.cleanup()

    def _load(self, email: Any, environment: dict[str, str] | None = None) -> NotificationSettings:
        self.path.write_text(json.dumps({"email": email}))
        with patch.dict("os.environ", CREDENTIALS if environment is None else environment, clear=True):
            return NotificationSettings.from_config(self.path)

    def test_reads_the_email_section_and_credentials(self) -> None:
        settings = self._load({**EMAIL, "smtp_port": 587})

        self.assertEqual(settings.smtp.host, "smtp.example.com")
        self.assertEqual(settings.smtp.port, 587)
        self.assertEqual(settings.smtp.recipients, ["a@example.com", "b@example.com"])
        self.assertEqual(settings.smtp.username, "station@example.com")
        self.assertEqual(settings.smtp.password, "app-password")
        self.assertEqual(settings.database_path, Path("instance/weather.sqlite3"))

    def test_port_defaults_to_465(self) -> None:
        self.assertEqual(self._load(EMAIL).smtp.port, 465)

    def test_missing_recipients_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "recipients"):
            self._load({"smtp_host": "smtp.example.com"})

    def test_empty_recipients_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "recipients"):
            self._load({**EMAIL, "recipients": []})

    def test_recipients_must_be_strings(self) -> None:
        for recipients in ("a@example.com", [1], [""]):
            with self.subTest(recipients=recipients):
                with self.assertRaisesRegex(InvalidEmailConfiguration, "recipients"):
                    self._load({**EMAIL, "recipients": recipients})

    def test_missing_host_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "smtp_host"):
            self._load({"recipients": ["a@example.com"]})

    def test_host_must_be_a_string(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "smtp_host"):
            self._load({**EMAIL, "smtp_host": 5})

    def test_port_rejects_wrong_type_and_bool(self) -> None:
        for port in ("465", 465.0, True):
            with self.subTest(port=port):
                with self.assertRaisesRegex(InvalidEmailConfiguration, "integer"):
                    self._load({**EMAIL, "smtp_port": port})

    def test_unknown_key_is_rejected(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "recipient"):
            self._load({**EMAIL, "recipient": "a@example.com"})

    def test_section_must_be_an_object(self) -> None:
        with self.assertRaisesRegex(InvalidEmailConfiguration, "must be an object"):
            self._load([])

    def test_missing_smtp_credentials_name_the_variable(self) -> None:
        for name in CREDENTIALS:
            with self.subTest(name=name):
                environment = {key: value for key, value in CREDENTIALS.items() if key != name}
                with self.assertRaisesRegex(MissingSmtpCredential, name):
                    self._load(EMAIL, environment)


class AlarmSettingsTests(unittest.TestCase):
    def setUp(self) -> None:
        self.directory = TemporaryDirectory()
        self.path = Path(self.directory.name) / "config.json"

    def tearDown(self) -> None:
        self.directory.cleanup()

    def test_reads_rules_stale_window_and_state_path_without_email_settings(self) -> None:
        rule = {"sensor": "room", "property": "air-temperature", "above": 35}
        self.path.write_text(
            json.dumps(
                {
                    "storage": {"database_path": "data/weather.sqlite3"},
                    "status": {"stale_after_minutes": 9},
                    "alarms": [rule],
                }
            )
        )

        with patch.dict("os.environ", {}, clear=True):
            settings = AlarmSettings.from_config(self.path)

        self.assertEqual(settings.state_path, Path("data/alarm-state.json"))
        self.assertEqual(settings.stale_after.total_seconds(), 9 * 60)
        self.assertEqual([item.key for item in settings.rules], ["room/air-temperature"])


if __name__ == "__main__":
    unittest.main()
