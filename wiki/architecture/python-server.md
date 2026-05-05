# Python server

The server layer lives in `server/rhapsode/`. It is a standard Python package with a FastAPI application, WebSocket endpoint, LLM client abstraction, and prompt builder.

## Package structure

```
server/
├── pyproject.toml          # Package metadata, dependencies, entry points
├── rhapsode/
│   ├── __init__.py
│   ├── app.py              # FastAPI application factory
│   ├── ws.py               # WebSocket endpoint
│   ├── session.py          # Per-session state management
│   ├── prompt.py           # Prompt builder
│   └── llm/
│       ├── __init__.py     # get_client() factory
│       ├── base.py         # BaseLLMClient ABC
│       ├── gemini.py       # Google Gemini implementation
│       └── openai.py       # OpenAI implementation
└── scenarios/
    └── tavern.json         # Sample scenario file
```

## Application factory (`app.py`)

```python
from fastapi import FastAPI
from rhapsode.ws import router as ws_router

def create_app() -> FastAPI:
    app = FastAPI(title="Rhapsode")
    app.include_router(ws_router)
    return app

app = create_app()
```

Run with: `uvicorn rhapsode.app:app --reload`

## WebSocket endpoint (`ws.py`)

Single endpoint at `/ws`. Protocol:

```
Client -> Server (JSON):
  { "type": "player_message", "content": "I draw my sword." }

Server -> Client (JSON):
  { "type": "assistant_message", "content": "The barkeep looks alarmed..." }
  { "type": "status", "state": "building_prompt" | "running_llm" | "idle" }
  { "type": "error", "detail": "..." }
```

### Lifecycle

1. Client connects to `ws://host:8000/ws`.
2. Server creates a `Session` (loads default scenario or accepts a scenario ID).
3. Client sends `player_message`.
4. Server calls `session.loop.submit_input(text)`.
5. Callbacks fire: prompt builder -> LLM -> turn complete.
6. `turn_complete_callback` pushes `assistant_message` to the WebSocket.
7. Client disconnects -> session is cleaned up.

### Sketch

```python
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from rhapsode.session import Session

router = APIRouter()

@router.websocket("/ws")
async def websocket_endpoint(ws: WebSocket):
    await ws.accept()
    session = Session.create_default()

    async def on_turn_complete(msg):
        await ws.send_json({
            "type": "assistant_message",
            "content": msg.content
        })

    session.set_turn_complete_handler(on_turn_complete)

    try:
        while True:
            data = await ws.receive_json()
            if data.get("type") == "player_message":
                await session.handle_input(data["content"])
    except WebSocketDisconnect:
        session.cleanup()
```

## Session management (`session.py`)

Each WebSocket connection gets a `Session` that owns:

- A `Scene` instance (loaded from scenario JSON)
- A `SceneLoop` instance (with callbacks wired)
- A reference to the LLM client

```python
from rhapsode._core import Scene, SceneLoop
from rhapsode.prompt import build_prompt
from rhapsode.llm import get_client

class Session:
    def __init__(self, scene: Scene, loop: SceneLoop, llm_client):
        self.scene = scene
        self.loop = loop
        self.llm_client = llm_client

    @classmethod
    def create_default(cls) -> "Session":
        scene = Scene.load_json("scenarios/tavern.json")
        loop = SceneLoop()
        loop.load_scene(scene)
        client = get_client()  # reads config/env for provider
        return cls(scene, loop, client)

    async def handle_input(self, text: str):
        # SceneLoop.submit_input is synchronous (C++),
        # but callbacks may need async (LLM HTTP).
        # MVP: run in thread executor to avoid blocking the event loop.
        import asyncio
        await asyncio.get_event_loop().run_in_executor(
            None, self.loop.submit_input, text
        )
```

## Prompt builder (`prompt.py`)

Transforms scene state into the messages list expected by chat LLM APIs.

```python
def build_prompt(history_snapshot: list, scene) -> str:
    """Build a prompt string from history and scenario metadata.

    For MVP, this concatenates the system prompt with recent history
    into a simple chat-style messages format.
    """
    messages = []

    # System message from scenario
    messages.append({
        "role": "system",
        "content": scene.system_prompt
    })

    # Character context
    chars = ", ".join(
        f"{c.name} ({c.description})"
        for c in scene.characters if not c.is_player
    )
    if chars:
        messages.append({
            "role": "system",
            "content": f"Characters present: {chars}"
        })

    # History (already ordered chronologically)
    for msg in history_snapshot:
        messages.append({
            "role": msg.role.name.lower(),
            "content": msg.content
        })

    return messages
```

## LLM client abstraction (`llm/`)

### Base class (`llm/base.py`)

```python
from abc import ABC, abstractmethod

class BaseLLMClient(ABC):
    @abstractmethod
    async def complete(self, messages: list[dict]) -> str:
        """Send messages to the LLM and return assistant text."""
        ...
```

### Gemini client (`llm/gemini.py`)

Primary client for MVP. Uses `google-genai` SDK.

```python
from google import genai
from rhapsode.llm.base import BaseLLMClient

class GeminiClient(BaseLLMClient):
    def __init__(self, model: str = "gemini-2.0-flash"):
        self.client = genai.Client()  # reads GOOGLE_API_KEY from env
        self.model = model

    async def complete(self, messages: list[dict]) -> str:
        response = self.client.models.generate_content(
            model=self.model,
            contents=messages
        )
        return response.text
```

### OpenAI client (`llm/openai.py`)

Secondary/alternative client.

```python
from openai import AsyncOpenAI
from rhapsode.llm.base import BaseLLMClient

class OpenAIClient(BaseLLMClient):
    def __init__(self, model: str = "gpt-4o-mini"):
        self.client = AsyncOpenAI()  # reads OPENAI_API_KEY from env
        self.model = model

    async def complete(self, messages: list[dict]) -> str:
        response = await self.client.chat.completions.create(
            model=self.model,
            messages=messages
        )
        return response.choices[0].message.content
```

### Factory (`llm/__init__.py`)

```python
import os
from rhapsode.llm.base import BaseLLMClient

def get_client() -> BaseLLMClient:
    provider = os.environ.get("RHAPSODE_LLM_PROVIDER", "gemini")
    if provider == "openai":
        from rhapsode.llm.openai import OpenAIClient
        return OpenAIClient()
    from rhapsode.llm.gemini import GeminiClient
    return GeminiClient()
```

## pybind11 import pattern

The C++ binding module is built as `_core.so` / `_core.pyd` and placed in the `rhapsode` package directory:

```python
from rhapsode._core import Scene, SceneLoop, SceneMessage, History, Character, Role
```

This is imported by `session.py` and `prompt.py`. The binding module is built by CMake and either installed into the package or symlinked during development.

## Configuration

MVP uses environment variables:

| Variable | Default | Purpose |
|----------|---------|---------|
| `RHAPSODE_LLM_PROVIDER` | `gemini` | Which LLM client to use (`gemini` or `openai`) |
| `GOOGLE_API_KEY` | (required for Gemini) | Gemini API key |
| `OPENAI_API_KEY` | (required for OpenAI) | OpenAI API key |
| `RHAPSODE_SCENARIO` | `scenarios/tavern.json` | Default scenario file path |
