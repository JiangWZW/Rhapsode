"""Lifecycle verdict adapter.

The lifecycle policy -- when a beat forks, merges, concludes, or exits a
character -- lives in the C++ engine (`Story::decide_lifecycle`). This module only
runs the completion: the engine hands us the instructions and a description of the
beat, and we return the model's JSON verdict for the engine to parse and apply.

It remains a focused verdict call, but may inspect the engine's call-scoped
history, graph, mind, and live-scene tools before returning its decision.
"""

import json
import os

from rhapsode.llm import complete_with_tools
from rhapsode.llm_tools import NARRATOR_TOOLS


def make_lifecycle_callback():
    def lifecycle(instructions: str, user: str, read_tool) -> str:
        def dispatch(name: str, args: dict) -> str:
            return read_tool(name, json.dumps(args or {}))

        # Base model + thinking off — do not inherit RHAPSODE_DEEPSEEK_THINKING.
        return complete_with_tools([
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": user}]},
        ], NARRATOR_TOOLS, dispatch, model=os.environ.get("RHAPSODE_MODEL"),
           thinking=False, stage="lifecycle", phase="verdict")
    return lifecycle
