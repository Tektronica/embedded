from weather_station.shared.errors import WeatherStationError


class InvalidEmailConfiguration(WeatherStationError):
    code = "invalid_email_configuration"


class MissingSmtpCredential(WeatherStationError):
    code = "missing_smtp_credential"


class InvalidAlarmConfiguration(WeatherStationError):
    code = "invalid_alarm_configuration"


class InvalidAlarmState(WeatherStationError):
    code = "invalid_alarm_state"
