"""Optional per-API-hop profiling for DeepSeek (and compatible) calls.

Enable: RHAPSODE_LLM_PROFILE=1
Path:   RHAPSODE_LLM_PROFILE_PATH (default: logs/llm_profile.jsonl)

Each enabled hop appends one JSON line. No extra console spam — callers keep
their normal start/done logs. wall_ms is full client RTT (not prefill vs decode).
"""

from __future__ import annotations

import json
import logging
import os
import threading
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

log = logging.getLogger(__name__)

_lock = threading.Lock()
_seq = 0


def profiling_enabled() -> bool:
    raw = (os.environ.get("RHAPSODE_LLM_PROFILE") or "").strip().lower()
    return raw not in ("", "0", "false", "off", "no")


def profile_path() -> Path:
    override = (os.environ.get("RHAPSODE_LLM_PROFILE_PATH") or "").strip()
    if override:
        return Path(override)
    root = Path(os.environ.get("RHAPSODE_LOG_DIR") or "logs")
    return root / "llm_profile.jsonl"


def infer_narrator_phase(instructions: str) -> str:
    """Beat vs graph from the stable GRAPH_UPDATE marker in C++ prompts."""
    return "graph" if "GRAPH_UPDATE" in (instructions or "") else "beat"


def _usage_fields(usage: Any) -> dict[str, int]:
    if usage is None:
        return {}
    out: dict[str, int] = {}
    for key in (
        "prompt_tokens",
        "completion_tokens",
        "total_tokens",
        "prompt_cache_hit_tokens",
        "prompt_cache_miss_tokens",
    ):
        val = getattr(usage, key, None)
        if isinstance(val, int):
            out[key] = val
    details = getattr(usage, "completion_tokens_details", None)
    if details is not None:
        reasoning = getattr(details, "reasoning_tokens", None)
        if isinstance(reasoning, int):
            out["reasoning_tokens"] = reasoning
    prompt_details = getattr(usage, "prompt_tokens_details", None)
    if prompt_details is not None and "prompt_cache_hit_tokens" not in out:
        cached = getattr(prompt_details, "cached_tokens", None)
        if isinstance(cached, int):
            out["prompt_cache_hit_tokens"] = cached
    return out


def reasoning_tokens(usage: Any) -> int | None:
    return _usage_fields(usage).get("reasoning_tokens")


@dataclass(frozen=True)
class ApiHop:
    seq: int
    ts: str
    kind: str  # "complete" | "tools_round"
    stage: str
    phase: str
    model: str
    thinking: bool
    wall_ms: int
    finish: str | None = None
    content_len: int = 0
    tool_round: int | None = None
    tool_calls: int = 0
    tools: str = ""
    prompt_tokens: int | None = None
    completion_tokens: int | None = None
    total_tokens: int | None = None
    reasoning_tokens: int | None = None
    prompt_cache_hit_tokens: int | None = None
    prompt_cache_miss_tokens: int | None = None
    reasoning: str | None = None


def record_api_hop(
    *,
    kind: str,
    stage: str,
    phase: str,
    model: str,
    thinking: bool,
    wall_ms: int,
    finish: str | None = None,
    content_len: int = 0,
    tool_round: int | None = None,
    tool_calls: int = 0,
    tools: str = "",
    usage: Any = None,
    reasoning: str | None = None,
) -> ApiHop | None:
    """Append one hop to the JSONL file when profiling is enabled."""
    if not profiling_enabled():
        return None

    global _seq
    with _lock:
        _seq += 1
        seq = _seq

    fields = _usage_fields(usage)
    hop = ApiHop(
        seq=seq,
        ts=datetime.now(timezone.utc).isoformat(timespec="milliseconds"),
        kind=kind,
        stage=stage,
        phase=phase,
        model=model,
        thinking=thinking,
        wall_ms=wall_ms,
        finish=finish,
        content_len=content_len,
        tool_round=tool_round,
        tool_calls=tool_calls,
        tools=tools,
        prompt_tokens=fields.get("prompt_tokens"),
        completion_tokens=fields.get("completion_tokens"),
        total_tokens=fields.get("total_tokens"),
        reasoning_tokens=fields.get("reasoning_tokens"),
        prompt_cache_hit_tokens=fields.get("prompt_cache_hit_tokens"),
        prompt_cache_miss_tokens=fields.get("prompt_cache_miss_tokens"),
        reasoning=reasoning or None,
    )

    path = profile_path()
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        line = json.dumps(asdict(hop), ensure_ascii=False)
        with _lock:
            with path.open("a", encoding="utf-8") as f:
                f.write(line + "\n")
    except OSError as exc:
        log.warning("llm profile write failed path=%s: %s", path, exc)
        return None
    return hop
