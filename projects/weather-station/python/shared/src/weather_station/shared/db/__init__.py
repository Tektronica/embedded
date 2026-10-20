from importlib.resources import files

SCHEMA_VERSION = 1


def schema_sql() -> str:
    return files(__package__).joinpath("schema.sql").read_text(encoding="utf-8")
