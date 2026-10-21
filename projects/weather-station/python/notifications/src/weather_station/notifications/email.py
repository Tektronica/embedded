import smtplib
from email.message import EmailMessage

from weather_station.notifications.contracts import Notification
from weather_station.notifications.settings import SmtpSettings


class EmailNotifier:
    def __init__(self, settings: SmtpSettings) -> None:
        self._settings = settings

    def send(self, notification: Notification) -> None:
        message = EmailMessage()
        message["From"] = self._settings.username
        message["To"] = ", ".join(self._settings.recipients)
        message["Subject"] = notification.subject
        message.set_content(notification.body)

        with smtplib.SMTP_SSL(self._settings.host, self._settings.port, timeout=30) as smtp:
            smtp.login(self._settings.username, self._settings.password)
            smtp.send_message(message)
