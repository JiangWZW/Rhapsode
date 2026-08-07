"""Scripted-line injection and save seeding for the session eval pipeline.

Stdlib-only, no server/LLM imports, so it can be exercised standalone
(see experiments/test_injection.py). run.py wires it in via --inject and
--seed-saves.

Turn counting contract: the C++ runner (core/src/eval/session_eval.cpp)
invokes the Python player callback exactly once per turn — a single
``player_llm_(prompt)`` call inside the ``for turn = 1..max_turns`` loop,
with no retry path (an exception ends the run; the bad-action retry in
player_agent.py happens *inside* one callback call). So the wrapper's
invocation count is the 1-based turn number relative to run start.
"""
from __future__ import annotations

import json
import shutil
import tomllib
from collections.abc import Callable
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path


@dataclass(frozen=True)
class Injection:
    turn: int
    text: str
    label: str


def load_injections(path: str | Path) -> dict[int, Injection]:
    """Load rows of {turn, text, label} from TOML ([[injection]]) or JSON.

    JSON: a top-level list of objects. TOML: an ``[[injection]]`` array of
    tables (``[[injections]]`` also accepted), so an experiment spec file
    can double as the injection file.
    """
    path = Path(path)
    if path.suffix.lower() == ".json":
        rows = json.loads(path.read_text(encoding="utf-8"))
    else:
        with path.open("rb") as f:
            data = tomllib.load(f)
        rows = data.get("injection", data.get("injections", []))
    table: dict[int, Injection] = {}
    for row in rows:
        inj = Injection(
            turn=int(row["turn"]),
            text=str(row["text"]).strip(),
            label=str(row.get("label", "")),
        )
        if inj.turn < 1:
            raise ValueError(f"injection turn must be >= 1, got {inj.turn}")
        if inj.turn in table:
            raise ValueError(f"duplicate injection for turn {inj.turn}")
        if not inj.text:
            raise ValueError(f"empty injection text for turn {inj.turn}")
        table[inj.turn] = inj
    return table


def _log(log_path: Path | None, line: str) -> None:
    stamped = f"[{datetime.now().isoformat(timespec='seconds')}] {line}"
    print(stamped, flush=True)
    if log_path is not None:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        with log_path.open("a", encoding="utf-8") as f:
            f.write(stamped + "\n")


def wrap_player(
    player: Callable[[str], str],
    injections: dict[int, Injection],
    log_path: str | Path | None = None,
) -> Callable[[str], str]:
    """Wrap the player callback: on injected turns return the scripted text
    directly (player LLM never called); otherwise delegate untouched."""
    log_file = Path(log_path) if log_path is not None else None
    state = {"turn": 0}

    def wrapped(prompt: str) -> str:
        state["turn"] += 1
        turn = state["turn"]
        inj = injections.get(turn)
        if inj is None:
            return player(prompt)
        _log(
            log_file,
            f"INJECT turn={turn} label={inj.label!r} "
            f"({len(inj.text)} chars, player LLM skipped): {inj.text}",
        )
        return inj.text

    return wrapped


def seed_saves(
    src: str | Path,
    dest: str | Path,
    log_path: str | Path | None = None,
) -> Path | None:
    """Copy save files from ``src`` into ``dest`` (the dir the server loads
    from: rhapsode.config.SAVES_DIR = <server>/saves).

    Never overwrites silently: a non-empty ``dest`` is first *moved* to a
    sibling ``<dest>_backup_<timestamp>`` folder. ``src`` is only read.
    Returns the backup path, or None if no backup was needed.
    """
    src, dest = Path(src), Path(dest)
    log_file = Path(log_path) if log_path is not None else None
    if not src.is_dir():
        raise FileNotFoundError(f"seed-saves source dir not found: {src}")
    backup: Path | None = None
    if dest.is_dir() and any(dest.iterdir()):
        ts = datetime.now().strftime("%Y%m%d-%H%M%S")
        backup = dest.with_name(f"{dest.name}_backup_{ts}")
        dest.rename(backup)
        _log(log_file, f"SEED moved existing saves: {dest} -> {backup}")
    shutil.copytree(src, dest, dirs_exist_ok=True)
    copied = sorted(p.name for p in dest.iterdir())
    _log(
        log_file,
        f"SEED copied {len(copied)} file(s) {src} -> {dest}: {', '.join(copied)}",
    )
    return backup
