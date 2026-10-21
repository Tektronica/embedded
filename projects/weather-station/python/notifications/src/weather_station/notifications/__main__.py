import logging
from datetime import datetime

from weather_station.notifications.digest import DigestBuilder
from weather_station.notifications.email import EmailNotifier
from weather_station.notifications.settings import NotificationSettings
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader


def main() -> None:
    """Sends one digest and exits. A systemd timer runs it on a schedule."""
    settings = NotificationSettings.from_config()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    digest = DigestBuilder(SQLiteMeasurementReader(settings.database_path), settings.unit_system)
    EmailNotifier(settings.smtp).send(digest.build(datetime.now().astimezone()))
    logging.info("Sent weather station digest")


if __name__ == "__main__":
    main()
