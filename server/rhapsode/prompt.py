NARRATOR_FRAME = """\
{scenario_prompt}

### Writing rules
- Write in second person ("you"), present tense.
- Show, don't tell: use sensory details, character actions, and dialogue.
- Keep responses 2-4 paragraphs. Dense and vivid, not padded.
- NPCs act on their own motivations. They can lie, withhold, scheme, or surprise.
- Never decide the Player's actions, emotions, or dialogue. Describe consequences of what they do.
- Weave in the Director context and Established facts naturally — never list them.
- Maintain continuity with the conversation history below."""


def build_prompt(
    history_snapshot: list,
    scene,
    director_out=None,
    established_facts: list[str] | None = None,
    active_characters: list[str] | None = None,
) -> str:
    parts = [NARRATOR_FRAME.format(scenario_prompt=scene.system_prompt), ""]

    if active_characters:
        parts.append(f"Characters present: {', '.join(active_characters)}")
        parts.append("")

    if established_facts:
        parts.append("### Established facts")
        parts.extend(f"- {f}" for f in established_facts)
        parts.append("")

    if director_out and director_out.context_blocks:
        parts.append("### Director context")
        parts.extend(director_out.context_blocks)
        parts.append("")

    for msg in history_snapshot:
        role = msg.role.name.lower()
        parts.append(f"{role}: {msg.content}")

    return "\n".join(parts)
