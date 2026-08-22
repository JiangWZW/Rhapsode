"""Narrator tool schemas and the callback that runs one tool-use loop through
the engine's call-scoped read function."""

import json
import logging
import os

from rhapsode.llm import complete, complete_with_tools
from rhapsode.llm_profile import infer_narrator_phase

log = logging.getLogger(__name__)


def _base_model() -> str | None:
    return os.environ.get("RHAPSODE_MODEL")


def _pro_model() -> str | None:
    return os.environ.get("RHAPSODE_NARRATOR_MODEL") or _base_model()


def call_llm(
    prompt: str,
    *,
    stage: str = "llm",
    model: str | None = None,
    thinking: bool | None = None,
    phase: str = "",
) -> str:
    return complete(
        [{"role": "user", "parts": [{"text": prompt}]}],
        model=model,
        thinking=thinking,
        stage=stage,
        phase=phase,
    )


def make_llm_callback(
    stage: str,
    *,
    model: str | None = None,
    thinking: bool | None = None,
):
    """Plain completion callback. model/thinking resolve at call time if None
    is passed for model — callers should pass explicit thinking for non-narrator
    stages so they do not inherit RHAPSODE_DEEPSEEK_THINKING."""
    def _call(prompt: str) -> str:
        return call_llm(
            prompt,
            stage=stage,
            model=model if model is not None else _base_model(),
            thinking=thinking,
        )
    return _call


def make_weaver_callback():
    """Weave + expiry: base (flash) model, thinking off."""
    def _call(prompt: str) -> str:
        head = prompt[:600].lower()
        stage = "expiry" if ("expir" in head or "valid_until" in head) else "weave"
        return call_llm(
            prompt, stage=stage, model=_base_model(), thinking=False)
    return _call


MONOLOGUE_USER_SENTINEL = "<<<RHAPSODE_MONOLOGUE_USER>>>"


def split_monologue_prompt(prompt: str) -> tuple[str | None, str]:
    """Split the native concatenated monologue blob into system + user.

    C++ still hands Python one string (LLMCallback). The sentinel marks where
    the shared craft/schema ends and the inhabitation sheet begins. Missing
    sentinel (old _core.pyd) keeps the whole blob as the user message.
    """
    idx = prompt.find(MONOLOGUE_USER_SENTINEL)
    if idx < 0:
        return None, prompt
    system = prompt[:idx]
    if system.endswith("\n"):
        system = system[:-1]
    user = prompt[idx + len(MONOLOGUE_USER_SENTINEL):]
    if user.startswith("\n"):
        user = user[1:]
    return system, user


def make_reflection_callback():
    """On-stage monologue updater: narrator/pro model, thinking on.

    No chat history. Each take is system (craft+schema) plus one user sheet.
    """
    def _call(prompt: str) -> str:
        system, user = split_monologue_prompt(prompt)
        if system is None:
            return call_llm(
                prompt, stage="monologue", model=_pro_model(), thinking=True)
        return complete(
            [
                {"role": "system", "parts": [{"text": system}]},
                {"role": "user", "parts": [{"text": user}]},
            ],
            model=_pro_model(),
            thinking=True,
            stage="monologue",
        )
    return _call


NARRATOR_TOOLS = [
    {
        "name": "query_graph",
        "description": (
            "Search the world graph by entity name or free text. For entity matches, "
            "returns the full entity-timeline chain (chronologically ordered nodes with "
            "valid_until annotations: -1 = still true, N = was true until turn N). "
            "For text matches, returns matching nodes with their chain predecessors. "
            "Use entity names like 'Player', 'Aqua' to trace an entity's history."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "Entity name (exact match, e.g. 'Player') or free-text search",
                },
            },
            "required": ["query"],
        },
    },
    {
        "name": "query_mind",
        "description": (
            "Inspect a character's mind: continuity core, active monologue "
            "streams (subtext), compact factual beliefs, and dialogue voice."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "character": {
                    "type": "string",
                    "description": "Character name",
                },
            },
            "required": ["character"],
        },
    },
    {
        "name": "query_history",
        "description": (
            "Search a scene's past conversation history for relevant events by keyword. "
            "Omit scene_id to search the scene currently being narrated."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "scene_id": {
                    "type": "string",
                    "description": "Optional scene id to search",
                },
                "query": {
                    "type": "string",
                    "description": "What to search for",
                },
            },
            "required": ["query"],
        },
    },
    {
        "name": "list_scenes",
        "description": (
            "List every live storyline (this one and the parallel ones) as JSON "
            "rows: scene_id, cast, driving_intention, charge, staleness, "
            "player_present, last_narration. Use it to find the id of another "
            "storyline you want to merge into."
        ),
        "parameters": {"type": "object", "properties": {}},
    },
]
# Lifecycle (fork/conclude/merge/exit) is not a narrator tool. A separate
# post-turn callback proposes operations, and Story applies their coded checks.


def make_narrator_callback():
    """A single narrator callback for every scene.

    The engine drives a turn and tells us which scene it is via `scene_id`; each
    tool call is forwarded through the call-scoped read function supplied by the
    engine. This module only adapts the LLM's tool-use protocol to that function.
    """
    def narrator(scene_id: str, instructions: str, turn_state: str, read_tool) -> str:
        def dispatch(name: str, args: dict) -> str:
            log.debug("[narrator %s] tool: %s %s", scene_id, name,
                     json.dumps(args or {}, ensure_ascii=False))
            return read_tool(name, json.dumps(args or {}))

        messages = [
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": turn_state}]},
        ]
        return complete_with_tools(
            messages, NARRATOR_TOOLS, dispatch,
            stage=f"narrator:{scene_id}",
            phase=infer_narrator_phase(instructions),
        )
    return narrator
