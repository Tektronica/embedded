import sqlite3
from collections.abc import Iterator
from contextlib import contextmanager
from datetime import datetime, timezone
from pathlib import Path
from uuid import UUID

from weather_station.shared.db import SCHEMA_VERSION
from weather_station.shared.errors import (
    DatabaseUnavailable,
    MeasurementPersistenceError,
    SchemaVersionMismatch,
)
from weather_station.shared.measurements import Measurement, MeasurementQuality, StoredMeasurement
from weather_station.shared.registry import get_property_definition


class SQLiteMeasurementReader:
    """Read-only access to the measurement database. Opens SQLite in mode=ro, so it can never write."""

    def __init__(self, database_path: str | Path) -> None:
        self.database_path = Path(database_path)

    @contextmanager
    def _connection(self) -> Iterator[sqlite3.Connection]:
        if not self.database_path.exists():
            raise DatabaseUnavailable(
                f"No database at {self.database_path}. Start the sensors service to create it."
            )
        connection: sqlite3.Connection | None = None
        try:
            connection = sqlite3.connect(
                f"file:{self.database_path.resolve()}?mode=ro", uri=True, timeout=10
            )
            connection.row_factory = sqlite3.Row
            connection.execute("PRAGMA busy_timeout = 10000")
            self._check_schema(connection)
            yield connection
        except sqlite3.Error as error:
            raise MeasurementPersistenceError("SQLite measurement read failed") from error
        finally:
            if connection is not None:
                connection.close()

    @staticmethod
    def _check_schema(connection: sqlite3.Connection) -> None:
        row = connection.execute("SELECT version FROM schema_version").fetchone()
        version = int(row["version"]) if row else None
        if version != SCHEMA_VERSION:
            raise SchemaVersionMismatch(
                f"Database schema version {version} does not match supported version {SCHEMA_VERSION}"
            )

    def registered_sensors(self) -> list[tuple[str, str]]:
        with self._connection() as connection:
            rows = connection.execute(
                "SELECT sensor_id, sensor_type FROM sensors ORDER BY sensor_id"
            ).fetchall()
        return [(str(row["sensor_id"]), str(row["sensor_type"])) for row in rows]

    def sensor_type(self, sensor_id: str) -> str | None:
        with self._connection() as connection:
            row = connection.execute(
                "SELECT sensor_type FROM sensors WHERE sensor_id = ?",
                (sensor_id,),
            ).fetchone()
        return str(row["sensor_type"]) if row else None

    def latest(self) -> list[StoredMeasurement]:
        with self._connection() as connection:
            rows = connection.execute(
                """
                SELECT m.*, s.sensor_type
                FROM measurements AS m
                JOIN sensors AS s USING (sensor_id)
                WHERE m.id = (
                    SELECT candidate.id
                    FROM measurements AS candidate
                    WHERE candidate.sensor_id = m.sensor_id
                      AND candidate.property_id = m.property_id
                    ORDER BY candidate.observed_at DESC, candidate.id DESC
                    LIMIT 1
                )
                ORDER BY m.sensor_id, m.property_id
                """
            ).fetchall()
        return [self._decode(row) for row in rows]

    def latest_for_sensor(self, sensor_id: str) -> list[StoredMeasurement]:
        return [item for item in self.latest() if item.measurement.sensor_id == sensor_id]

    def series(
        self,
        sensor_id: str,
        property_id: str,
        since: datetime,
        limit: int = 3000,
    ) -> list[StoredMeasurement]:
        with self._connection() as connection:
            rows = connection.execute(
                """
                SELECT m.*, s.sensor_type
                FROM measurements AS m
                JOIN sensors AS s USING (sensor_id)
                WHERE m.sensor_id = ? AND m.property_id = ? AND m.observed_at >= ?
                ORDER BY m.observed_at ASC, m.id ASC
                LIMIT ?
                """,
                (
                    sensor_id,
                    property_id,
                    since.astimezone(timezone.utc).isoformat(),
                    limit,
                ),
            ).fetchall()
        return [self._decode(row) for row in rows]

    @staticmethod
    def _decode(row: sqlite3.Row) -> StoredMeasurement:
        sensor_type = str(row["sensor_type"])
        measurement = Measurement(
            id=UUID(row["id"]),
            created_at=datetime.fromisoformat(row["created_at"]),
            observed_at=datetime.fromisoformat(row["observed_at"]),
            sensor_id=str(row["sensor_id"]),
            property_id=str(row["property_id"]),
            value=float(row["value"]),
            unit=str(row["unit"]),
            quality=MeasurementQuality(row["quality"]),
            uncertainty=float(row["uncertainty"]) if row["uncertainty"] is not None else None,
        )
        get_property_definition(sensor_type, measurement.property_id)
        return StoredMeasurement(sensor_type, measurement)
