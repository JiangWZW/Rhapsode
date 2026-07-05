"""Local LLM callback for C++ MemorySystem."""

from __future__ import annotations

import json
import logging
import os
import re
import sys
import time

import httpx
import json_repair

log = logging.getLogger(__name__)

LLAMA_URL = os.environ.get("RHAPSODE_LOCAL_LLM_URL", "http://127.0.0.1:8012")
LLAMA_TIMEOUT = float(os.environ.get("RHAPSODE_LOCAL_LLM_TIMEOUT", "120"))
_HTTP_BODY_LOG_LIMIT = 2048


def _call_kind(prompt: str) -> str:
    """Tag local LLM calls for correlated logging with C++ [weave] / [expiry] lines."""
    if "narrative knowledge graph" in prompt and "### Current edges" in prompt:
        return "weave"
    if "NO LONGER TRUE" in prompt and '"superseded"' in prompt:
        return "expiry"
    if "continuity editor" in prompt and "contradicts established facts" in prompt:
        return "validator"
    return "other"


def _prompt_prefix(prompt: str, limit: int = 120) -> str:
    collapsed = " ".join(prompt.split())
    if len(collapsed) <= limit:
        return collapsed
    return collapsed[: limit - 3] + "..."


def _log_local_llm(kind: str, message: str) -> None:
    line = f"  [local_llm:{kind}] {message}"
    print(line, file=sys.stderr, flush=True)
    log.warning("[local_llm:%s] %s", kind, message)


def _post_local_chat_completion(client, base: str, prompt: str) -> dict:
    """POST to the chat completions endpoint, return parsed JSON payload."""
    resp = client.post(
        f"{base}/v1/chat/completions",
        json={
            "messages": [
                {"role": "user", "content": prompt},
            ],
            "temperature": 0.0,
            "max_tokens": 4096,
        },
    )
    resp.raise_for_status()
    return resp.json()


def _extract_local_llm_choice(payload: dict):
    """Extract (raw, source, finish_reason, usage) from the response payload.

    Falls back to reasoning_content when content is empty.
    """
    choice = payload.get("choices", [{}])[0]
    msg = choice.get("message", {})
    finish_reason = choice.get("finish_reason")
    usage = payload.get("usage")
    raw = (msg.get("content") or "").strip()
    source = "content"
    if not raw:
        reasoning = (msg.get("reasoning_content") or "").strip()
        if reasoning:
            raw = reasoning
            source = "reasoning_content"
    return raw, source, finish_reason, usage


def _strip_redacted_thinking(raw: str) -> str:
    """Remove redacted-thinking tags (<think>...</think>) from the response."""
    answer = re.sub(
        r"<think>.*?</think>", "", raw, flags=re.DOTALL
    ).strip()
    if not answer:
        m = re.search(r"<think>(.*)</think>", raw, flags=re.DOTALL)
        answer = m.group(1).strip() if m else raw.strip()
    return answer


def _finalize_local_llm_response(answer: str, *, repair_json: bool,
                                 kind: str) -> str:
    """Return the final string — plaintext verbatim or JSON-repaired."""
    if not repair_json:
        _log_local_llm(kind, f"plaintext — returning {len(answer)} chars")
        return answer

    repaired = json_repair.loads(answer)
    if isinstance(repaired, str):
        repaired = json_repair.loads(repaired)
    if isinstance(repaired, dict):
        out = json.dumps(repaired)
        _log_local_llm(kind, f"json_repair ok — returning {len(out)} chars")
        return out

    _log_local_llm(
        kind,
        f"json_repair yielded non-object — returning raw ({len(answer)} chars)",
    )
    return answer


def _local_llm_call(client, base: str, prompt: str, *, repair_json: bool) -> str:
    kind = _call_kind(prompt)
    t0 = time.monotonic()
    _log_local_llm(
        kind,
        f"POST {base}/v1/chat/completions prompt_chars={len(prompt)} "
        f'prefix="{_prompt_prefix(prompt)}"',
    )
    try:
        payload = _post_local_chat_completion(client, base, prompt)
        elapsed_ms = int((time.monotonic() - t0) * 1000)

        raw, source, finish_reason, usage = _extract_local_llm_choice(payload)
        if source == "reasoning_content":
            _log_local_llm(
                kind,
                f"HTTP 200 elapsed_ms={elapsed_ms} content empty — "
                f"using reasoning_content ({len(raw)} chars) "
                f"finish_reason={finish_reason!r} usage={usage!r}",
            )
        else:
            _log_local_llm(
                kind,
                f"HTTP 200 elapsed_ms={elapsed_ms} source={source} "
                f"response_chars={len(raw)} finish_reason={finish_reason!r} "
                f"usage={usage!r}",
            )

        answer = _strip_redacted_thinking(raw)
        if not answer:
            _log_local_llm(kind, "response empty after stripping — returning ''")
            return ""

        return _finalize_local_llm_response(answer, repair_json=repair_json,
                                             kind=kind)
    except httpx.HTTPStatusError as e:
        elapsed_ms = int((time.monotonic() - t0) * 1000)
        body = e.response.text[:_HTTP_BODY_LOG_LIMIT]
        _log_local_llm(
            kind,
            f"HTTP {e.response.status_code} elapsed_ms={elapsed_ms} body={body!r}",
        )
        return ""
    except Exception as exc:
        elapsed_ms = int((time.monotonic() - t0) * 1000)
        _log_local_llm(
            kind,
            f"failed elapsed_ms={elapsed_ms} error={type(exc).__name__}: {exc}",
        )
        log.warning("llama.cpp server unreachable or unparseable", exc_info=True)
        return ""


def make_local_llm_callback(url: str = LLAMA_URL, *, repair_json: bool = True) -> callable:
    """Create a synchronous local LLM callback for C++ MemorySystem.

    Uses llama.cpp server's OpenAI-compatible endpoint.
    Returns "" if unreachable — C++ handles fallback logic.

    When `repair_json` is True (default), malformed JSON is repaired before
    returning (LLMs produce trailing commas, missing quotes, markdown fences,
    etc.) — for the JSON consumers (validator, expiry, weave). When False, the
    stripped completion is returned verbatim — for plain-text consumers like the
    downsampler, whose one-line summaries are not JSON and must never pass
    through json_repair (which would coerce/mangle text containing {, :, or a
    bare number).
    """
    base = url.rstrip("/")

    # trust_env=False: never route the localhost call through an ambient
    # HTTP(S)_PROXY/ALL_PROXY. A proxy that can't forward to 127.0.0.1 returns
    # an empty-body 502 (which llama.cpp itself never emits), silently killing
    # weave/expiry/validator while cloud calls to the public API still work.
    client = httpx.Client(trust_env=False, timeout=LLAMA_TIMEOUT)

    return lambda prompt: _local_llm_call(client, base, prompt,
                                          repair_json=repair_json)
