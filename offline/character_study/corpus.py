"""Load the novel, volume map, and Wikipedia briefs."""

from __future__ import annotations

import json
import os
import re
from pathlib import Path
from typing import Any

import yaml

ROOT = Path(__file__).resolve().parent
WIKI = "https://en.wikipedia.org/wiki/List_of_KonoSuba_volumes"
NOVEL_ENV = "CHARACTER_STUDY_NOVEL"


def load_local_env() -> None:
    """Load KEY=value from this package's .env only. Does not read server/.env."""
    path = ROOT / ".env"
    if not path.is_file():
        return
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, val = line.split("=", 1)
        key = key.strip()
        if key and key not in os.environ:
            os.environ[key] = val.strip()


def resolve_config_path(path: str | Path | None = None) -> Path:
    raw = Path(path) if path else Path("config.yaml")
    if raw.is_absolute():
        return raw
    return ROOT / raw


def load_config(path: str | Path | None = None) -> dict[str, Any]:
    with resolve_config_path(path).open(encoding="utf-8") as f:
        return yaml.safe_load(f)


def novel_path(cfg: dict | None = None) -> Path:
    env = os.environ.get(NOVEL_ENV, "").strip()
    if env:
        return Path(env).expanduser().resolve()
    cfg = cfg or load_config()
    raw = Path(cfg["paths"]["novel"])
    if raw.is_absolute():
        return raw
    return (ROOT / raw).resolve()


def load_lines(cfg: dict | None = None) -> list[str]:
    return novel_path(cfg).read_text(encoding="utf-8").splitlines()


def load_volumes(cfg: dict | None = None) -> list[dict]:
    cfg = cfg or load_config()
    path = ROOT / cfg["paths"]["volumes"]
    with path.open(encoding="utf-8") as f:
        data = json.load(f)
    return data["volumes"]


def brief_path(volume: int, cfg: dict | None = None) -> Path:
    cfg = cfg or load_config()
    return ROOT / cfg["paths"]["briefs"] / f"vol{volume:02d}.md"


def read_brief(volume: int, cfg: dict | None = None) -> str:
    path = brief_path(volume, cfg)
    if not path.is_file():
        return f"(no brief for volume {volume})"
    return path.read_text(encoding="utf-8")


def hit_pattern(names: list[str]) -> re.Pattern[str]:
    return re.compile("|".join(re.escape(n) for n in names), re.IGNORECASE)


def corpus_ready(cfg: dict | None = None) -> bool:
    cfg = cfg or load_config()
    vols = ROOT / cfg["paths"]["volumes"]
    return novel_path(cfg).is_file() and vols.is_file()
