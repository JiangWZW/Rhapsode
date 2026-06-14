"""Narrator prompt assembly: static system message + dynamic user message."""

FORMAT_AND_RULES = """\
OUTPUT: 2-4 paragraphs of second-person present-tense prose, sensory grounding.
PROSE IS NARRATION ONLY -- describe what happens; never write what anyone says.
No quotation marks, no character dialogue, and no *asterisks* / stage directions in the
prose. Each character's spoken words go in speech_turns.line, written verbatim in that
character's own voice; writing speech in the prose duplicates it and breaks the display.
No markdown formatting in prose.

Then output the sentinel line verbatim on its own:
<<<RHAPSODE_JSON>>>
Then raw JSON (no fences). Use ONLY straight ASCII double quotes (") for all keys
and strings -- never smart/curly quotes (" " ' '), or the JSON will not parse:
{"transitions":[{"id":<node_id>,"state":"dormant|foreshadowed|active|resolved"}],
 "new_nodes":[{"fact":"<=15 words, atomic","type":"plot|scene|world|relationship","state":"dormant|foreshadowed|active|resolved","foreshadow_ctx":"...","active_ctx":"...","entities":[],"audience":[]}],
 "speech_turns":[{"character":"Name","line":"the actual words spoken, verbatim, in this character's voice","action":"brief stage action, optional"}],
 "new_characters":[{"name":"...","description":"2 sentences","dialogue_instructions":"1 sentence"}],
 "active_cast":["present NPC names"],
 "location":"current physical setting (only when it changes this turn)"}

RULES:
- Never narrate unperformed Player actions. May foreshadow options.
- Setting: the "### Setting" line is where the scene currently is. Honor it -- do not silently relocate or invent a new place. When the action genuinely moves (the player leaves, travels, goes home), set "location" to the new setting AND list in active_cast ONLY the characters who are actually there; everyone left behind at the old place is dropped (no speech_turn for them). Respect established places from the graph -- if the party's home is a mansion, they sleep in the mansion, not a new place you invent.
- Facts: each one atomic proposition, <=15 words. Emit a new_node for EVERY development the turn introduces -- a state change, a revealed intention, a threat, a death, a relationship shift, a thing learned -- not for sensory description or mood. Do NOT drop a real development to stay under a count; capture them all (typically 3-6, more on eventful turns).
- entities: the canonical subject(s) this fact is about. Use the EXACT name from the Cast for any NPC -- never a title, synonym, or description ("Warden Elara Voss", not "the warden"). For the player character (the "you" of the narration), ALWAYS use "Player". Coin a new string only for a genuinely new, unnamed thing/place/faction with no Cast entry; once you name it, reuse that exact string every time. This is how a fact reaches the right character's memory -- inconsistent names splinter one subject into several.
- audience: which characters perceive this fact (by name). Omit/[] for a public beat everyone present perceives. Name a narrow audience only when something is private -- a fact only one character learns or witnesses. This decides who knows what; the unlisted stay ignorant. (This never means writing dialogue in the prose.)
- speech_turns: one entry per NPC who speaks this turn; `line` is their exact words in their own voice, `action` an optional brief stage action. [] if ambience-only. Only characters who CAN speak right now -- never one who is asleep, unconscious, incapacitated, dead, or no longer present. If your prose just put someone to sleep or under, they get no speech_turn.
- new_characters: first-time speaking NPCs only. [] if none introduced.
- active_cast: all living NPCs physically present this scene. [] if player alone."""


def build_system_message(scene) -> str:
    """Static system message: scene premise + output format rules.

    Built once per session; cacheable by the API provider.
    """
    return scene.system_prompt.strip() + "\n\n" + FORMAT_AND_RULES


_VERBATIM_TAIL = 6
_MAX_MSG_CHARS = 400
_MAX_FACTS = 8
_MAX_STORY_CHARS = 1500


def build_user_message(
    history_snapshot,
    scene,
    *,
    director_focus_text: str = "",
    established_facts: list[str] | None = None,
    active_characters: list[str] | None = None,
    story_so_far: str = "",
    inner_states: str = "",
) -> str:
    """Dynamic user message: cast, graph, inner states, past memories, story, conversation."""
    parts: list[str] = []

    if active_characters:
        parts += ["### Cast", *active_characters]

    if director_focus_text:
        parts += ["", "### Graph", director_focus_text]

    if established_facts:
        facts = established_facts[:_MAX_FACTS]
        parts += ["", "### Past", *[f"- {f}" for f in facts]]

    if story_so_far:
        parts += ["", "### Story so far", story_so_far[:_MAX_STORY_CHARS]]

    if inner_states:
        # Pre-formatted by the C++ scene loop (already carries its own
        # "### Inner lives" header); splice in verbatim.
        parts += ["", inner_states.rstrip("\n")]

    conv = history_snapshot[-_VERBATIM_TAIL:] if story_so_far else history_snapshot
    parts += [
        "",
        "### Conversation",
        *[f"{m.role.name.lower()}: {m.content[:_MAX_MSG_CHARS]}" for m in conv],
    ]

    # Final, highest-salience reminder of the two most-violated constraints --
    # placed last so it survives the conversation-history feedback loop above.
    parts += [
        "",
        "### Remember",
        "- Prose is narration only: never a character's spoken words or *actions* -- "
        "each character's words go in speech_turns.line, written in their own voice.",
        "- Give a speech_turn only to a character who can speak right now "
        "(not asleep, unconscious, incapacitated, dead, or absent).",
    ]

    return "\n".join(parts)
