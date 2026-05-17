"""NPC line synthesis via local llama.cpp OpenAI-compatible API."""

from __future__ import annotations

import logging
import os

import httpx

log = logging.getLogger(__name__)

LLAMA_URL = os.environ.get("RHAPSODE_LOCAL_LLM_URL", "http://localhost:8012")
LLAMA_TIMEOUT = float(os.environ.get("RHAPSODE_LOCAL_LLM_TIMEOUT", "120"))


def make_character_synth(scene, url: str = LLAMA_URL):
    """Closure for SceneLoop.set_character_synth_callback.

    Returns synchronized list[str] — one line per (name, cue).
    """
    by_name = {c.name: c for c in scene.characters}
    client = httpx.Client(base_url=url.rstrip("/"), timeout=LLAMA_TIMEOUT)

    def run(
        cues: list[tuple[str, str]],
        narration_snapshot: str,
    ) -> list[str]:
        lines: list[str] = []
        for name, cue in cues:
            ch = by_name.get(name)
            profile = ch.description if ch else "(no profile)"
            prompt = (
                f"You are **{name}** in a tabletop story.\n\n"
                f"Character sheet:\n{profile}\n\n"
                f"Latest narrator beat (for context, do not contradict):\n{narration_snapshot}\n\n"
                f"Stage direction for your line:\n{cue}\n\n"
                "Reply with **one** in-character spoken sentence (or two very short ones).\n"
                "No asterisks-actions, no narration, no quotation marks around the line — just the words you say aloud."
            )
            try:
                resp = client.post(
                    "/v1/chat/completions",
                    json={
                        "messages": [{"role": "user", "content": prompt}],
                        "temperature": 0.75,
                        "max_tokens": 200,
                    },
                )
                resp.raise_for_status()
                text = resp.json()["choices"][0]["message"]["content"].strip()
                lines.append(text if text else "...")
            except Exception:
                log.warning("Character synth failed for %s", name, exc_info=True)
                lines.append(f'({name} is at a loss for words.)')
        return lines

    return run
