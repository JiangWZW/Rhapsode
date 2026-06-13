"""Local LLM callback for C++ MemorySystem."""

from __future__ import annotations

import json
import logging
import os
import re

import httpx
import json_repair

log = logging.getLogger(__name__)

LLAMA_URL = os.environ.get("RHAPSODE_LOCAL_LLM_URL", "http://127.0.0.1:8012")
LLAMA_TIMEOUT = float(os.environ.get("RHAPSODE_LOCAL_LLM_TIMEOUT", "120"))


def make_local_llm_callback(url: str = LLAMA_URL) -> callable:
    """Create a synchronous local LLM callback for C++ MemorySystem.

    Uses llama.cpp server's OpenAI-compatible endpoint.
    Returns "" if unreachable — C++ handles fallback logic.
    Repairs malformed JSON before returning (LLMs produce trailing commas,
    missing quotes, markdown fences, etc.).
    """
    base = url.rstrip("/")

    def call(prompt: str) -> str:
        try:
            resp = httpx.post(
                f"{base}/v1/chat/completions",
                json={
                    "messages": [
                        {"role": "user", "content": prompt},
                    ],
                    "temperature": 0.0,
                    "max_tokens": 4096,
                },
                timeout=LLAMA_TIMEOUT,
            )
            resp.raise_for_status()
            msg = resp.json()["choices"][0]["message"]
            raw = msg.get("content") or ""
            answer = re.sub(r"<think>.*?</think>", "", raw, flags=re.DOTALL).strip()
            if not answer:
                m = re.search(r"<think>(.*)</think>", raw, flags=re.DOTALL)
                answer = m.group(1).strip() if m else raw.strip()
            repaired = json_repair.loads(answer)
            if isinstance(repaired, str):
                repaired = json_repair.loads(repaired)
            return json.dumps(repaired) if isinstance(repaired, dict) else answer
        except httpx.HTTPStatusError as e:
            log.warning("llama.cpp HTTP %s: %s", e.response.status_code, e.response.text[:500])
            return ""
        except Exception:
            log.warning("llama.cpp server unreachable or unparseable", exc_info=True)
            return ""

    return call
