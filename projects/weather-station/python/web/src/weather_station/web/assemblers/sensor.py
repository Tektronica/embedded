from typing import Any

from weather_station.shared.registry import SENSOR_DEFINITIONS


class SensorAssembler:
    @staticmethod
    def catalog_to_json() -> list[dict[str, Any]]:
        return [
            {
                "sensor_type": definition.key,
                "label": definition.label,
                "properties": [
                    {
                        "key": prop.key,
                        "property_id": prop.property_id,
                        "label": prop.label,
                        "quantity_kind_uri": prop.quantity_kind_uri,
                        "unit": prop.si_unit,
                        "display_unit": prop.display_unit,
                    }
                    for prop in definition.properties
                ],
            }
            for definition in SENSOR_DEFINITIONS.values()
        ]
