"""Fixture and extract checks for narrator_experiment. No server, no LLM."""

from __future__ import annotations

import json
from pathlib import Path

from narrator_experiment import (
    CAST,
    INJECTIONS,
    PERCEPTION_RE,
    SHORT_NOTES,
    TURNS,
    _arm_env,
    _copy_inject,
    apply_references,
    build_scenarios,
    injections_toml,
    parse_labels,
    parse_perceptions,
    select_injections,
    study_prose,
    validate_arm,
)


def test_study_prose_drops_title():
    assert study_prose("# Megumin\n\nShe casts.\n") == "She casts."
    assert study_prose("Already prose.") == "Already prose."


def test_build_scenarios_none_leaves_narrator_alone():
    production = {
        "system_prompt": "Narrate.",
        "characters": [
            {"name": "Darkness", "core": "compact D"},
            {"name": "Megumin", "core": "compact M"},
        ],
    }
    studies = {"Darkness": "full D", "Megumin": "full M"}
    old, new = build_scenarios(production, studies, "none")
    assert [c["core"] for c in old["characters"]] == ["compact D", "compact M"]
    assert [c["core"] for c in new["characters"]] == ["full D", "full M"]
    assert old["system_prompt"] == "Narrate."
    assert new["system_prompt"] == "Narrate."
    assert CAST == ("Darkness", "Megumin")


def test_build_scenarios_short_is_same_on_both_sides():
    production = {
        "system_prompt": "Narrate.",
        "characters": [
            {"name": "Darkness", "core": "compact D"},
            {"name": "Megumin", "core": "compact M"},
        ],
    }
    studies = {"Darkness": "full D", "Megumin": "full M"}
    old, new = build_scenarios(production, studies, "short")
    assert old["system_prompt"] == new["system_prompt"]
    assert "this scene only" in old["system_prompt"]
    assert SHORT_NOTES["Darkness"] in old["system_prompt"]
    assert "full D" not in old["system_prompt"]
    assert "compact D" not in old["system_prompt"]


def test_build_scenarios_full_pastes_pages():
    production = {
        "system_prompt": "Narrate.",
        "characters": [
            {"name": "Darkness", "core": "compact D"},
            {"name": "Megumin", "core": "compact M"},
        ],
    }
    studies = {"Darkness": "full D", "Megumin": "full M"}
    old, new = build_scenarios(production, studies, "full")
    assert "compact D" in old["system_prompt"]
    assert "full D" in new["system_prompt"]
    assert "compact D" not in new["system_prompt"]


def test_identical_wrappers():
    refs = {"Darkness": "BODY", "Megumin": "BODY"}
    text = apply_references("Style.", refs)
    assert text.startswith("Style.\n\nDarkness reference")
    assert "temporary test only" in text


def test_injections_five_labeled_turns():
    text = injections_toml()
    assert text.count("[[injection]]") == 5
    assert [row[0] for row in INJECTIONS] == [1, 2, 3, 4, 5]
    assert [row[1] for row in INJECTIONS] == [
        "board",
        "vote",
        "pocket",
        "coming",
        "formation",
    ]
    assert "aren't you telling" in text
    assert "Darkness" in INJECTIONS[3][2]
    assert "Darkness" in INJECTIONS[4][2]
    assert "let her finish" not in INJECTIONS[3][2]


def test_copy_inject_preserves_wording():
    import tempfile

    src = Path(tempfile.mkdtemp()) / "injections.toml"
    src.write_text(
        "[[injection]]\nturn = 1\nlabel = 'coming'\n"
        "text = '''I leave Luna's nets. \"Darkness, are you coming?\"'''\n",
        encoding="utf-8",
    )
    dest = src.parent / "copy.toml"
    rows = _copy_inject(src, dest)
    assert rows == [(1, "coming", 'I leave Luna\'s nets. "Darkness, are you coming?"')]


def test_select_two_injects_renumbers():
    rows = select_injections(["board", "vote"])
    assert [row[0] for row in rows] == [1, 2]
    assert [row[1] for row in rows] == ["board", "vote"]
    assert parse_labels("board,vote") == ["board", "vote"]
    assert parse_labels("all") == [row[1] for row in INJECTIONS]


def test_perception_parse():
    log = (
        "12:00:00 INFO  perception: Darkness t=1 The pocket.\n"
        "12:00:01 INFO  perception: Megumin t=1 (empty)\n"
        "12:00:02 INFO  perception: Darkness t=2 A vote.\n"
    )
    found = parse_perceptions(log)
    assert found["Darkness"] == [(1, "The pocket."), (2, "A vote.")]
    assert found["Megumin"] == [(1, "")]
    assert PERCEPTION_RE.search("junk") is None


def test_turns_default_is_five():
    assert TURNS == 5


def test_validate_arm_wants_player_and_no_script():
    import tempfile

    root = Path(tempfile.mkdtemp())
    (root / "manifest.json").write_text(
        '{"end_reason":"MaxTurns","turns_completed":5}\n', encoding="utf-8"
    )
    (root / "report.json").write_text(
        '{"end_reason":"MaxTurns","turns_completed":5}\n', encoding="utf-8"
    )
    (root / "console.log").write_text(
        "16:00:00 INFO  [player] decide model=x thinking=False tools=read\n",
        encoding="utf-8",
    )
    assert validate_arm("old", root, 5) == []
    (root / "injections.log").write_text("INJECT turn=1 label='board'\n", encoding="utf-8")
    assert any("injects=" in err for err in validate_arm("old", root, 5))


def test_arm_env_gates_core_tool():
    old = _arm_env(Path("old.json"), query_character_core=False)
    new = _arm_env(Path("new.json"), query_character_core=True)
    assert old["RHAPSODE_QUERY_CHARACTER_CORE"] == "0"
    assert new["RHAPSODE_QUERY_CHARACTER_CORE"] == "1"
    assert old["RHAPSODE_NARRATOR_MAX_ROUNDS"] == "24"
    assert new["RHAPSODE_NARRATOR_MAX_ROUNDS"] == "24"
    assert old["RHAPSODE_SCENARIO"].endswith("old.json")
    assert new["RHAPSODE_SCENARIO"].endswith("new.json")


if __name__ == "__main__":
    test_study_prose_drops_title()
    test_build_scenarios_none_leaves_narrator_alone()
    test_build_scenarios_short_is_same_on_both_sides()
    test_build_scenarios_full_pastes_pages()
    test_identical_wrappers()
    test_injections_five_labeled_turns()
    test_copy_inject_preserves_wording()
    test_select_two_injects_renumbers()
    test_perception_parse()
    test_turns_default_is_five()
    test_validate_arm_wants_player_and_no_script()
    test_arm_env_gates_core_tool()
    print("ok")
    json.dumps({"ok": True})
