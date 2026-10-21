import unittest
from unittest.mock import MagicMock, patch

from weather_station.notifications.contracts import Notification
from weather_station.notifications.email import EmailNotifier
from weather_station.notifications.settings import SmtpSettings


class EmailNotifierTests(unittest.TestCase):
    def test_to_header_joins_every_recipient(self) -> None:
        settings = SmtpSettings(
            "smtp.example.com", 465, "station@example.com", "secret", ["a@example.com", "b@example.com"]
        )

        with patch("weather_station.notifications.email.smtplib.SMTP_SSL") as smtp_class:
            smtp = MagicMock()
            smtp_class.return_value.__enter__.return_value = smtp
            EmailNotifier(settings).send(Notification("Subject", "Body"))

        sent = smtp.send_message.call_args.args[0]
        self.assertEqual(sent["To"], "a@example.com, b@example.com")
        smtp.login.assert_called_once_with("station@example.com", "secret")


if __name__ == "__main__":
    unittest.main()
