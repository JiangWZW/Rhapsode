"""LLM provider abstraction. Supports Gemini and DeepSeek, selectable via RHAPSODE_PROVIDER."""

import os
import time
import logging

log = logging.getLogger(__name__)

_provider = None

_MAX_RETRIES = 3

# Shared output-token ceiling for every provider.  Raised from the old hard-coded
# 4096 -- a reasoning/"pro" model can spend that on hidden reasoning and return
# empty content (finish_reason=length).  Billing is by actual usage, so a higher
# ceiling only adds headroom.  Override via env if a model still truncates.
_MAX_OUTPUT_TOKENS = int(os.environ.get("RHAPSODE_MAX_OUTPUT_TOKENS", "8192"))


def _retry_complete(fn):
    """Retry an LLM call with exponential backoff.

    Retries on BOTH a raised exception and a falsy (empty) return -- an empty
    completion is not an exception but is usually a transient gateway/model
    hiccup worth retrying.  Contract preserved: a persistent exception still
    raises on the final attempt; a persistent empty still returns "" (callers
    tolerate the empty string and must not start crashing on it).
    """
    result = ""
    for attempt in range(_MAX_RETRIES):
        last = attempt == _MAX_RETRIES - 1
        try:
            result = fn()
        except Exception as e:
            if last:
                raise
            wait = 2 ** attempt
            log.warning("LLM call failed (attempt %d/%d): %s -- retrying in %ds",
                        attempt + 1, _MAX_RETRIES, e, wait)
            time.sleep(wait)
            continue
        if result:
            return result
        if last:
            return result  # persistent empty -- preserve the "" fallback contract
        wait = 2 ** attempt
        log.warning("LLM returned empty (attempt %d/%d) -- retrying in %ds",
                    attempt + 1, _MAX_RETRIES, wait)
        time.sleep(wait)
    return result


def _get_provider():
    global _provider
    if _provider is not None:
        return _provider

    name = os.environ.get("RHAPSODE_PROVIDER", "gemini").lower()

    if name == "gemini":
        _provider = _GeminiProvider()
    elif name == "deepseek":
        _provider = _DeepSeekProvider()
    else:
        raise ValueError(f"Unknown RHAPSODE_PROVIDER: {name!r}. Use 'gemini' or 'deepseek'.")

    log.info("LLM provider: %s (model=%s)", name, _provider.model)
    return _provider


class _GeminiProvider:
    def __init__(self):
        from google import genai
        from google.genai.types import HttpOptions
        base_url = os.environ.get("RHAPSODE_API_BASE")
        http_options = HttpOptions(baseUrl=base_url) if base_url else None
        self.client = genai.Client(
            api_key=os.environ["GOOGLE_API_KEY"],
            http_options=http_options,
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "gemini-2.0-flash")

    def complete(self, messages: list[dict]) -> str:
        system_parts = []
        contents = []
        for msg in messages:
            if msg.get("role") == "system":
                system_parts.extend(p.get("text", "") for p in msg.get("parts", []))
            else:
                contents.append(msg)
        config = {"maxOutputTokens": _MAX_OUTPUT_TOKENS}
        if system_parts:
            config["systemInstruction"] = "\n".join(system_parts)
        kwargs = {"model": self.model, "contents": contents, "config": config}

        def _call():
            response = self.client.models.generate_content(**kwargs)
            text = response.text or ""
            if not text:
                try:
                    cand = (response.candidates or [None])[0]
                    log.warning(
                        "Gemini empty text: finish_reason=%s prompt_feedback=%s",
                        getattr(cand, "finish_reason", None),
                        getattr(response, "prompt_feedback", None),
                    )
                except Exception:  # noqa: BLE001 -- diagnostics must never throw
                    log.warning("Gemini empty text (no diagnostics available)")
            return text

        return _retry_complete(_call)


class _DeepSeekProvider:
    def __init__(self):
        from openai import OpenAI
        self.client = OpenAI(
            api_key=os.environ["DEEPSEEK_API_KEY"],
            base_url=os.environ.get("DEEPSEEK_API_BASE", "https://api.deepseek.com"),
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "deepseek-chat")

    def complete(self, messages: list[dict]) -> str:
        converted = _to_openai_messages(messages)

        def _call():
            response = self.client.chat.completions.create(
                model=self.model,
                messages=converted,
                temperature=1.0,
                max_tokens=_MAX_OUTPUT_TOKENS,
            )
            choice = response.choices[0]
            content = choice.message.content or ""
            if not content:
                reasoning = getattr(choice.message, "reasoning_content", "") or ""
                log.warning(
                    "DeepSeek empty content: finish_reason=%s reasoning_len=%d usage=%s",
                    getattr(choice, "finish_reason", None),
                    len(reasoning),
                    getattr(response, "usage", None),
                )
            return content

        return _retry_complete(_call)


def _to_openai_messages(gemini_messages: list[dict]) -> list[dict]:
    """Convert Gemini-style messages to OpenAI-style messages."""
    result = []
    for msg in gemini_messages:
        role = msg.get("role", "user")
        parts = msg.get("parts", [])
        text = " ".join(p.get("text", "") for p in parts if "text" in p)
        result.append({"role": role, "content": text})
    return result


def complete(messages: list[dict]) -> str:
    """Call the configured LLM provider."""
    return _get_provider().complete(messages)
