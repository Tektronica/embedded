import os
from pathlib import Path

from dotenv import load_dotenv


PYTHON_ROOT = Path(__file__).resolve().parent


def load_settings() -> None:
    load_dotenv(PYTHON_ROOT / ".env")


def database_path() -> Path:
    return Path(os.getenv("DATABASE_PATH", str(PYTHON_ROOT / "instance" / "weather.sqlite3")))