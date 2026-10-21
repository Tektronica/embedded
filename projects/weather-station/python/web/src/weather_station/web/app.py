import os
from datetime import datetime, timedelta, timezone
from typing import Any, cast

from flask import Flask, abort, current_app, jsonify, render_template, request

from weather_station.shared.contracts import MeasurementReader
from weather_station.shared.db.sqlite_reader import SQLiteMeasurementReader
from weather_station.shared.registry import get_sensor_definition
from weather_station.shared.settings import database_path, load_env
from weather_station.web.assemblers.dashboard import DashboardAssembler
from weather_station.web.assemblers.measurement import MeasurementAssembler
from weather_station.web.assemblers.sensor import SensorAssembler
from weather_station.web.errors import register_error_handlers


def create_app(test_config: dict[str, Any] | None = None) -> Flask:
    load_env()
    app = Flask(__name__)
    app.config.from_mapping(
        DATABASE_PATH=database_path(),
        STALE_AFTER=timedelta(minutes=int(os.getenv("STALE_AFTER_MINUTES", "5"))),
    )
    if test_config:
        app.config.update(test_config)

    reader = SQLiteMeasurementReader(app.config["DATABASE_PATH"])
    app.extensions["measurement_reader"] = reader
    app.extensions["dashboard_assembler"] = DashboardAssembler(reader, app.config["STALE_AFTER"])
    register_error_handlers(app)

    @app.get("/")
    def dashboard() -> str:
        view = _dashboard_assembler().assemble(
            request.args.get("sensor_id"),
            request.args.get("property"),
        )
        return render_template("dashboard.html", view=view)

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
