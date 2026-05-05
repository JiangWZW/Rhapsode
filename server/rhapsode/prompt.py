def build_prompt(history_snapshot: list, scene) -> str:
    parts = [scene.system_prompt, ""]

    npc_names = [c.name for c in scene.characters if not c.is_player]
    if npc_names:
        parts.append(f"Characters present: {', '.join(npc_names)}")
        parts.append("")

    for msg in history_snapshot:
        role = msg.role.name.lower()
        parts.append(f"{role}: {msg.content}")

    return "\n".join(parts)
