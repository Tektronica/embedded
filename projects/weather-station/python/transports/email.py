import smtplib
from email.message import EmailMessage

from domain.measurements import Measurement


class EmailTransport:
    def __init__(
        self,
        host: str,
        port: int,
        username: str,
        password: str,
        recipient: str,
    ) -> None:
        self._host = host
        self._port = port
        self._username = username
        self._password = password
        self._recipient = recipient

    def send(self, measurement: Measurement) -> None:
        message = EmailMessage()
        message["From"] = self._username
        message["To"] = self._recipient
        message["Subject"] = (
            f"Sensor update: {measurement.sensor_id} / {measurement.property_id}"
        )
        message.set_content(measurement.model_dump_json(indent=2))

        with smtplib.SMTP_SSL(self._host, self._port, timeout=30) as smtp:
            smtp.login(self._username, self._password)
            smtp.send_message(message)
