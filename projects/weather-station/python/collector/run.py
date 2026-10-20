import logging
import os

from collector.service import run
from config import database_path, load_settings
from dal.sqlite_readings import SQLiteMeasurementRepository
from sensors.am2302 import AM2302Sensor
from services.measurement_service import MeasurementService
from transports.email import EmailTransport


def main() -> None:
    load_settings()
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
    )
    sensor = AM2302Sensor(
        sensor_id=os.getenv("SENSOR_ID", "room-am2302"),
        gpio_pin=int(os.getenv("GPIO_PIN", "4")),
    )
    repository = SQLiteMeasurementRepository(database_path())
    service = MeasurementService(repository)
    transport = EmailTransport(
        host=os.environ["SMTP_HOST"],
        port=int(os.getenv("SMTP_PORT", "465")),
        username=os.environ["SMTP_USER"],
        password=os.environ["SMTP_PASSWORD"],
        recipient=os.environ["EMAIL_TO"],
    )
    run(
        sensor,
        service,
        interval_seconds=int(os.getenv("INTERVAL_SECONDS", "900")),
        transport=transport,
    )


if __name__ == "__main__":
    main()