from datetime import datetime, timedelta, timezone
from typing import Any, cast

from flask import Flask, abort, current_app, jsonify, render_template, request

from config import database_path, load_settings
from dal.sqlite_readings import SQLiteMeasurementRepository
from domain.registry import get_sensor_definition
from services.measurement_service import MeasurementService
from web.assemblers.dashboard import DashboardAssembler
from web.assemblers.measurement import MeasurementAssembler
from web.assemblers.sensor import SensorAssembler
from web.errors import register_error_handlers


def create_app(test_config: dict[str, Any] | None = None) -> Flask:
    load_settings()
    app = Flask(__name__)
    app.config.from_mapping(DATABASE_PATH=database_path())
    if test_config:
        app.config.update(test_config)

    service = app.config.get("MEASUREMENT_SERVICE")
    if service is None:
        repository = SQLiteMeasurementRepository(app.config["DATABASE_PATH"])
        service = MeasurementService(repository)
    app.extensions["measurement_service"] = service
    app.extensions["dashboard_assembler"] = DashboardAssembler(service)
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
            [MeasurementAssembler.stored_to_json(item) for item in _service().latest()]
        )

    @app.post("/api/sensors")
    def register_sensor() -> Any:
        body = request.get_json(silent=True)
        registration = SensorAssembler.registration_from_request(body)
        _service().register_sensor(registration.sensor_id, registration.sensor_type)
        return jsonify(
            SensorAssembler.registration_to_json(
                registration.sensor_id,
                registration.sensor_type,
            )
        ), 201

    @app.post("/api/readings")
    def ingest_reading() -> Any:
        body = request.get_json(silent=True)
        measurement = MeasurementAssembler.from_request(body)
        measurement_id = _service().record(measurement)
        return jsonify(id=str(measurement_id)), 201

    @app.get("/api/series/<sensor_id>")
    def sensor_series(sensor_id: str) -> Any:
        sensor_type = _service().sensor_type(sensor_id)
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
        history = _service().history(
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


def _service() -> MeasurementService:
    return cast(MeasurementService, current_app.extensions["measurement_service"])


def _dashboard_assembler() -> DashboardAssembler:
    return cast(DashboardAssembler, current_app.extensions["dashboard_assembler"])


def _thirty_days_ago() -> datetime:
    return datetime.now(timezone.utc) - timedelta(days=30)
