---
sources:
  - core/include/rhapsode/scene_loop.h
  - core/src/scene_loop.cpp
  - server/rhapsode/app.py
last_updated: 2026-05-17
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

The SceneLoop is a C++ finite state machine that drives the turn cycle. It orchestrates the sequence: player input → merged prompt building → single LLM call → response splitting (prose + JSON) → Director graph apply → character synthesis → output collection.

## State diagram

```
        load_scene()
[Idle] ──────────────→ [WaitingForInput]
                              │
                        submit_input(text)
                              │
                       (append user msg)
                       (append user msg,
                       (Director focus_payload_json,
                        prompt_callback with 4 args)
                              │
                        (llm_callback,
                         split prose / JSON)
                       [BuildingPrompt]
                       (Director apply_planned_turn,
                        append narrator msg,
                        character synth callback,
                        append character msgs)
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
| `ProcessingInput` | Appends user message to history. | Automatic |
| `BuildingPrompt` | Gets Director focus payload, invokes the prompt callback with history window + scene + DirectorOutput + focus JSON. | Callback returns |
| `RunningLLM` | Invokes the LLM callback with the assembled prompt. Splits response into prose and JSON via `<<<RHAPSODE_JSON>>>` sentinel. | LLM returns |
| `AppendingResult` | Applies graph updates via Director. Appends narrator message. Runs character synthesis. Appends character dialogue messages. Fires turn-complete for each message. | Automatic → `WaitingForInput` |

## Merged response splitting

The LLM returns a single response containing narrative prose followed by a `<<<RHAPSODE_JSON>>>` sentinel and structured JSON:

```
The barkeep sets down a mug, eyes darting toward the door...
<<<RHAPSODE_JSON>>>
{"transitions": [...], "new_nodes": [...], "speech_turns": [...]}
```

`split_merged_response()` extracts the prose and JSON portions. If the sentinel is missing, it falls back to finding a trailing balanced JSON object. If no valid JSON is found, the entire response is treated as prose with an empty plan.

## Director integration

The SceneLoop integrates with the Director in two phases:

1. **Before prompt** — `director_->focus_payload_json(turn_index, scene_context)` builds the JSON context blob containing all non-resolved nodes and a 2-hop BFS neighborhood. This is passed to the prompt callback as the fourth argument.

2. **After LLM response** — `director_->apply_planned_turn(turn_index, json_plan)` applies transitions and new nodes from the parsed JSON. The resulting `DirectorOutput` is stored in `last_director_out_`.

The Director does not make its own LLM call in this flow — all graph instructions come from the merged narrator response.

## Character synthesis

After the narrator prose is appended, the SceneLoop extracts `speech_turns` from the JSON plan — an array of `{character, cue}` pairs indicating which NPCs should speak and the emotional/situational context for their lines.

If a `CharacterSynthCallback` is set, it receives the cues and the narrator prose snapshot. It returns one spoken line per cue. Each line is appended to history as a separate `SceneMessage` with `metadata.scene_kind = "character"` and `metadata.speaker = <name>`.

If the callback is not set, speech cues are silently skipped.

`build_actor_prompt` gives each speaking NPC two first-person knowledge sections (added 2026-06-06): an `Inner state` section reading the character's persistent `self_state()` (who I am right now — a free read of the state computed in Phase 1) and a `Relevant memories` section from the cue-keyed `briefing()` (what I recall about this beat).

## History windowing

The SceneLoop controls how many history messages reach the prompt callback:

| Mode | Window size | When |
|------|-------------|------|
| Normal | `window_size_` (default 8) | Regular turns |
| Resume | `resume_window_size_` (default 12) | First turn after loading a save |

After the first turn with resume, the flag resets to normal windowing. This gives the LLM more context to reorient after a session break without overloading every turn's prompt.

Configurable via `set_history_window(normal, resume)`.

## Output collection

`take_last_turn_outputs()` returns all SceneMessages produced during the last turn — the narrator message plus any character dialogue messages. The internal buffer is cleared on consume. The typical pattern:

```python
loop.submit_input(text)
for msg in loop.take_last_turn_outputs():
    await ws.send_json(scene_ws_payload(msg))
```

## Callbacks

### PromptCallback

```cpp
std::function<std::pair<std::string, std::string>(
                          const std::vector<SceneMessage>&,
                          const Scene&,
                          const DirectorOutput&,
                          const std::string& director_focus_text,
                          const std::string& inner_states)>   // added 2026-06-06
```

Receives the history window, the scene object, the previous turn's Director output, the Director's focus payload text, and an `inner_states` block. Returns the `(system, user)` prompt pair.

The `inner_states` argument (added 2026-06-06) is built by `SceneLoop::advance` in Phase 1 via the `build_inner_states` helper: for each on-stage NPC it advances the character's persistent `self_state_` (`CharacterMemory::update_self_state`) and emits a first-person `### Inner states` block. Because there is **no separate Director LLM** — the merged narrator prompt is the actual decision-maker for `speech_turns` emotional_state/dramatic_intent — this is how character interiority reaches the decision. See [[memory-system]] (Self-state) and [[character-system]]. In practice, this calls `prompt.py:build_merged_prompt()` which concatenates system prompt, narrative frame, graph rules, speech rules, active characters, established facts, plot pressures, graph snapshot JSON, and conversation backlog.

### LLMCallback

```cpp
std::function<std::string(const std::string& prompt)>
```

Sends the prompt to the LLM and returns the raw response text. Uses the multi-provider `llm.py:complete()` abstraction (Gemini or DeepSeek).

### TurnCompleteCallback

```cpp
std::function<void(const SceneMessage& assistant_msg)>
```

Fires after each assistant message is appended — once for the narrator message, then once for each character dialogue line.

### CharacterSynthCallback

```cpp
std::function<std::vector<std::string>(const std::vector<std::pair<std::string, std::string>>& cues,
                                       const std::string& narration_prose)>
```

Receives `(character_name, cue)` pairs and the narrator prose for context. Returns one spoken line per cue, in order. Uses the local llama.cpp server via `character_agent.py:make_character_synth()`.

## Wiring in Python

The server wires the loop in `app.py:_wire_loop()`:

```python
loop = SceneLoop()
loop.load_scene(scene)
loop.set_director(director)
loop.set_character_synth_callback(make_character_synth(scene))
loop.set_prompt_callback(
    lambda hist, scene_obj, director_out, focus_json: build_merged_prompt(
        hist, scene_obj, director_out,
        director_focus_json=focus_json,
        established_facts=_established_facts(memory, hist, scene_obj, director_out),
        active_characters=_active_characters(scene_obj),
    )
)
loop.set_llm_callback(_call_llm)
```

`submit_input()` is synchronous C++, so the FastAPI server wraps it in `asyncio.run_in_executor()`. See [[callback-vs-pull]] for the rationale.

## Error recovery

If a turn fails — exception from LLM callback, JSON parse error — the server catches the exception and sends an error message to the client. It then reconstructs the SceneLoop from the scene state. The scene, Director, and WorldGraph remain valid; only the loop needs rewiring.

## Design notes

- `submit_input()` drives the entire turn synchronously through all stages. The loop is self-contained — no polling, no coroutine splitting.
- The loop does not own the Scene or Director. It holds pointers set by the caller. Ownership stays with the server session.
- The merged-prompt design reduces LLM calls from two per turn (Director + narrator) to one, at the cost of requiring the LLM to produce both prose and structured JSON in a single response.
