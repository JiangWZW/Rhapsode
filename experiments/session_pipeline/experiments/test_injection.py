"""Self-test for injection.py — no server, no LLM, no rhapsode imports.

Run:  ..\\..\\server\\.venv\\Scripts\\python.exe experiments\\test_injection.py
(from experiments\\session_pipeline)
"""
from __future__ import annotations

import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from injection import load_injections, seed_saves, wrap_player  # noqa: E402


def test_e1_spec_loads() -> None:
    table = load_injections(HERE / "e1_perturbation.toml")
    assert sorted(table) == [1, 50, 100], sorted(table)
    assert table[1].label == "plot_probe"
    assert table[50].label == "commitment_probe"
    assert table[100].label == "style_control"
    assert all(inj.text for inj in table.values())
    assert "Gerhardt" in table[1].text and table[1].text.endswith('?"')


def test_wrapper_fires_on_right_turns(tmp: Path) -> None:
    table = load_injections(HERE / "e1_perturbation.toml")
    calls: list[tuple[int, str]] = []
    n = {"i": 0}

    def stub(prompt: str) -> str:
        n["i"] += 1
        calls.append((n["i"], prompt))
        return f"stub-action-{n['i']}"

    log = tmp / "injections.log"
    wrapped = wrap_player(stub, table, log_path=log)

    total = 120
    results = [wrapped(f"prompt-{turn}") for turn in range(1, total + 1)]

    # Injection fires at exactly turns 1/50/100 with the scripted text.
    assert results[0] == table[1].text
    assert results[49] == table[50].text
    assert results[99] == table[100].text
    # Stub is called on every other turn, in order, with the right prompt.
    injected = {1, 50, 100}
    expected_stub_turns = [t for t in range(1, total + 1) if t not in injected]
    assert len(calls) == len(expected_stub_turns), (len(calls), len(expected_stub_turns))
    for (i, prompt), turn in zip(calls, expected_stub_turns):
        assert prompt == f"prompt-{turn}", (prompt, turn)
    for idx, turn in enumerate(expected_stub_turns):
        assert results[turn - 1] == f"stub-action-{idx + 1}"
    # Log file written, one INJECT line per fired injection.
    text = log.read_text(encoding="utf-8")
    inject_lines = [ln for ln in text.splitlines() if "INJECT " in ln]
    assert len(inject_lines) == 3, text
    assert "turn=1 " in inject_lines[0] and "plot_probe" in inject_lines[0]
    assert "turn=50 " in inject_lines[1] and "commitment_probe" in inject_lines[1]
    assert "turn=100 " in inject_lines[2] and "style_control" in inject_lines[2]


def test_seed_saves_backs_up(tmp: Path) -> None:
    src = tmp / "src_saves"
    src.mkdir(parents=True)
    for name in ("story.json", "world.json", "konosuba.json"):
        (src / name).write_text(f"NEW {name}", encoding="utf-8")
    dest = tmp / "saves"
    dest.mkdir()
    (dest / "story.json").write_text("OLD", encoding="utf-8")

    log = tmp / "seed.log"
    backup = seed_saves(src, dest, log_path=log)

    assert backup is not None and backup.is_dir()
    assert backup.name.startswith("saves_backup_")
    assert (backup / "story.json").read_text(encoding="utf-8") == "OLD"
    assert (dest / "story.json").read_text(encoding="utf-8") == "NEW story.json"
    assert sorted(p.name for p in dest.iterdir()) == [
        "konosuba.json", "story.json", "world.json"]
    # Source untouched.
    assert sorted(p.name for p in src.iterdir()) == [
        "konosuba.json", "story.json", "world.json"]
    assert (src / "story.json").read_text(encoding="utf-8") == "NEW story.json"
    assert "SEED moved existing saves" in log.read_text(encoding="utf-8")

    # Empty/absent dest: no backup created.
    dest2 = tmp / "saves2"
    assert seed_saves(src, dest2, log_path=log) is None
    assert (dest2 / "world.json").is_file()


def main() -> int:
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        test_e1_spec_loads()
        print("ok: e1_perturbation.toml parses, 3 probes at turns 1/50/100")
        test_wrapper_fires_on_right_turns(tmp / "wrap")
        print("ok: wrapper injects at 1/50/100, stub on the other 117 turns, log written")
        test_seed_saves_backs_up(tmp / "seed")
        print("ok: seed_saves backs up non-empty dest, copies 3 files, source untouched")
    print("ALL PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
