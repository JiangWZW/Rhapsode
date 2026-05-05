# MVP v0 — minimal playable prototype

## Acceptance criteria

1. **C++** holds `Scene`, `History` (`SceneMessage` structs), characters; JSON serialize/deserialize.
2. **`SceneLoop` (C++)** drives turns: player input -> stage asks Python for **prompt** -> Python runs **LLM** -> C++ appends messages.
3. **FastAPI + WebSocket** — send player line; push assistant reply (+ optional status).
4. **Vue 3** — proper chat view (list + input + connection state).
5. **Scenario JSON** — load title, system stub, characters, optional seed messages.

## Explicitly out of scope for v0

- Vector DB / RAG / hnswlib / embeddings pipeline
- Node editor
- General-purpose graph runtime

## End-to-end turn flow

```mermaid
sequenceDiagram
    participant Vue as Vue3_SPA
    participant WS as WebSocket
    participant API as FastAPI
    participant Bind as pybind11
    participant Loop as SceneLoop_C++
    participant Prompt as PromptBuilder_Py
    participant LLM as LLM_Client_Py

    Vue->>WS: send player message
    WS->>API: on_message(text)
    API->>Bind: scene_loop.submit_input(text)
    Bind->>Loop: submit_input(text)
    Loop->>Loop: append user SceneMessage
    Loop->>Loop: advance to BuildPrompt stage
    Loop->>Bind: invoke prompt_callback
    Bind->>Prompt: build_prompt(history_snapshot, scenario)
    Prompt-->>Bind: messages list
    Loop->>Loop: advance to RunLLM stage
    Loop->>Bind: invoke llm_callback(messages)
    Bind->>LLM: complete(messages)
    LLM-->>Bind: assistant text
    Bind-->>Loop: assistant text
    Loop->>Loop: append assistant SceneMessage
    Loop->>Loop: advance to Idle
    API-->>WS: push assistant message
    WS-->>Vue: display assistant message
```

## Per-layer task breakdown

### C++ core (`core/`)

| File | Contents | Key API |
|------|----------|---------|
| `include/rhapsode/scene_message.h` | `Role` enum, `SceneMessage` struct | `Role::User`, `Role::Assistant`, `Role::System` |
| `include/rhapsode/history.h` | `History` class | `append()`, `snapshot(n)`, `size()`, `clear()` |
| `include/rhapsode/character.h` | `Character` struct | `name`, `description`, `is_player` |
| `include/rhapsode/scene.h` | `Scene` coordinator | `title`, `system_prompt`, `characters`, `history`, `load_json()`, `save_json()` |
| `include/rhapsode/scene_loop.h` | `SceneLoop` FSM | `submit_input()`, `advance()`, `state()`, callback setters |
| `src/*.cpp` | Implementations | JSON via nlohmann `to_json`/`from_json` |
| `tests/test_scene.cpp` | Unit tests | Catch2: create scene, serialize round-trip, loop state transitions |

### pybind11 bindings (`bindings/`)

| File | Exposed types |
|------|---------------|
| `bind_rhapsode.cpp` | `SceneMessage`, `History`, `Character`, `Scene`, `SceneLoop`; callback registration for prompt builder + LLM caller |

### Python server (`server/rhapsode/`)

| File | Responsibility |
|------|----------------|
| `app.py` | FastAPI app factory, lifespan (load scenario, create Scene) |
| `ws.py` | WebSocket endpoint: receive player text, call `scene_loop.submit_input()`, push reply |
| `session.py` | Per-session state: Scene instance, SceneLoop, connected client tracking |
| `prompt.py` | `build_prompt(history_snapshot, scenario_meta) -> list[dict]` |
| `llm/base.py` | `BaseLLMClient` ABC: `async complete(messages) -> str` |
| `llm/gemini.py` | Google Gemini implementation |
| `llm/openai.py` | OpenAI implementation |

### Vue frontend (`frontend/src/`)

| File | Responsibility |
|------|----------------|
| `App.vue` | Root layout, mounts ChatView |
| `components/ChatView.vue` | Container: MessageList + InputBar + ConnectionStatus |
| `components/MessageList.vue` | Renders list of messages (role-based styling) |
| `components/InputBar.vue` | Text input + send button, emits on submit |
| `components/ConnectionStatus.vue` | WebSocket state indicator |
| `stores/websocket.ts` | Pinia store: connect/disconnect/send/onMessage/reconnect |
| `types/index.ts` | `Message`, `ConnectionState` TypeScript interfaces |

## Scenario JSON schema

Example file: `server/scenarios/tavern.json`

```json
{
  "title": "The Dusty Flagon",
  "system_prompt": "You are the narrator of a fantasy RPG. Describe the scene vividly and respond to the player's actions in character.",
  "characters": [
    {
      "name": "Player",
      "description": "A wandering adventurer",
      "is_player": true
    },
    {
      "name": "Barkeep",
      "description": "A gruff dwarf who runs the tavern",
      "is_player": false
    }
  ],
  "seed_messages": [
    {
      "role": "assistant",
      "content": "You push open the heavy oak door of The Dusty Flagon. The warmth hits you first, then the smell of roasting meat and spilled ale. A dwarf behind the bar eyes you with cautious interest."
    }
  ]
}
```

## Definition of done

- [ ] `cmake --build build` succeeds; C++ tests pass (scene create, serialize round-trip, loop state transitions)
- [ ] `pip install -e ./server` imports `rhapsode._core` without error
- [ ] `uvicorn rhapsode.app:app` starts; WebSocket at `ws://localhost:8000/ws` accepts connections
- [ ] Send a player message via WebSocket; receive an LLM-generated assistant reply
- [ ] `frontend/` dev server shows chat UI; messages appear in real time
- [ ] Load `tavern.json` scenario at startup; title and seed messages display in chat
- [ ] Save/load a session (Scene JSON round-trip) works from the server

## Next slice (MVP+)

- Long-term memory: **hnswlib** in C++, embeddings from Python, retrieval merged into prompt-building stage.
