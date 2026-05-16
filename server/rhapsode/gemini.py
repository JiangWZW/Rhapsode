import os
from google import genai
from google.genai.types import HttpOptions

_client = None


def _make_client() -> genai.Client:
    base_url = os.environ.get("RHAPSODE_API_BASE")
    http_options = HttpOptions(baseUrl=base_url) if base_url else None
    return genai.Client(
        api_key=os.environ["GOOGLE_API_KEY"],
        http_options=http_options,
    )


def complete(messages: list[dict]) -> str:
    global _client
    if _client is None:
        _client = _make_client()
    model = os.environ.get("RHAPSODE_MODEL", "gemini-2.0-flash")
    response = _client.models.generate_content(
        model=model,
        contents=messages,
    )
    return response.text
