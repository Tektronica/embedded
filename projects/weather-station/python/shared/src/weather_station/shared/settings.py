import os
from pathlib import Path

from dotenv import find_dotenv, load_dotenv


def load_env() -> None:
    """Loads .env from the working directory or its nearest parent. Variables already set win."""
    load_dotenv(find_dotenv(usecwd=True))


def database_path() -> Path:
    return Path(os.getenv("DATABASE_PATH", "instance/weather.sqlite3"))
