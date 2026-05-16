"""Local LLM callback for C++ MemorySystem."""

from __future__ import annotations

import json
import logging

import httpx
import json_repair

log = logging.getLogger(__name__)

LLAMA_URL = "http://localhost:8012"
LLAMA_TIMEOUT = 120.0


def make_local_llm_callback(url: str = LLAMA_URL) -> callable:
    """Create a synchronous local LLM callback for C++ MemorySystem.

    Uses llama.cpp server's OpenAI-compatible endpoint.
    Returns "" if unreachable — C++ handles fallback logic.
    Repairs malformed JSON before returning (LLMs produce trailing commas,
    missing quotes, markdown fences, etc.).
    """
    client = httpx.Client(base_url=url, timeout=LLAMA_TIMEOUT)

    def call(prompt: str) -> str:
        try:
            resp = client.post(
                "/v1/chat/completions",
                json={
                    "messages": [{"role": "user", "content": prompt}],
                    "temperature": 0.0,
                },
            )
            resp.raise_for_status()
            raw = resp.json()["choices"][0]["message"]["content"]
            repaired = json_repair.loads(raw)
            return json.dumps(repaired)
        except Exception:
            log.warning("llama.cpp server unreachable or unparseable", exc_info=True)
            return ""

    return call
