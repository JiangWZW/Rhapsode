import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from rhapsode.llm_tools import (
    NARRATOR_TOOLS,
    QUERY_CHARACTER_CORE_TOOL,
    beat_narrator_tools,
    narrator_max_rounds,
)


def test_shared_list_has_no_core_tool():
    assert all(tool["name"] != "query_character_core" for tool in NARRATOR_TOOLS)


def test_beat_tools_on_by_default(monkeypatch):
    monkeypatch.delenv("RHAPSODE_QUERY_CHARACTER_CORE", raising=False)
    names = [tool["name"] for tool in beat_narrator_tools()]
    assert names[-1] == "query_character_core"
    assert "query_mind" in names


def test_beat_tools_omitted_when_off(monkeypatch):
    monkeypatch.setenv("RHAPSODE_QUERY_CHARACTER_CORE", "0")
    names = [tool["name"] for tool in beat_narrator_tools()]
    assert "query_character_core" not in names


def test_beat_tools_on_when_enabled(monkeypatch):
    monkeypatch.setenv("RHAPSODE_QUERY_CHARACTER_CORE", "1")
    names = [tool["name"] for tool in beat_narrator_tools()]
    assert names.count("query_character_core") == 1


def test_mind_schema_does_not_name_core_tool():
    mind = next(tool for tool in NARRATOR_TOOLS if tool["name"] == "query_mind")
    assert "query_character_core" not in mind["description"]
    assert "Who you are page" in mind["description"]
    assert "perception" in mind["description"]
    assert "monologue" in mind["description"]


def test_core_schema_asks_for_a_call():
    text = QUERY_CHARACTER_CORE_TOOL["description"]
    assert "query_mind" in text
    assert "Call this before you write" in text
    assert "Do not call it every turn" not in text
    assert "Who you are page" in text


def test_narrator_max_rounds_default(monkeypatch):
    monkeypatch.delenv("RHAPSODE_NARRATOR_MAX_ROUNDS", raising=False)
    assert narrator_max_rounds() == 24


def test_narrator_max_rounds_env(monkeypatch):
    monkeypatch.setenv("RHAPSODE_NARRATOR_MAX_ROUNDS", "32")
    assert narrator_max_rounds() == 32
