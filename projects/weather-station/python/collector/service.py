import logging
import time

from domain.contracts import Sensor, Transport
from domain.errors import InvalidCollectorConfiguration
from domain.measurements import Measurement
from services.measurement_service import MeasurementService


def collect_once(
    sensor: Sensor,
    service: MeasurementService,
    transport: Transport | None = None,
) -> list[Measurement]:
    measurements = sensor.read()
    service.record_many(measurements)
    if transport is not None:
        for measurement in measurements:
            transport.send(measurement)
    return measurements


def run(
    sensor: Sensor,
    service: MeasurementService,
    interval_seconds: int,
    transport: Transport | None = None,
) -> None:
    if interval_seconds <= 0:
        sensor.close()
        raise InvalidCollectorConfiguration("interval_seconds must be positive")
    try:
        service.register_sensor(sensor.sensor_id, sensor.sensor_type)
        while True:
            try:
                measurements = collect_once(sensor, service, transport)
                logging.info(
                    "Stored %d measurements for %s",
                    len(measurements),
                    measurements[0].sensor_id if measurements else "sensor",
                )
            except Exception:
                logging.exception("Collection cycle failed")
            time.sleep(interval_seconds)
    finally:
        sensor.close()