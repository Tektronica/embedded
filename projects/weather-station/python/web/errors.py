from http import HTTPStatus
from typing import Any

from flask import Flask, jsonify
from werkzeug.exceptions import HTTPException

from domain.errors import (
    InvalidCollectorConfiguration,
    InvalidMeasurementUnit,
    InvalidSensorConfiguration,
    InvalidSensorId,
    MeasurementPersistenceError,
    RequestValidationError,
    SensorNotRegistered,
    SensorReadError,
    SensorRegistrationConflict,
    SensorUnavailable,
    UnknownProperty,
    UnknownSensorType,
    WeatherStationError,
)


HTTP_STATUS_BY_ERROR: dict[type[WeatherStationError], HTTPStatus] = {
    InvalidSensorId: HTTPStatus.BAD_REQUEST,
    InvalidSensorConfiguration: HTTPStatus.UNPROCESSABLE_ENTITY,
    UnknownSensorType: HTTPStatus.BAD_REQUEST,
    UnknownProperty: HTTPStatus.NOT_FOUND,
    InvalidMeasurementUnit: HTTPStatus.UNPROCESSABLE_ENTITY,
    RequestValidationError: HTTPStatus.UNPROCESSABLE_ENTITY,
    SensorRegistrationConflict: HTTPStatus.CONFLICT,
    SensorNotRegistered: HTTPStatus.NOT_FOUND,
    SensorUnavailable: HTTPStatus.SERVICE_UNAVAILABLE,
    SensorReadError: HTTPStatus.SERVICE_UNAVAILABLE,
    MeasurementPersistenceError: HTTPStatus.SERVICE_UNAVAILABLE,
    InvalidCollectorConfiguration: HTTPStatus.INTERNAL_SERVER_ERROR,
}


def register_error_handlers(app: Flask) -> None:
    @app.errorhandler(WeatherStationError)
    def handle_weather_station_error(error: WeatherStationError) -> tuple[Any, int]:
        status = HTTP_STATUS_BY_ERROR.get(type(error), HTTPStatus.INTERNAL_SERVER_ERROR)
        details = getattr(error, "details", None)
        return _error_response(error.code, str(error), details), int(status)

    @app.errorhandler(HTTPException)
    def handle_http_error(error: HTTPException) -> tuple[Any, int]:
        status = HTTPStatus(error.code or HTTPStatus.INTERNAL_SERVER_ERROR)
        return _error_response(
            f"http_{status.value}",
            status.phrase,
            None,
        ), int(status)

    @app.errorhandler(Exception)
    def handle_unexpected_error(error: Exception) -> tuple[Any, int]:
        app.logger.exception("Unhandled web request failure", exc_info=error)
        return _error_response(
            "internal_error",
            "An unexpected server error occurred",
            None,
        ), int(HTTPStatus.INTERNAL_SERVER_ERROR)


def _error_response(code: str, message: str, details: object) -> Any:
    error: dict[str, object] = {"code": code, "message": message}
    if details is not None:
        error["details"] = details
    return jsonify(error=error)
