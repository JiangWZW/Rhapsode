"""Player LLM for the session eval pipeline.

Loads the on-disk save. C++ supplies situation (cast + board). The same
narrator read tools (query_graph / query_mind / query_history / list_scenes)
run through Story.dispatch_tool. Returns one short player action line.
"""
from __future__ import annotations

import json
import logging
import os
import re
from collections.abc import Callable
from pathlib import Path

from rhapsode._core import Story
from rhapsode.llm import complete_with_tools
from rhapsode.llm_tools import NARRATOR_TOOLS

log = logging.getLogger("rhapsode.player")


def _player_model() -> str:
    """Resolve at call time so dotenv is already loaded; never fall back to pro."""
    return (
        os.environ.get("RHAPSODE_PLAYER_MODEL")
        or os.environ.get("RHAPSODE_MODEL")
        or "deepseek-v4-flash"
    )

_META_RE = re.compile(
    r"(chatgpt|knowledge cutoff|as an ai|i will adhere|"
    r"how can i assist|json format|system prompt|role-playing as)",
    re.IGNORECASE,
)


def _player_description(story: Story) -> str:
    for character in story.world().characters:
        if character.is_player and character.description.strip():
            return character.description.strip()
    return ""


def _build_system(protocol: str, story: Story, guide_text: str) -> str:
    parts = [protocol.strip(), story.player_situation()]
    persona = _player_description(story)
    if persona:
        parts.append("### You\n" + persona)
    brief = guide_text.strip()
    if brief:
        parts.append("### Brief\n" + brief)
    return "\n\n".join(p for p in parts if p)


def _is_bad_action(action: str, on_stage: list[str] | None = None) -> bool:
    text = action.strip()
    if not text:
        return True
    if text[0] in "{[":
        return True
    if _META_RE.search(text):
        return True
    if text.lower().startswith("understood"):
        return True
    # Must be the player acting — not narrating another character.
    if not re.search(r"\b(I|I'll|I'm|I've|I'd|me|my|myself)\b", text):
        return True
    first = text.split("\n", 1)[0].strip()
    for name in on_stage or []:
        # Reject clear NPC ventriloquism ("Luna smiles…"), not observations
        # like "Luna's waving me over…".
        if re.match(
            rf"^{re.escape(name)}\s+(smiles|says|taps|folds|slides|watches|"
            rf"looks|nods|sighs|asks|replies)\b",
            first,
            re.IGNORECASE,
        ):
            return True
    return False


def make_player_llm(
    saves_dir: str | Path,
    scenario_path: str | Path,
    *,
    protocol: str,
    guide_text: str = "",
    empty_action: str = "I look around carefully.",
) -> Callable[[str], str]:
    saves_dir = str(saves_dir)
    scenario_path = str(scenario_path)
    guide_text = guide_text or ""
    empty_action = empty_action or "I look around carefully."
    last_action = {"text": ""}

    def player(prompt: str) -> str:
        story = Story.load_scenario(scenario_path)
        if story.has_save(saves_dir):
            story.load_save(saves_dir)
        scene_id = story.active_scene_id or ""

        user_prompt = prompt
        if last_action["text"]:
            user_prompt += (
                "\n\n### Your previous action (do NOT repeat or reverse it)\n"
                + last_action["text"]
            )

        on_stage_names = [
            ch.name
            for ch in story.world().characters
            if (not ch.is_player)
            and (not ch.dead)
            and scene_id
            and ch.in_scene(scene_id)
        ]

        model = _player_model()

        def dispatch(name: str, args: dict) -> str:
            return story.dispatch_tool(
                scene_id, name, json.dumps(args or {}))

        def once(extra: str = "") -> str:
            system = _build_system(protocol, story, guide_text)
            if extra:
                system += "\n\n" + extra
            messages = [
                {"role": "system", "parts": [{"text": system}]},
                {"role": "user", "parts": [{"text": user_prompt}]},
            ]
            log.info("[player] decide model=%s thinking=False tools=read", model)
            return complete_with_tools(
                messages, NARRATOR_TOOLS, dispatch,
                model=model, thinking=False, stage="player",
                phase="decide",
            ).strip()

        action = once()
        if _is_bad_action(action, on_stage_names):
            log.warning("[player] rejecting bad action: %r", action[:200])
            action = once(
                "RETRY: reply as YOUR player character only, first person. "
                "Do NOT write other characters' dialogue or stage directions."
            )
        if _is_bad_action(action, on_stage_names):
            log.warning("[player] fallback empty_action after second failure")
            action = empty_action

        last_action["text"] = action
        return action

    return player
