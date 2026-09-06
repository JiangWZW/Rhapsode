"""Study A/B through session eval. Stdlib only. Does not import rhapsode."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import threading
import tomllib
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[1]
PRODUCTION = REPO / "server" / "scenarios" / "konosuba.json"
RUN_PY = REPO / "experiments" / "session_pipeline" / "run.py"
PIPE = REPO / "experiments" / "session_pipeline"
SERVER_PY = REPO / "server" / ".venv" / "Scripts" / "python.exe"
STUDIES = {
    "Darkness": ROOT / "study" / "darkness.md",
    "Megumin": ROOT / "study" / "megumin.md",
}
CAST = tuple(STUDIES)
MODES = ("none", "short", "full")
TURNS = 5
INJECTIONS = (
    (
        1,
        "board",
        'I take the Brutal Alligator notice from the board and read the reward twice. "Luna, what aren\'t you telling us about this one?"',
    ),
    (
        2,
        "vote",
        'I put the notice in the middle of the table. "Before we vote: one practical objection each."',
    ),
    (
        3,
        "pocket",
        'Darkness has touched the same spot on her armor three times. I look at the pocket, then at her. "Is that going to follow us into the sewer?"',
    ),
    (
        4,
        "coming",
        'I look at Darkness. "Are you coming with us into the sewer?"',
    ),
    (
        5,
        "formation",
        'I slide the notice to Darkness. "If we\'re doing this, give us the formation."',
    ),
)
INJECT_BY_LABEL = {row[1]: row for row in INJECTIONS}
REF_HEAD = (
    "{name} reference (temporary test only; play her as a person in this "
    "scene, not as an essay):"
)
SHORT_HEAD = "{name}, this scene only:"
SHORT_NOTES = {
    "Darkness": (
        "She wants the front of any real danger and is embarrassed by that want. "
        "She may volunteer first. She does not lecture."
    ),
    "Megumin": (
        "She knows a blast under the market costs the pay. "
        "She will argue the job. She does not give a speech about her life."
    ),
}
PERCEPTION_RE = re.compile(
    r"perception:\s+(Darkness|Megumin)\s+t=(\d+)(?:\s+\(empty\)|\s+(.*))$"
)


def study_prose(text: str) -> str:
    text = text.strip()
    if text.startswith("# "):
        _, _, rest = text.partition("\n")
        return rest.strip()
    return text


def character(scenario: dict[str, Any], name: str) -> dict[str, Any]:
    for row in scenario.get("characters") or []:
        if row.get("name") == name:
            return row
    raise SystemExit(f"missing character {name}")


def reference_block(name: str, body: str, head: str = REF_HEAD) -> str:
    return head.format(name=name) + "\n" + body.strip()


def apply_references(
    system_prompt: str,
    refs: dict[str, str],
    head: str = REF_HEAD,
) -> str:
    blocks = [reference_block(name, refs[name], head) for name in CAST]
    return system_prompt.rstrip() + "\n\n" + "\n\n".join(blocks) + "\n"


def prompt_words(text: str) -> int:
    return len(text.split())


def build_scenarios(
    production: dict[str, Any],
    studies: dict[str, str],
    mode: str = "none",
) -> tuple[dict, dict]:
    if mode not in MODES:
        raise SystemExit(f"unknown mode {mode}")
    old = json.loads(json.dumps(production))
    new = json.loads(json.dumps(production))
    old_refs: dict[str, str] = {}
    new_refs: dict[str, str] = {}
    for name, prose in studies.items():
        old_refs[name] = str(character(old, name).get("core") or "")
        character(new, name)["core"] = prose
        new_refs[name] = prose
    old_style = str(old.get("system_prompt") or "")
    new_style = str(new.get("system_prompt") or "")
    if mode == "none":
        old["system_prompt"] = old_style
        new["system_prompt"] = new_style
    elif mode == "short":
        old["system_prompt"] = apply_references(old_style, SHORT_NOTES, SHORT_HEAD)
        new["system_prompt"] = apply_references(new_style, SHORT_NOTES, SHORT_HEAD)
    else:
        old["system_prompt"] = apply_references(old_style, old_refs)
        new["system_prompt"] = apply_references(new_style, new_refs)
    return old, new


def select_injections(labels: list[str]) -> list[tuple[int, str, str]]:
    if not labels:
        labels = [row[1] for row in INJECTIONS]
    chosen: list[tuple[int, str, str]] = []
    for index, label in enumerate(labels, 1):
        if label not in INJECT_BY_LABEL:
            raise SystemExit(f"unknown inject label {label}")
        _, _, text = INJECT_BY_LABEL[label]
        chosen.append((index, label, text))
    return chosen


def injections_toml(rows: list[tuple[int, str, str]] | None = None) -> str:
    if rows is None:
        rows = list(INJECTIONS)
    chunks = []
    for turn, label, text in rows:
        escaped = text.replace("'''", "\\'\\'\\'")
        chunks.append(
            f"[[injection]]\nturn = {turn}\nlabel = {label!r}\ntext = '''{escaped}'''\n"
        )
    return "\n".join(chunks)


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def link_arm(link: Path, target: Path) -> None:
    if link.exists() or link.is_symlink():
        return
    try:
        link.symlink_to(target, target_is_directory=True)
        return
    except OSError:
        pass
    subprocess.run(
        ["cmd", "/c", "mklink", "/J", str(link), str(target)],
        check=False,
        capture_output=True,
    )
    if not link.exists():
        link.write_text(str(target) + "\n", encoding="utf-8")


def load_studies() -> dict[str, str]:
    studies = {}
    for name, path in STUDIES.items():
        if not path.is_file():
            raise SystemExit(f"missing study {path}")
        studies[name] = study_prose(path.read_text(encoding="utf-8"))
        if not studies[name]:
            raise SystemExit(f"empty study {path}")
    return studies


def _copy_inject(src: Path, dest: Path) -> list[tuple[int, str, str]]:
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(src.read_text(encoding="utf-8"), encoding="utf-8")
    table = tomllib.loads(dest.read_text(encoding="utf-8"))
    rows: list[tuple[int, str, str]] = []
    for row in table.get("injection") or []:
        rows.append((int(row["turn"]), str(row.get("label") or ""), str(row["text"])))
    if not rows:
        raise SystemExit(f"no [[injection]] rows in {src}")
    return rows


def prepare(stamp: str, mode: str = "none", turns: int = TURNS) -> dict[str, Any]:
    if not PRODUCTION.is_file():
        raise SystemExit(f"missing production scenario {PRODUCTION}")
    if turns < 1:
        raise SystemExit("turns must be >= 1")
    studies = load_studies()
    production = json.loads(PRODUCTION.read_text(encoding="utf-8"))
    old, new = build_scenarios(production, studies, mode)

    fixture = ROOT / "checkpoints" / f"narrator-ab-{stamp}"
    fixture.mkdir(parents=True, exist_ok=False)
    old_path = fixture / "old" / "konosuba.json"
    new_path = fixture / "new" / "konosuba.json"
    write_json(old_path, old)
    write_json(new_path, new)

    runs = PIPE / "runs"
    paths = {
        "fixture": fixture,
        "old_scenario": old_path,
        "new_scenario": new_path,
        "old_out": runs / f"study-ab-old-{mode}-{stamp}",
        "new_out": runs / f"study-ab-new-{mode}-{stamp}",
        "mode": mode,
        "turns": turns,
        "old_prompt_words": prompt_words(str(old.get("system_prompt") or "")),
        "new_prompt_words": prompt_words(str(new.get("system_prompt") or "")),
    }
    write_json(
        fixture / "manifest.json",
        {
            "experiment": "study-ab-session-eval",
            "stamp": stamp,
            "mode": mode,
            "injects": [],
            "turns": turns,
            "player_played": True,
            "old_prompt_words": paths["old_prompt_words"],
            "new_prompt_words": paths["new_prompt_words"],
            "production": str(PRODUCTION),
            "studies": {name: str(path) for name, path in STUDIES.items()},
            "old_scenario": str(old_path),
            "new_scenario": str(new_path),
            "old_out": str(paths["old_out"]),
            "new_out": str(paths["new_out"]),
            "query_character_core": {"old": False, "new": True},
            "narrator_max_rounds": 24,
        },
    )
    return paths


def _arm_env(scenario: Path, *, query_character_core: bool) -> dict[str, str]:
    env = os.environ.copy()
    env["RHAPSODE_SCENARIO"] = str(scenario.resolve())
    env["RHAPSODE_NARRATOR_MAX_ROUNDS"] = "24"
    env["RHAPSODE_QUERY_CHARACTER_CORE"] = "1" if query_character_core else "0"
    env["NO_PROXY"] = "127.0.0.1,localhost"
    env["no_proxy"] = "127.0.0.1,localhost"
    env["PYTHONUNBUFFERED"] = "1"
    for key in ("RHAPSODE_SAVES_DIR", "RHAPSODE_CHROMA_DIR"):
        env.pop(key, None)
    return env


def _pump(name: str, proc: subprocess.Popen[str], log_path: Path) -> None:
    assert proc.stdout is not None
    with log_path.open("w", encoding="utf-8") as log:
        for line in proc.stdout:
            log.write(line)
            log.flush()
            print(f"[{name}] {line}", end="", flush=True)


def launch_arm(
    name: str,
    scenario: Path,
    out_dir: Path,
    log_path: Path,
    turns: int,
    seed_saves: Path | None = None,
) -> subprocess.Popen[str]:
    if not SERVER_PY.is_file():
        raise SystemExit(f"missing server venv python {SERVER_PY}")
    if not RUN_PY.is_file():
        raise SystemExit(f"missing {RUN_PY}")
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = [
        str(SERVER_PY),
        str(RUN_PY),
        "--turns",
        str(turns),
        "--port",
        "8080",
        "--out-dir",
        str(out_dir.resolve()),
    ]
    if seed_saves is not None:
        cmd.extend(["--seed-saves", str(seed_saves.resolve())])
    proc = subprocess.Popen(
        cmd,
        cwd=str(PIPE),
        env=_arm_env(scenario, query_character_core=(name == "new")),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    threading.Thread(target=_pump, args=(name, proc, log_path), daemon=True).start()
    return proc


def count_injects(out_dir: Path) -> int:
    log = out_dir / "injections.log"
    if not log.is_file():
        return 0
    return sum(1 for line in log.read_text(encoding="utf-8").splitlines() if "INJECT " in line)


def player_decided(out_dir: Path) -> bool:
    profile = out_dir / "llm_profile.jsonl"
    if profile.is_file():
        for line in profile.read_text(encoding="utf-8", errors="replace").splitlines():
            if '"stage": "player"' in line or '"stage":"player"' in line:
                return True
    log = out_dir / "console.log"
    if log.is_file():
        return "[player] decide" in log.read_text(encoding="utf-8", errors="replace")
    return False


def end_reason(out_dir: Path) -> str:
    manifest = out_dir / "manifest.json"
    if not manifest.is_file():
        return ""
    data = json.loads(manifest.read_text(encoding="utf-8"))
    return str(data.get("end_reason") or "")


def turns_completed(out_dir: Path) -> int:
    report = out_dir / "report.json"
    if report.is_file():
        data = json.loads(report.read_text(encoding="utf-8"))
        return int(data.get("turns_completed") or 0)
    manifest = out_dir / "manifest.json"
    if manifest.is_file():
        data = json.loads(manifest.read_text(encoding="utf-8"))
        return int(data.get("turns_completed") or 0)
    return 0


def validate_arm(name: str, out_dir: Path, expected: int) -> list[str]:
    errors: list[str] = []
    reason = end_reason(out_dir)
    if reason != "MaxTurns":
        errors.append(f"{name} end_reason={reason or 'missing'}")
    done = turns_completed(out_dir)
    if done != expected:
        errors.append(f"{name} turns={done} expected={expected}")
    injected = count_injects(out_dir)
    if injected != 0:
        errors.append(f"{name} injects={injected} expected=0")
    if not player_decided(out_dir):
        errors.append(f"{name} player did not play")
    return errors


def parse_perceptions(console: str) -> dict[str, list[tuple[int, str]]]:
    found: dict[str, list[tuple[int, str]]] = {name: [] for name in CAST}
    for line in console.splitlines():
        match = PERCEPTION_RE.search(line)
        if not match:
            continue
        name, turn, text = match.group(1), int(match.group(2)), match.group(3) or ""
        found[name].append((turn, text.strip()))
    return found


def load_world(out_dir: Path) -> dict[str, Any]:
    for rel in ("live/saves/world.json", "saves/world.json"):
        path = out_dir / rel
        if path.is_file():
            return json.loads(path.read_text(encoding="utf-8"))
    return {}


def public_beats(turns_path: Path) -> list[dict[str, Any]]:
    beats: list[dict[str, Any]] = []
    if not turns_path.is_file():
        return beats
    for line in turns_path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        prose: list[str] = []
        speech: list[tuple[str, str]] = []
        for msg in row.get("messages") or []:
            if msg.get("type") != "scene_message":
                continue
            content = str(msg.get("content") or "").strip()
            if not content:
                continue
            speaker = str(msg.get("speaker") or "").strip()
            if speaker and speaker.lower() != "narrator":
                speech.append((speaker, content))
            else:
                prose.append(content)
        beats.append(
            {
                "turn": row.get("turn"),
                "input": row.get("input", ""),
                "prose": prose,
                "speech": speech,
            }
        )
    return beats


def render_arm(title: str, out_dir: Path) -> str:
    chunks = [f"## {title}\n"]
    chunks.append(f"out: `{out_dir}`")
    chunks.append(f"end_reason: {end_reason(out_dir) or 'missing'}")
    chunks.append(f"scripted Kazuma lines: {count_injects(out_dir)}")
    console = ""
    console_path = out_dir / "console.log"
    if console_path.is_file():
        console = console_path.read_text(encoding="utf-8", errors="replace")
    perceptions = parse_perceptions(console)
    world = load_world(out_dir)
    memories = world.get("character_memories") or {}
    for beat in public_beats(out_dir / "turns.jsonl"):
        chunks.append(f"\n### Turn {beat['turn']}\n")
        chunks.append(f"**Kazuma.** {beat['input']}\n")
        if not beat["prose"] and not beat["speech"]:
            chunks.append("The room did not answer.\n")
        for prose in beat["prose"]:
            chunks.append(prose + "\n")
        for speaker, line in beat["speech"]:
            chunks.append(f"**{speaker}.** {line}\n")
        for name in CAST:
            seen = [text for turn, text in perceptions[name] if turn == beat["turn"]]
            if seen:
                chunks.append(f"*{name}, in her head.* {seen[-1]}\n")
    chunks.append("\n### Later thoughts (not spoken)\n")
    for name in CAST:
        rows = (memories.get(name) or {}).get("monologue") or []
        if not rows:
            chunks.append(f"*{name}:* (none)\n")
            continue
        for row in rows:
            chunks.append(f"*{name} after turn {int(row.get('turn', 0)) + 1}:* {row.get('text', '')}\n")
        final = str((memories.get(name) or {}).get("perception") or "").strip()
        if final:
            chunks.append(f"*{name} last private look:* {final}\n")
    return "\n".join(chunks).rstrip() + "\n"


def write_reading(fixture: Path, old_out: Path, new_out: Path) -> Path:
    parts = [
        "# Pair extract\n",
        "Left folder is the game's current Darkness and Megumin text. "
        "Right folder is those two texts swapped for the pages you wrote.\n",
        render_arm("Old character text", old_out),
        render_arm("Your new writeups", new_out),
    ]
    path = fixture / "reading.md"
    path.write_text("\n".join(parts), encoding="utf-8")
    return path


def summarize_arm(title: str, out_dir: Path) -> str:
    lines = [f"## {title}", ""]
    beats = public_beats(out_dir / "turns.jsonl")
    if not beats:
        lines.append("No turns were written.")
        return "\n".join(lines) + "\n"
    for beat in beats:
        speakers = [who for who, _ in beat["speech"]]
        lines.append(f"Turn {beat['turn']}. Kazuma: {beat['input']}")
        if not beat["prose"] and not speakers:
            lines.append("Nobody answered.")
        else:
            if beat["prose"]:
                lines.append("The room described something.")
            if speakers:
                lines.append("Who spoke: " + ", ".join(speakers) + ".")
            else:
                lines.append("Nobody spoke. There was only description.")
        lines.append("")
    return "\n".join(lines)


def write_plain(fixture: Path, paths: dict[str, Any]) -> Path:
    mode = paths["mode"]
    how = {
        "none": "The narrator got only the short Konosuba line. No extra pages.",
        "short": "The narrator got two short scene notes, the same on both sides.",
        "full": "The narrator got the full character pages pasted under the Konosuba line.",
    }[mode]
    text = (
        "# What this pair was\n\n"
        f"{how}\n\n"
        f"Kazuma was played by the player model for {paths['turns']} turns. "
        "No scripted Kazuma sentences.\n\n"
        f"Narrator prompt size: old {paths['old_prompt_words']} words, "
        f"new {paths['new_prompt_words']} words.\n\n"
        + summarize_arm("Old character text", paths["old_out"])
        + "\n"
        + summarize_arm("Your new writeups", paths["new_out"])
    )
    path = fixture / "plain.md"
    path.write_text(text, encoding="utf-8")
    return path


def launch_pair(paths: dict[str, Any]) -> int:
    fixture = paths["fixture"]
    turns = int(paths["turns"])
    old = launch_arm(
        "old",
        paths["old_scenario"],
        paths["old_out"],
        fixture / "old.launch.log",
        turns,
        paths.get("seed_old"),
    )
    new = launch_arm(
        "new",
        paths["new_scenario"],
        paths["new_out"],
        fixture / "new.launch.log",
        turns,
        paths.get("seed_new"),
    )
    old_rc = old.wait()
    new_rc = new.wait()
    link_arm(fixture / "old-run", paths["old_out"])
    link_arm(fixture / "new-run", paths["new_out"])
    errors = []
    if old_rc != 0:
        errors.append(f"old exit {old_rc}")
    if new_rc != 0:
        errors.append(f"new exit {new_rc}")
    errors.extend(validate_arm("old", paths["old_out"], turns))
    errors.extend(validate_arm("new", paths["new_out"], turns))
    reading = write_reading(fixture, paths["old_out"], paths["new_out"])
    plain = write_plain(fixture, paths)
    print(f"reading -> {reading}")
    print(f"plain -> {plain}")
    if errors:
        print("FAILED: " + "; ".join(errors), file=sys.stderr)
        return 1
    print(f"ok: both sides MaxTurns, {turns} turns, player played, no scripted Kazuma")
    return 0


def parse_labels(raw: str) -> list[str]:
    raw = raw.strip()
    if raw in ("", "all"):
        return [row[1] for row in INJECTIONS]
    return [part.strip() for part in raw.split(",") if part.strip()]


def main() -> int:
    parser = argparse.ArgumentParser(description="Study A/B through session eval")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--stamp", default="")
    parser.add_argument(
        "--mode",
        choices=MODES,
        default="none",
        help="none: do not paste extra text onto the narrator. "
        "short: paste two tiny scene notes, same on both sides. "
        "full: paste the whole character pages (the first pair).",
    )
    parser.add_argument(
        "--turns",
        type=int,
        default=TURNS,
        help="Turns for each side. Default is 5. Kazuma is played, not scripted.",
    )
    parser.add_argument(
        "--seed-old",
        default="",
        help="Optional previous live/saves dir for the old side (continuation).",
    )
    parser.add_argument(
        "--seed-new",
        default="",
        help="Optional previous live/saves dir for the new side (continuation).",
    )
    args = parser.parse_args()
    stamp = args.stamp or datetime.now().strftime("%Y%m%d-%H%M%S")
    paths = prepare(stamp, args.mode, args.turns)
    if args.seed_old:
        paths["seed_old"] = Path(args.seed_old)
    if args.seed_new:
        paths["seed_new"] = Path(args.seed_new)
    print(f"fixture -> {paths['fixture']}")
    print(f"mode -> {paths['mode']}")
    print(f"turns -> {paths['turns']}")
    print("Kazuma -> player model (no scripted sentences)")
    print(f"old scenario -> {paths['old_scenario']}")
    print(f"new scenario -> {paths['new_scenario']}")
    print(f"old out -> {paths['old_out']}")
    print(f"new out -> {paths['new_out']}")
    print(f"prompt words old/new -> {paths['old_prompt_words']}/{paths['new_prompt_words']}")
    if args.dry_run:
        return 0
    return launch_pair(paths)


if __name__ == "__main__":
    sys.exit(main())
