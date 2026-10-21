class WeatherStationError(Exception):
    code = "weather_station_error"


class UnknownSensorType(WeatherStationError):
    code = "unknown_sensor_type"


class UnknownProperty(WeatherStationError):
    code = "unknown_property"


class InvalidMeasurementUnit(WeatherStationError):
    code = "invalid_measurement_unit"


class MeasurementPersistenceError(WeatherStationError):
    code = "measurement_persistence_error"


class DatabaseUnavailable(WeatherStationError):
    code = "database_unavailable"


class SchemaVersionMismatch(WeatherStationError):
    code = "schema_version_mismatch"


class ConfigFileError(WeatherStationError):
    code = "config_file_error"


class InvalidStorageConfiguration(WeatherStationError):
    code = "invalid_storage_configuration"


class InvalidStatusConfiguration(WeatherStationError):
    code = "invalid_status_configuration"


class InvalidDisplayConfiguration(WeatherStationError):
    code = "invalid_display_configuration"
