"""Tool-using player LLM for the session eval pipeline.

Loads the on-disk save, runs the same read tools as the narrator
(query_graph / query_mind / query_history / list_scenes), then returns
one short player action line.
"""
from __future__ import annotations

import json
import logging
import re
from collections.abc import Callable
from pathlib import Path

from rhapsode._core import Story
from rhapsode.llm import complete_with_tools
from rhapsode.llm_tools import NARRATOR_TOOLS

log = logging.getLogger("rhapsode.player")

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


def _situation_block(story: Story, scene_id: str) -> str:
    """Force-feed live cast + storylines so the model cannot invent presence."""
    lines: list[str] = ["### Situation (authoritative — obey this)"]
    lines.append(f"Active scene: {scene_id or '(none)'}")

    on_stage: list[str] = []
    off_stage: list[str] = []
    for ch in story.world().characters:
        if ch.is_player or ch.dead:
            continue
        if scene_id and ch.in_scene(scene_id):
            on_stage.append(ch.name)
        else:
            off_stage.append(ch.name)
    lines.append(
        "On-stage with you now (only these NPCs can be addressed): "
        + (", ".join(on_stage) if on_stage else "(none — you are alone)")
    )
    if off_stage:
        lines.append("Elsewhere (NOT here): " + ", ".join(off_stage))

    try:
        raw = story.dispatch_tool(scene_id, "list_scenes", "{}")
        payload = json.loads(raw) if raw else []
        if isinstance(payload, list):
            scenes = payload
        elif isinstance(payload, dict):
            scenes = payload.get("scenes") or payload.get("storylines") or payload
        else:
            scenes = payload
        if not isinstance(scenes, list):
            scenes = []
        lines.append("list_scenes → " + json.dumps(scenes, ensure_ascii=False)[:1200])

        off_scenes = [
            row
            for row in scenes
            if isinstance(row, dict)
            and row.get("scene_id")
            and row.get("scene_id") != scene_id
            and not row.get("player_present")
        ]
        if off_scenes:
            lines.append("### PRIORITY THIS TURN (hard rule)")
            lines.append(
                "An off-stage storyline is live. Leave your current conversation "
                "and travel to that cast THIS turn. Arrive and greet them by name "
                "when you share their place. Do NOT continue desk/debt/quest chat, "
                "shopping, or new side plots until you stand with them."
            )
            for row in off_scenes:
                cast = row.get("cast") or []
                cast_s = ", ".join(cast) if isinstance(cast, list) else str(cast)
                intent = str(row.get("driving_intention") or "").strip()
                last = str(row.get("last_narration") or "").strip().replace("\n", " ")
                if len(last) > 180:
                    last = last[:177] + "..."
                lines.append(
                    f"- target scene_id={row.get('scene_id')} cast=[{cast_s}] "
                    f"intention={intent or '(none)'} last={last or '(none)'}"
                )
    except Exception as exc:  # noqa: BLE001 — best-effort grounding
        log.warning("[player] list_scenes failed: %s", exc)

    return "\n".join(lines)


def _build_system(protocol: str, story: Story, guide_text: str, situation: str) -> str:
    parts = [protocol.strip(), situation]
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
        situation = _situation_block(story, scene_id)

        def dispatch(name: str, args: dict) -> str:
            log.info(
                "[player] tool: %s %s",
                name,
                json.dumps(args or {}, ensure_ascii=False),
            )
            return story.dispatch_tool(
                scene_id, name, json.dumps(args or {}, ensure_ascii=False)
            )

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

        def once(extra: str = "") -> str:
            system = _build_system(protocol, story, guide_text, situation)
            if extra:
                system += "\n\n" + extra
            messages = [
                {"role": "system", "parts": [{"text": system}]},
                {"role": "user", "parts": [{"text": user_prompt}]},
            ]
            return complete_with_tools(messages, NARRATOR_TOOLS, dispatch).strip()

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
