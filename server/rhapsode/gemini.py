import os
from google import genai

_client = None

def complete(messages: list[dict]) -> str:
    global _client
    if _client is None:
        _client = genai.Client(api_key=os.environ["GOOGLE_API_KEY"])
    model = os.environ.get("RHAPSODE_MODEL", "gemini-2.0-flash")
    response = _client.models.generate_content(
        model=model,
        contents=messages,
    )
    return response.text
