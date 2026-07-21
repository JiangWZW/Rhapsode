"""Scheduler LLM adapter.

The scheduler's policy -- the instructions, the weighing, and validating the
pick -- lives in the C++ engine (`Story::pick_off_stage_scene`). This module only
holds the tool schemas and runs the tool-use loop through a call-scoped native
read function."""

import json

from rhapsode.llm import complete_with_tools
from rhapsode.llm_tools import NARRATOR_TOOLS

SCHEDULER_TOOLS = [
    NARRATOR_TOOLS[3],  # list_scenes
    NARRATOR_TOOLS[0],  # query_graph
    NARRATOR_TOOLS[1],  # query_mind
    {
        "name": "advance_scene",
        "description": (
            "Pick the single off-stage storyline to advance this turn. Call this "
            "exactly once, with the scene_id of the most deserving parallel scene "
            "(never the player's active scene). If no off-stage scene deserves a "
            "beat this turn, call it with an empty scene_id."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "scene_id": {
                    "type": "string",
                    "description": "The off-stage storyline to advance, or empty for none.",
                },
            },
            "required": ["scene_id"],
        },
    },
]


def make_scheduler_callback():
    """Adapt the scheduler tool-use loop.

    The engine passes its instructions, prompt, and a call-scoped read function;
    we run the tool loop, capture the `advance_scene` decision, and return the
    chosen scene_id ("" for none) for the engine to validate.
    """
    def scheduler(instructions: str, user: str, read_tool) -> str:
        picked = {"id": ""}

        def dispatch(name: str, args: dict) -> str:
            if name == "advance_scene":
                picked["id"] = (args.get("scene_id") or "").strip()
                return json.dumps({"ok": True, "picked": picked["id"]})
            return read_tool(name, json.dumps(args or {}))

        messages = [
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": user}]},
        ]
        complete_with_tools(messages, SCHEDULER_TOOLS, dispatch)
        return picked["id"]
    return scheduler
