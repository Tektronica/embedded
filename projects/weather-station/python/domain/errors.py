class WeatherStationError(Exception):
    code = "weather_station_error"


class UnknownSensorType(WeatherStationError):
    code = "unknown_sensor_type"


class UnknownProperty(WeatherStationError):
    code = "unknown_property"


class SensorRegistrationConflict(WeatherStationError):
    code = "sensor_registration_conflict"


class InvalidSensorId(WeatherStationError):
    code = "invalid_sensor_id"


class InvalidSensorConfiguration(WeatherStationError):
    code = "invalid_sensor_configuration"


class SensorUnavailable(WeatherStationError):
    code = "sensor_unavailable"


class SensorReadError(WeatherStationError):
    code = "sensor_read_error"


class SensorNotRegistered(WeatherStationError):
    code = "sensor_not_registered"


class InvalidMeasurementUnit(WeatherStationError):
    code = "invalid_measurement_unit"


class InvalidCollectorConfiguration(WeatherStationError):
    code = "invalid_collector_configuration"


class MeasurementPersistenceError(WeatherStationError):
    code = "measurement_persistence_error"


class RequestValidationError(WeatherStationError):
    code = "request_validation_error"

    def __init__(self, details: object) -> None:
        super().__init__("Request data failed validation")
        self.details = details
