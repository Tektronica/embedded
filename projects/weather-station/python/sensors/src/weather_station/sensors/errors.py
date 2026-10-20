from weather_station.shared.errors import WeatherStationError


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


class SensorRegistrationConflict(WeatherStationError):
    code = "sensor_registration_conflict"


class InvalidCollectorConfiguration(WeatherStationError):
    code = "invalid_collector_configuration"
