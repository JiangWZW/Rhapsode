"""Merged narrator + plot-graph prompts (single premium-LLM call)."""

GRAPH_RULES = """\
GRAPH OUTPUT (inside the JSON blob below):

  transitions:
    [{"id": <number>, "state": "dormant|foreshadowed|active|resolved"}, ...]

  new_nodes:
    [{"fact": <string>, "type": "<plot|scene|world|relationship>",
      "state": "dormant|foreshadowed|active|resolved",
      "foreshadow_ctx": <string>, "active_ctx": <string>,
      "known_by": [<string>], "entities": [<optional strings>]}, ...]

PLAYER AGENCY (strict):
- NEVER generate facts describing Player actions the Player has not taken.
- Only the Player's own words determine what they do.
- You MAY foreshadow Player options ("foreshadowed") but NEVER assert them as resolved.

FACT FORMAT — one atomic proposition, <=15 spoken English tokens, numeric digits okay:

GOOD: "barkeep owes thieves guild 200g"
BAD:  "The barkeep finally admits he owes coins because reasons"

Maintain tension — max ~3 new_nodes / turn unless graph is sparse.
Contexts must be evocative plain prose — no bullets, Markdown, quotation marks."""

SPEECH_RULES = """\
speech_turns (ordering matters):
[
  {"character": "<exact scenario name>", "cue": "<1–2 sentences of emotional / situational context — NOT scripted speech>"}
]

Include one entry each time an NPC should audibly react this chronology;
use [] only if the narrator beat is ambience-only."""

NARRATIVE_FRAME = """\
### Narrative (markdown body, FLOW 1)
2–4 dense paragraphs — second-person present, sensory grounding.

RULES FOR THIS BODY:
- **No quoted dialogue.** Never wrap spoken lines in quotation marks — that work is outsourced.
- NEVER write lines for NPCs verbatim; cues go in speech_turns only.
- *Italic emphasis* sparingly for sensory hits.

THEN output the sentinel line verbatim on its own:
<<<RHAPSODE_JSON>>>

Immediately follow with raw JSON (**no fences**) matching:
{
  "transitions": [...],
  "new_nodes": [...],
  "speech_turns": [...]
}
"""


def build_merged_prompt(
    history_snapshot,
    scene,
    director_out=None,
    *,
    director_focus_json: str = "{}",
    established_facts: list[str] | None = None,
    active_characters: list[str] | None = None,
) -> str:
    parts = [
        scene.system_prompt.strip(),
        "",
        NARRATIVE_FRAME,
        GRAPH_RULES,
        "",
        SPEECH_RULES,
    ]

    if active_characters:
        parts += ["", "Characters present:", ", ".join(active_characters)]

    if established_facts:
        parts += ["", "### Established memories", *[f"- {f}" for f in established_facts]]

    if director_out and director_out.context_blocks:
        parts += ["", "### Active plot pressures", *director_out.context_blocks]

    parts += [
        "",
        "### Plot graph snapshot JSON (immutable — reference exact node IDs)",
        director_focus_json,
        "",
        "### Conversation backlog",
        *[f"{m.role.name.lower()}: {m.content}" for m in history_snapshot],
    ]

    return "\n".join(parts)
