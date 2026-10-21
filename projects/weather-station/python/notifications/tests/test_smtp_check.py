import io
import json
import os
import smtplib
import ssl
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import MagicMock, patch

from weather_station.notifications.smtp_check import main

PASSWORD = "app-password-secret"
CONFIG = {
    "email": {
        "smtp_host": "smtp.example.com",
        "smtp_port": 465,
        "recipients": ["a@example.com", "b@example.com"],
    }
}


class SmtpCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        directory = TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.config_path = Path(directory.name) / "config.json"
        self.config_path.write_text(json.dumps(CONFIG), encoding="utf-8")

        environment = patch.dict(os.environ, {"SMTP_USER": "station@example.com", "SMTP_PASSWORD": PASSWORD})
        environment.start()
        self.addCleanup(environment.stop)
        load_env = patch("weather_station.notifications.settings.load_env")
        load_env.start()
        self.addCleanup(load_env.stop)

        smtp_patch = patch("smtplib.SMTP_SSL")
        self.smtp_class = smtp_patch.start()
        self.addCleanup(smtp_patch.stop)
        self.smtp = MagicMock()
        self.smtp_class.return_value.__enter__.return_value = self.smtp

    def run_main(self, *args: str) -> tuple[int, str]:
        output = io.StringIO()
        with redirect_stdout(output):
            code = main(list(args), self.config_path)
        return code, output.getvalue()

    def test_login_only_logs_in_and_sends_nothing(self) -> None:
        code, output = self.run_main()

        self.assertEqual(code, 0)
        self.assertEqual(output, "Logged in to smtp.example.com:465 as station@example.com\n")
        self.smtp_class.assert_called_once_with("smtp.example.com", 465, timeout=30)
        self.smtp.login.assert_called_once_with("station@example.com", PASSWORD)
        self.smtp.send_message.assert_not_called()
        self.assertNotIn(PASSWORD, output)

    def test_send_delivers_one_message_to_every_recipient(self) -> None:
        with patch("weather_station.notifications.smtp_check.socket.gethostname", return_value="station-test"):
            code, output = self.run_main("--send")

        self.assertEqual(code, 0)
        self.assertEqual(output, "Sent a test email to 2 recipients\n")
        self.smtp.send_message.assert_called_once()
        sent = self.smtp.send_message.call_args.args[0]
        self.assertEqual(sent["To"], "a@example.com, b@example.com")
        self.assertEqual(sent["Subject"], "Weather station test email")
        self.assertIn("Sent by smtp_check from station-test.", sent.get_content())

    def test_authentication_failure_exits_one_without_printing_password(self) -> None:
        self.smtp.login.side_effect = smtplib.SMTPAuthenticationError(535, b"Username and Password not accepted")

        code, output = self.run_main()

        self.assertEqual(code, 1)
        self.assertEqual(output, "Authentication failed. Check SMTP_USER and SMTP_PASSWORD.\n")
        self.assertNotIn(PASSWORD, output)

    def test_connection_timeout_exits_one(self) -> None:
        self.smtp_class.side_effect = TimeoutError("timed out")

        code, output = self.run_main()

        self.assertEqual(code, 1)
        self.assertEqual(output, "Could not connect or timed out: smtp.example.com:465\n")

    def test_tls_error_exits_one(self) -> None:
        self.smtp_class.side_effect = ssl.SSLError("wrong version number")

        code, output = self.run_main()

        self.assertEqual(code, 1)
        self.assertEqual(output, "TLS error with smtp.example.com:465\n")

    def test_missing_password_exits_one_as_configuration_error(self) -> None:
        with patch.dict(os.environ):
            os.environ.pop("SMTP_PASSWORD", None)
            code, output = self.run_main()

        self.assertEqual(code, 1)
        self.assertEqual(output, "Configuration error: Environment variable SMTP_PASSWORD is not set\n")
        self.smtp_class.assert_not_called()


if __name__ == "__main__":
    unittest.main()
