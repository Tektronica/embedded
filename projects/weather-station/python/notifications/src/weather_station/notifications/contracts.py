from dataclasses import dataclass
from typing import Protocol


@dataclass(frozen=True)
class Notification:
    subject: str
    body: str


class Notifier(Protocol):
    """One delivery channel. Email today, ntfy or others later, chosen in __main__."""

    def send(self, notification: Notification) -> None: ...
