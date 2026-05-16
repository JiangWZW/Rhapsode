---
sources:
  - core/include/rhapsode/scene_loop.h
  - core/src/scene_loop.cpp
  - server/rhapsode/app.py
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/cpp-data-model]]"
  - "[[decisions/callback-vs-pull]]"
tags:
  - cpp-core
---

# SceneLoop

The SceneLoop is a C++ finite state machine that drives the turn cycle. It orchestrates the sequence: player input → Director tick → prompt building → LLM call → response append.

## State diagram

```
        load_scene()
[Idle] ──────────────→ [WaitingForInput]
                              │
                        submit_input(text)
                              │
                              ▼
                       [ProcessingInput]
                       (append user msg,
                        Director.tick())
                              │
                              ▼
                       [BuildingPrompt]
                       (prompt_callback)
                              │
                              ▼
                        [RunningLLM]
                        (llm_callback)
                              │
                              ▼
                       [AppendingResult]
                       (append assistant msg,
                        turn_complete_callback)
                              │
                              └──→ [WaitingForInput]
```

## States

| State | Description | Trigger to next |
|-------|-------------|-----------------|
| `Idle` | No scene loaded. | `load_scene(scene)` |
| `WaitingForInput` | Scene active, waiting for player text. | `submit_input(text)` |
| `ProcessingInput` | Appends user message to history. If a Director is set, calls `Director::tick()`. | Automatic |
| `BuildingPrompt` | Invokes the prompt callback. Passes a history window and the DirectorOutput. | Callback returns |
| `RunningLLM` | Invokes the LLM callback with the assembled prompt. | Callback returns |
| `AppendingResult` | Appends assistant message to history. Fires turn-complete notification. | Automatic → `WaitingForInput` |

## Director integration

When a `Director*` is set via `set_director()`, the `advance()` method calls `director->tick(turn_index, scene_context)` during the `ProcessingInput` stage. The resulting `DirectorOutput` is stored in `last_director_out_` and passed to the prompt callback.

The scene context is built from the system prompt and character descriptions. The Director uses this plus the current node pool to determine transitions, create new nodes, and collect context blocks for the narrative prompt.

## History windowing

The SceneLoop controls how many history messages reach the prompt callback:

| Mode | Window size | When |
|------|-------------|------|
| Normal | `window_size_` (default 3) | Regular turns |
| Resume | `resume_window_size_` (default 10) | First turn after loading a save |

After the first turn with resume, the flag resets to normal windowing. This gives the LLM more context to reorient after a session break without overloading every turn's prompt.

Configurable via `set_history_window(normal, resume)`.

## Callbacks

Three callbacks must be registered before the loop is usable:

### PromptCallback

```cpp
std::function<std::string(const std::vector<SceneMessage>&, const Scene&, const DirectorOutput&)>
```

Receives the history window, the scene object, and the Director's output. Returns the assembled prompt string. In practice, this calls `prompt.py:build_prompt()` which concatenates:

1. Scene system prompt
2. NPC character names
3. Director context blocks — `foreshadow_ctx` + `active_ctx` from active nodes
4. Recent history messages

### LLMCallback

```cpp
std::function<std::string(const std::string& prompt)>
```

Sends the prompt to the LLM and returns the assistant text. Calls `gemini.py:complete()` via the Gemini API.

### TurnCompleteCallback

```cpp
std::function<void(const SceneMessage& assistant_msg)>
```

Fires after the assistant message is appended. In the server, this captures the response text for the WebSocket push.

## Wiring in Python

The server wires the loop in `app.py:_wire_loop()`:

```python
loop = SceneLoop()
loop.load_scene(scene)
loop.set_director(director)
loop.set_prompt_callback(lambda history, scene_obj, director_out: build_prompt(history, scene_obj, director_out))
loop.set_llm_callback(lambda prompt: complete([{"role": "user", "parts": [{"text": prompt}]}]))
loop.set_turn_complete_callback(on_turn_complete)
```

`submit_input()` is synchronous C++, so the FastAPI server wraps it in `asyncio.run_in_executor()`. See [[callback-vs-pull]] for the rationale.

## Error recovery

If a turn fails — exception from LLM callback, JSON parse error — the server catches the exception and sends an error message to the client. It then reconstructs the SceneLoop from the scene state. The scene and Director remain valid; only the loop needs rewiring.

## Design notes

- `submit_input()` drives the entire turn synchronously through all stages. The loop is self-contained — no polling, no coroutine splitting.
- The loop does not own the Scene or Director. It holds pointers set by the caller. Ownership stays with the server session.
- The FSM design is intentionally simple. A general DAG runtime — async nodes, parallel execution, visual editor — was considered and deferred. See [[ownership-split]].
