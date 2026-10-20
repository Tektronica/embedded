import sqlite3
from collections.abc import Iterator
from contextlib import contextmanager
from datetime import timezone
from pathlib import Path
from uuid import UUID

from weather_station.sensors.errors import (
    InvalidSensorId,
    SensorNotRegistered,
    SensorRegistrationConflict,
)
from weather_station.shared.db import SCHEMA_VERSION, schema_sql
from weather_station.shared.errors import MeasurementPersistenceError, SchemaVersionMismatch
from weather_station.shared.measurements import Measurement
from weather_station.shared.registry import validate_measurement


class SQLiteMeasurementWriter:
    """The only writer of the measurement database. Creates the schema and records its version."""

    def __init__(self, database_path: str | Path) -> None:
        self.database_path = Path(database_path)
        self.database_path.parent.mkdir(parents=True, exist_ok=True)
        self._apply_schema()

    def _connect(self) -> sqlite3.Connection:
        connection = sqlite3.connect(self.database_path, timeout=10)
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA busy_timeout = 10000")
        connection.execute("PRAGMA foreign_keys = ON")
        return connection

    @contextmanager
    def _connection(self) -> Iterator[sqlite3.Connection]:
        connection: sqlite3.Connection | None = None
        try:
            connection = self._connect()
            yield connection
            connection.commit()
        except sqlite3.Error as error:
            if connection is not None:
                connection.rollback()
            raise MeasurementPersistenceError("SQLite measurement operation failed") from error
        finally:
            if connection is not None:
                connection.close()

    def _apply_schema(self) -> None:
        with self._connection() as connection:
            connection.execute("PRAGMA journal_mode = WAL")
            connection.executescript(schema_sql())
            row = connection.execute("SELECT version FROM schema_version").fetchone()
            if row is None:
                connection.execute(
                    "INSERT INTO schema_version (version) VALUES (?)", (SCHEMA_VERSION,)
                )
            elif int(row["version"]) != SCHEMA_VERSION:
                raise SchemaVersionMismatch(
                    f"Database schema version {row['version']} does not match {SCHEMA_VERSION}"
                )

    def register_sensor(self, sensor_id: str, sensor_type: str) -> None:
        if not sensor_id or sensor_id != sensor_id.strip():
            raise InvalidSensorId("sensor_id must be non-empty and have no surrounding whitespace")
        with self._connection() as connection:
            connection.execute("BEGIN IMMEDIATE")
            existing = connection.execute(
                "SELECT sensor_type FROM sensors WHERE sensor_id = ?",
                (sensor_id,),
            ).fetchone()
            if existing and existing["sensor_type"] != sensor_type:
                raise SensorRegistrationConflict(
                    f"Sensor {sensor_id!r} is already registered as {existing['sensor_type']!r}"
                )
            connection.execute(
                "INSERT OR IGNORE INTO sensors (sensor_id, sensor_type) VALUES (?, ?)",
                (sensor_id, sensor_type),
            )

    def sensor_type(self, sensor_id: str) -> str | None:
        with self._connection() as connection:
            row = connection.execute(
                "SELECT sensor_type FROM sensors WHERE sensor_id = ?",
                (sensor_id,),
            ).fetchone()
        return str(row["sensor_type"]) if row else None

    def save(self, measurement: Measurement) -> UUID:
        self.save_many([measurement])
        return measurement.id

    def save_many(self, measurements: list[Measurement]) -> None:
        validated: list[Measurement] = []
        for measurement in measurements:
            sensor_type = self.sensor_type(measurement.sensor_id)
            if sensor_type is None:
                raise SensorNotRegistered(f"Sensor {measurement.sensor_id!r} is not registered")
            validated.append(validate_measurement(sensor_type, measurement))

        rows = [
            (
                str(item.id),
                item.created_at.astimezone(timezone.utc).isoformat(),
                item.observed_at.astimezone(timezone.utc).isoformat(),
                item.sensor_id,
                item.property_id,
                item.value,
                item.unit,
                item.quality.value,
                item.uncertainty,
            )
            for item in validated
        ]
        with self._connection() as connection:
            connection.executemany(
                """
                INSERT INTO measurements
                    (id, created_at, observed_at, sensor_id, property_id, value, unit, quality, uncertainty)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                rows,
            )
