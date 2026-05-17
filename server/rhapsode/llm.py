"""LLM provider abstraction. Supports Gemini and DeepSeek, selectable via RHAPSODE_PROVIDER."""

import os
import logging

log = logging.getLogger(__name__)

_provider = None


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
        response = self.client.models.generate_content(
            model=self.model,
            contents=messages,
        )
        return response.text


class _DeepSeekProvider:
    def __init__(self):
        from openai import OpenAI
        self.client = OpenAI(
            api_key=os.environ["DEEPSEEK_API_KEY"],
            base_url="https://api.deepseek.com",
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "deepseek-chat")

    def complete(self, messages: list[dict]) -> str:
        converted = _to_openai_messages(messages)
        response = self.client.chat.completions.create(
            model=self.model,
            messages=converted,
        )
        return response.choices[0].message.content


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
    """Call the configured LLM provider. Drop-in replacement for gemini.complete."""
    return _get_provider().complete(messages)
