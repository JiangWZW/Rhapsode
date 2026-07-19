"""Lifecycle verdict adapter.

The lifecycle policy -- when a beat forks, merges, concludes, or exits a
character -- lives in the C++ engine (`Story::decide_lifecycle`). This module only
runs the completion: the engine hands us the instructions and a description of the
beat, and we return the model's JSON verdict for the engine to parse and apply.

It is deliberately a plain, focused completion (no tool loop): isolating this one
decision -- instead of leaving it as an optional tool competing with prose -- is
what makes forking reliable. It runs on the reasoning model for consistent
rule-following.
"""

from rhapsode.llm import complete_reasoning


def make_lifecycle_callback():
    def lifecycle(instructions: str, user: str) -> str:
        return complete_reasoning([
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": user}]},
        ])
    return lifecycle
