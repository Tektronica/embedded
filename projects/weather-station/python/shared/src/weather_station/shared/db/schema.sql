-- The only contract between the sensors package (sole writer) and its readers (web, notifications).
-- Bump SCHEMA_VERSION in weather_station/shared/db/__init__.py with any change here.
CREATE TABLE IF NOT EXISTS schema_version (
    version INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS sensors (
    sensor_id TEXT PRIMARY KEY,
    sensor_type TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS measurements (
    id TEXT PRIMARY KEY,
    created_at TEXT NOT NULL,
    observed_at TEXT NOT NULL,
    sensor_id TEXT NOT NULL REFERENCES sensors(sensor_id),
    property_id TEXT NOT NULL,
    value REAL NOT NULL,
    unit TEXT NOT NULL,
    quality TEXT NOT NULL,
    uncertainty REAL
);
CREATE INDEX IF NOT EXISTS measurements_sensor_property_time
    ON measurements (sensor_id, property_id, observed_at DESC);
