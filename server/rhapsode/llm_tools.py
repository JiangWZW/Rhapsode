"""Narrator tool-use infrastructure: the read/decision tool schemas, the
dispatcher that routes tool calls to the shared World/Scene, and the callback
that runs the tool-use loop for a narrator beat."""

import json
import logging
import weakref

from rhapsode.llm import complete, complete_with_tools

log = logging.getLogger(__name__)


def call_llm(prompt: str) -> str:
    return complete([{"role": "user", "parts": [{"text": prompt}]}])


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
            "Get a character's current thoughts, beliefs, emotional state, "
            "and dialogue voice."
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
        "description": "Search past conversation history for relevant events by keyword.",
        "parameters": {
            "type": "object",
            "properties": {
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
# NOTE: lifecycle (fork/conclude/merge/exit) is NOT a narrator tool. It is decided
# by a dedicated verdict (Story::decide_lifecycle + rhapsode.lifecycle), the sole
# authority on cross-scene membership -- see that module for why.


def make_narrator_callback(story):
    """A single narrator callback for every scene.

    The engine drives a beat and tells us which scene it is via `scene_id`; each
    tool call is forwarded to the engine's own dispatcher (`Story.dispatch_tool`),
    so the tool bodies -- graph/mind/history reads and the fork/conclude/merge
    decisions -- live in C++, not here. This module only adapts the LLM's
    tool-use protocol to that dispatcher.
    """
    story_ref = weakref.proxy(story)

    def narrator(scene_id: str, instructions: str, turn_state: str) -> str:
        def dispatch(name: str, args: dict) -> str:
            log.info("[narrator %s] tool: %s %s", scene_id, name,
                     json.dumps(args or {}, ensure_ascii=False))
            return story_ref.dispatch_tool(scene_id, name, json.dumps(args or {}))

        messages = [
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": turn_state}]},
        ]
        return complete_with_tools(messages, NARRATOR_TOOLS, dispatch)
    return narrator
