"""Process-wide configuration: env loading, paths, and logging setup."""

import logging
import os
import pathlib

from dotenv import load_dotenv

load_dotenv()

SERVER_DIR = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = SERVER_DIR / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")
SAVES_DIR = str(SERVER_DIR / "saves")


class _ComponentFilter(logging.Filter):
    """Expose a short `component` field (strip leading `rhapsode.`)."""

    def filter(self, record: logging.LogRecord) -> bool:
        name = record.name
        if name.startswith("rhapsode."):
            record.component = name[len("rhapsode.") :]  # type: ignore[attr-defined]
        elif name == "rhapsode":
            record.component = "rhapsode"  # type: ignore[attr-defined]
        else:
            record.component = name  # type: ignore[attr-defined]
        return True


def configure_logging() -> None:
    """Make the `rhapsode.*` loggers emit on stderr beside C++ pipeline logs.

    Line shape matches C++ log_util:
      HH:MM:SS LEVEL component: message

    Level via RHAPSODE_LOG_LEVEL (default INFO). RHAPSODE_VERBOSE_LOG is a C++
    gate for stage/prompt dumps; set RHAPSODE_LOG_LEVEL=DEBUG for Python detail.
    """
    level_name = os.environ.get("RHAPSODE_LOG_LEVEL", "INFO").upper()
    pkg_log = logging.getLogger("rhapsode")
    if not any(isinstance(h, logging.StreamHandler) for h in pkg_log.handlers):
        handler = logging.StreamHandler()
        handler.addFilter(_ComponentFilter())
        handler.setFormatter(logging.Formatter(
            "%(asctime)s %(levelname)-5s %(component)s: %(message)s", "%H:%M:%S"))
        pkg_log.addHandler(handler)
    pkg_log.setLevel(getattr(logging, level_name, logging.INFO))
    pkg_log.propagate = False


configure_logging()
