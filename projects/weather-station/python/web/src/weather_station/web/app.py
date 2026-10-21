from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any, cast

from flask import Flask, Response, abort, current_app, jsonify, make_response, render_template, request

from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.registry import get_sensor_definition
from weather_station.shared.settings import (
    DEFAULT_CONFIG_PATH,
    database_path,
    load_config,
    read_stale_after_minutes,
    read_unit_system,
)
from weather_station.web.assemblers.dashboard import DashboardAssembler
from weather_station.web.assemblers.measurement import MeasurementAssembler
from weather_station.web.assemblers.sensor import SensorAssembler
from weather_station.web.errors import register_error_handlers
from weather_station.web.unit_preference import UNIT_COOKIE, UNIT_COOKIE_MAX_AGE, resolve_unit


def create_app(
    test_config: dict[str, Any] | None = None,
    config_path: Path = DEFAULT_CONFIG_PATH,
) -> Flask:
    app = Flask(__name__)
    document = load_config(config_path)
    app.config.from_mapping(
        DATABASE_PATH=database_path(config_path),
        STALE_AFTER=timedelta(minutes=read_stale_after_minutes(document)),
        DEFAULT_UNIT_SYSTEM=read_unit_system(document),
    )
    if test_config:
        app.config.update(test_config)

    reader = SQLiteMeasurementReader(app.config["DATABASE_PATH"])
    app.extensions["measurement_reader"] = reader
    app.extensions["dashboard_assembler"] = DashboardAssembler(reader, app.config["STALE_AFTER"])
    register_error_handlers(app)

    @app.get("/")
    def dashboard() -> Response:
        choice = resolve_unit(
            request.args.get("unit"),
            request.cookies.get(UNIT_COOKIE),
            app.config["DEFAULT_UNIT_SYSTEM"],
        )
        view = _dashboard_assembler().assemble(
            request.args.get("sensor_id"),
            request.args.get("property"),
            choice.system,
        )
        response = make_response(render_template("dashboard.html", view=view))
        response.vary.add("Cookie")
        if choice.from_query:
            response.set_cookie(
                UNIT_COOKIE,
                choice.system.code,
                max_age=UNIT_COOKIE_MAX_AGE,
                samesite="Lax",
                httponly=True,
            )
        return response

    @app.get("/api/readings")
    def latest_readings() -> Any:
        return jsonify(
            [MeasurementAssembler.stored_to_json(item) for item in _reader().latest()]
        )

    @app.get("/api/series/<sensor_id>")
    def sensor_series(sensor_id: str) -> Any:
        sensor_type = _reader().sensor_type(sensor_id)
        if sensor_type is None:
            abort(404)
        definition = get_sensor_definition(sensor_type)
        property_key = request.args.get("property")
        property_definition = next(
            (item for item in definition.properties if item.key == property_key),
            None,
        )
        if property_definition is None:
            abort(404)
        history = _reader().series(
            sensor_id,
            property_definition.property_id,
            _thirty_days_ago(),
        )
        return jsonify(
            sensor_id=sensor_id,
            property_id=property_definition.property_id,
            unit=property_definition.si_unit,
            measurements=[
                MeasurementAssembler.to_json(item.measurement) for item in history
            ],
        )

    @app.get("/api/sensor-types")
    def sensor_types() -> Any:
        return jsonify(SensorAssembler.catalog_to_json())

    return app


def _reader() -> MeasurementReader:
    return cast(MeasurementReader, current_app.extensions["measurement_reader"])


def _dashboard_assembler() -> DashboardAssembler:
    return cast(DashboardAssembler, current_app.extensions["dashboard_assembler"])


def _thirty_days_ago() -> datetime:
    return datetime.now(timezone.utc) - timedelta(days=30)
