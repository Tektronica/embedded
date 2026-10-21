import logging
import sys
from collections import defaultdict
from collections.abc import Callable
from datetime import datetime

from weather_station.notifications.alarm_rules import AlarmRule
from weather_station.notifications.alarm_state import AlarmStateStore
from weather_station.notifications.alarms import (
    AlarmReading,
    SensorSnapshot,
    evaluate_alarms,
    format_notification,
)
from weather_station.notifications.contracts import Notifier
from weather_station.notifications.email import EmailNotifier
from weather_station.notifications.settings import AlarmSettings, load_smtp_settings
from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.errors import WeatherStationError
from weather_station.shared.measurements import StoredMeasurement
from weather_station.shared.registry import get_property_definition


def read_snapshots(reader: MeasurementReader, rules: list[AlarmRule]) -> dict[str, SensorSnapshot]:
    sensors = {rule.sensor for rule in rules}
    stored_by_sensor: defaultdict[str, list[StoredMeasurement]] = defaultdict(list)
    for stored in reader.latest():
        if stored.measurement.sensor_id in sensors:
            stored_by_sensor[stored.measurement.sensor_id].append(stored)
    return {sensor: _snapshot(items) for sensor, items in stored_by_sensor.items()}


def _snapshot(items: list[StoredMeasurement]) -> SensorSnapshot:
    readings: dict[str, AlarmReading] = {}
    for stored in items:
        prop = get_property_definition(stored.sensor_type, stored.measurement.property_id)
        readings[prop.key] = AlarmReading(
            property=prop,
            value=prop.display_value(stored.measurement.value),
            si_value=stored.measurement.value,
            observed_at=stored.measurement.observed_at,
        )
    return SensorSnapshot(
        last_observed_at=max(item.measurement.observed_at for item in items),
        readings=readings,
    )


def run_alarms(
    settings: AlarmSettings,
    reader: MeasurementReader,
    store: AlarmStateStore,
    notifier_factory: Callable[[], Notifier],
    now: datetime,
) -> bool:
    """Returns False when the alarm email could not be sent. The state is then left as it was."""
    previous = store.load()
    snapshots = read_snapshots(reader, settings.rules) if settings.rules else {}
    state, events = evaluate_alarms(settings.rules, snapshots, previous, now, settings.stale_after)
    logging.info("Checked %d rules, %d events", len(settings.rules), len(events))
    if events:
        try:
            notifier_factory().send(format_notification(events, now, settings.unit_system))
        except (WeatherStationError, OSError):
            logging.exception("Could not send the alarm email for %d events", len(events))
            return False
        logging.info("Sent alarm email for %d events", len(events))
    store.save(state)
    return True


def main() -> None:
    """Checks every alarm rule once and exits. A systemd timer runs it every minute."""
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    settings = AlarmSettings.from_config()
    succeeded = run_alarms(
        settings,
        SQLiteMeasurementReader(settings.database_path),
        AlarmStateStore(settings.state_path),
        lambda: EmailNotifier(load_smtp_settings()),
        datetime.now().astimezone(),
    )
    sys.exit(0 if succeeded else 1)


if __name__ == "__main__":
    main()
