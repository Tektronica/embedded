import logging
import time
from collections.abc import Callable
from datetime import datetime, timedelta, timezone

from weather_station.sensors.contracts import Sensor
from weather_station.sensors.detection import DetectedSensor
from weather_station.sensors.errors import InvalidCollectorConfiguration, NoSensorsDetected
from weather_station.sensors.recording import RecordingService


PRUNE_INTERVAL = timedelta(hours=1)


class Collector:
    """Polls every detected sensor on one interval. One failing sensor never stops the others."""

    def __init__(
        self,
        detected: list[DetectedSensor],
        recorder: RecordingService,
        interval_seconds: int,
        retention_days: int,
        now: Callable[[], datetime] = lambda: datetime.now(timezone.utc),
    ) -> None:
        if interval_seconds <= 0:
            raise InvalidCollectorConfiguration("interval_seconds must be positive")
        if retention_days <= 0:
            raise InvalidCollectorConfiguration("retention_days must be positive")
        if not detected:
            raise NoSensorsDetected("No configured sensor answered at startup")
        self._sensors: list[Sensor] = [item.sensor for item in detected]
        self._recorder = recorder
        self._interval_seconds = interval_seconds
        self._retention = timedelta(days=retention_days)
        self._now = now
        self._last_pruned_at: datetime | None = None
        for item in detected:
            recorder.register_sensor(item.sensor.sensor_id, item.sensor.sensor_type)
            recorder.record_many(item.first_reading)

    def collect_once(self) -> int:
        stored = 0
        for sensor in self._sensors:
            try:
                measurements = sensor.read()
                self._recorder.record_many(measurements)
                stored += len(measurements)
            except Exception:
                logging.exception("Reading %s failed", sensor.sensor_id)
        return stored

    def run(self) -> None:
        try:
            self.prune()
            while True:
                time.sleep(self._interval_seconds)
                logging.info("Stored %d measurements", self.collect_once())
                if self._prune_is_due():
                    self.prune()
        finally:
            self.close()

    def prune(self) -> None:
        now = self._now()
        self._last_pruned_at = now
        cutoff = now - self._retention
        try:
            pruned = self._recorder.prune_observed_before(cutoff)
        except Exception:
            logging.exception("Pruning measurements older than %s failed", cutoff)
            return
        if pruned > 0:
            logging.info("Pruned %d measurements older than %s", pruned, cutoff)

    def _prune_is_due(self) -> bool:
        return self._last_pruned_at is None or self._now() - self._last_pruned_at >= PRUNE_INTERVAL

    def close(self) -> None:
        for sensor in self._sensors:
            try:
                sensor.close()
            except Exception:
                logging.exception("Closing %s failed", sensor.sensor_id)
