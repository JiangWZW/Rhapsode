"""Narrator tool schemas and the callback that runs one tool-use loop through
the engine's call-scoped read function."""

import json
import logging
import os
from concurrent.futures import wait

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
PERCEPTION_USER_SENTINEL = "<<<RHAPSODE_PERCEPTION_USER>>>"


def split_on_sentinel(prompt: str, sentinel: str) -> tuple[str | None, str]:
    idx = prompt.find(sentinel)
    if idx < 0:
        return None, prompt
    system = prompt[:idx]
    if system.endswith("\n"):
        system = system[:-1]
    user = prompt[idx + len(sentinel):]
    if user.startswith("\n"):
        user = user[1:]
    return system, user


def split_monologue_prompt(prompt: str) -> tuple[str | None, str]:
    """Split the native concatenated monologue blob into system + user."""
    return split_on_sentinel(prompt, MONOLOGUE_USER_SENTINEL)


def _system_user_callback(
    sentinel: str,
    stage: str,
    *,
    model: str | None,
    thinking: bool,
):
    def _call(prompt: str) -> str:
        system, user = split_on_sentinel(prompt, sentinel)
        if system is None:
            return call_llm(
                prompt, stage=stage, model=model, thinking=thinking)
        return complete(
            [
                {"role": "system", "parts": [{"text": system}]},
                {"role": "user", "parts": [{"text": user}]},
            ],
            model=model,
            thinking=thinking,
            stage=stage,
        )
    return _call


def make_reflection_callback():
    """On-stage monologue updater: narrator/pro model, thinking on."""
    return _system_user_callback(
        MONOLOGUE_USER_SENTINEL, "monologue",
        model=_pro_model(), thinking=True,
    )


def make_perception_callback():
    """Per-character perception: base (flash) model, thinking on."""
    return _system_user_callback(
        PERCEPTION_USER_SENTINEL, "perception",
        model=_base_model(), thinking=True,
    )


class PromptJobs:
    def __init__(self, call, pool):
        self._call = call
        self._pool = pool
        self._futs = {}

    def submit(self, jobs):
        for job in jobs:
            gen = getattr(job, "generation", 0)
            self._futs[(job.handle, job.staging_buf_id)] = (
                self._pool.submit(self._call, job.prompt), gen)

    def ready(self, handle, staging_buf_id, generation=0):
        item = self._futs.get((handle, staging_buf_id))
        if item is None:
            return None
        fut, gen = item
        if gen != generation:
            return None
        if not fut.done():
            return None
        self._futs.pop((handle, staging_buf_id))
        try:
            return fut.result(), False
        except Exception:
            log.exception(
                "prompt job failed handle=%s slot=%s gen=%s",
                handle, staging_buf_id, generation)
            return "", True

    def wait(self, timeout=None):
        if self._futs:
            wait([item[0] for item in self._futs.values()], timeout=timeout)


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
            "This turn's interior for one character: what they just took in "
            "(perception) and their last private lines (monologue). This is "
            "not who they durably are. It does not return the Who you are "
            "page."
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

QUERY_CHARACTER_CORE_TOOL = {
    "name": "query_character_core",
    "description": (
        "The Who you are page for one character. Call this before you write "
        "what they do or say. This is not this turn's thought; that is "
        "query_mind. Do not paste the page into prose or into speech_turns.line."
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
}

# Lifecycle (fork/conclude/merge/exit) is not a narrator tool. A separate
# post-turn callback proposes operations, and Story applies their coded checks.


def _env_flag_on(name: str, default: bool = True) -> bool:
    raw = os.environ.get(name)
    if raw is None or not str(raw).strip():
        return default
    return str(raw).strip().lower() not in ("0", "false", "off", "no")


def beat_narrator_tools() -> list[dict]:
    tools = list(NARRATOR_TOOLS)
    if _env_flag_on("RHAPSODE_QUERY_CHARACTER_CORE", True):
        tools.append(QUERY_CHARACTER_CORE_TOOL)
    return tools


def narrator_max_rounds() -> int:
    raw = os.environ.get("RHAPSODE_NARRATOR_MAX_ROUNDS", "").strip()
    if raw.isdigit() and int(raw) > 0:
        return int(raw)
    return 24


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
            messages, beat_narrator_tools(), dispatch,
            model=_pro_model(),
            stage=f"narrator:{scene_id}",
            phase=infer_narrator_phase(instructions),
            max_rounds=narrator_max_rounds(),
        )
    return narrator
