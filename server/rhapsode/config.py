"""Process-wide configuration: env loading, paths, and logging setup."""

import logging
import os
import pathlib

from dotenv import load_dotenv

load_dotenv()

SERVER_DIR = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = SERVER_DIR / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")
SAVES_DIR = str(SERVER_DIR / "saves")


def configure_logging() -> None:
    """Make the `rhapsode.*` loggers actually emit.

    Uvicorn only configures its own loggers, so application `log.info` calls are
    otherwise swallowed (root defaults to WARNING). Attach a stderr handler to
    the package logger so orchestration events -- session, scheduler, lifecycle,
    memory sync -- show alongside the C++ pipeline logs (which go to stderr too).
    Level is overridable via RHAPSODE_LOG_LEVEL (e.g. DEBUG, WARNING).
    """
    level_name = os.environ.get("RHAPSODE_LOG_LEVEL", "INFO").upper()
    pkg_log = logging.getLogger("rhapsode")
    if not any(isinstance(h, logging.StreamHandler) for h in pkg_log.handlers):
        handler = logging.StreamHandler()
        handler.setFormatter(logging.Formatter(
            "%(asctime)s %(levelname)-5s %(name)s: %(message)s", "%H:%M:%S"))
        pkg_log.addHandler(handler)
    pkg_log.setLevel(getattr(logging, level_name, logging.INFO))
    pkg_log.propagate = False


configure_logging()
