import logging
import sys
from collections.abc import Callable
from datetime import datetime, timedelta
from pathlib import Path

from weather_station.notifications.contracts import Notification, Notifier
from weather_station.notifications.email import EmailNotifier
from weather_station.notifications.settings import NotificationSettings
from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.errors import DatabaseUnavailable, WeatherStationError
from weather_station.shared.measurements import Measurement

UPTIME_PATH = Path("/proc/uptime")


def read_uptime(path: Path = UPTIME_PATH) -> timedelta:
    seconds = float(path.read_text().split()[0])
    return timedelta(seconds=seconds)


def last_reading_before(reader: MeasurementReader, started_at: datetime) -> Measurement | None:
    try:
        stored = reader.latest_before(started_at)
    except DatabaseUnavailable:
        return None
    return max(
        (item.measurement for item in stored),
        key=lambda measurement: measurement.observed_at,
        default=None,
    )


def format_offline_gap(gap: timedelta) -> str:
    minutes = round(gap.total_seconds() / 60)
    if minutes < 1:
        return "less than 1 min"
    days, remainder = divmod(minutes, 24 * 60)
    hours, minutes = divmod(remainder, 60)
    if days:
        return f"{days} {'day' if days == 1 else 'days'} {hours} h"
    if hours:
        return f"{hours} h {minutes} min"
    return f"{minutes} min"


def format_notification(started_at: datetime, last: Measurement | None) -> Notification:
    lines = [f"The weather station started at {_format_time(started_at, started_at)}."]
    if last is None:
        lines.append("No earlier reading was found.")
    else:
        lines.append(
            f"Last reading before the start: {_format_time(last.observed_at, started_at)} ({last.sensor_id})."
        )
        lines.append(f"Offline for about {format_offline_gap(started_at - last.observed_at)}.")
    return Notification(subject="Weather station started", body="\n".join(lines) + "\n")


def _format_time(moment: datetime, started_at: datetime) -> str:
    return f"{moment.astimezone(started_at.tzinfo):%Y-%m-%d %H:%M %Z}"


def run_boot(
    reader: MeasurementReader,
    notifier_factory: Callable[[], Notifier],
    now: datetime,
    uptime_reader: Callable[[], timedelta],
) -> bool:
    """Returns False when the email could not be sent."""
    started_at = now - uptime_reader()
    try:
        last = last_reading_before(reader, started_at)
        notifier_factory().send(format_notification(started_at, last))
    except (WeatherStationError, OSError):
        logging.exception("Could not send the station started email")
        return False
    logging.info("Sent station started email for start at %s", started_at.isoformat())
    return True


def main() -> None:
    """Sends one station started email and exits. systemd runs it once per boot."""
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    try:
        settings = NotificationSettings.from_config()
    except WeatherStationError:
        logging.exception("Could not read the notification settings")
        sys.exit(1)
    succeeded = run_boot(
        SQLiteMeasurementReader(settings.database_path),
        lambda: EmailNotifier(settings.smtp),
        datetime.now().astimezone(),
        read_uptime,
    )
    sys.exit(0 if succeeded else 1)


if __name__ == "__main__":
    main()
