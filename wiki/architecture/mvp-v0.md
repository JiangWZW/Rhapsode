---
sources:
  - server/rhapsode/app.py
  - core/CMakeLists.txt
last_updated: 2026-05-12
confidence: verified
tier: procedural
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/stack]]"
tags:
  - cross-layer
---

# MVP v0 — status and retrospective

MVP v0 defined the minimum playable prototype. It has been **completed and exceeded** — the working system includes the Director, node pool, and memory system, all of which were post-MVP targets.

## Original acceptance criteria (all met)

1. C++ holds Scene, History (SceneMessage structs), characters; JSON serialize/deserialize.
2. SceneLoop (C++) drives turns: player input → prompt callback → LLM callback → append messages.
3. FastAPI + WebSocket — send player line, push assistant reply + status.
4. Vue 3 — chat view with message list, input bar, connection status.
5. Scenario JSON — load title, system prompt, characters, seed messages.

## What was built beyond MVP

| Addition | Description |
|----------|-------------|
| **Node / NodePool** | Plot nodes with states (Dormant → Foreshadowed → Active → Resolved) |
| **Director** | LLM-driven node management each turn (transitions + new nodes) |
| **MemorySystem** | Hybrid retrieval (semantic + BM25 + entity), quality pipeline via local LLM |
| **Save/load** | Scene persistence to `saves/` directory with full state round-trip |
| **Resume support** | SceneLoop detects saved state and adjusts history windowing |
| **Seed nodes** | Scenarios can include pre-authored plot nodes |
| **Local LLM integration** | llama.cpp on port 8012 for memory quality pipeline |
| **Established facts injection** | Director LLM prompt includes memory-retrieved facts to prevent contradictions |

## Deviations from the original plan

| Planned | Actual |
|---------|--------|
| `llm/` subpackage with BaseLLMClient ABC, Gemini + OpenAI clients | Single `gemini.py` module, no abstraction layer |
| `ws.py` WebSocket endpoint + `session.py` session manager | Everything in `app.py` — single-file server |
| OpenAI as secondary LLM client | Not implemented; Gemini only |
| hnswlib in C++ for memory | ChromaDB in Python (more practical for prototype) |
| `types/index.ts` in frontend | Types inline in `websocket.ts` |
| `ConnectionStatus.vue` component | Inline in `ChatView.vue` header |
| Reconnect logic in WebSocket store | Not implemented |

## Scenario JSON schema (current)

```json
{
  "title": "The Dusty Flagon",
  "system_prompt": "You are the narrator of a fantasy RPG...",
  "characters": [
    { "name": "Player", "description": "A wandering adventurer", "is_player": true },
    { "name": "Barkeep", "description": "A gruff dwarf who runs the tavern", "is_player": false }
  ],
  "seed_messages": [
    { "role": "assistant", "content": "You push open the heavy oak door..." }
  ],
  "nodes": [
    {
      "fact": "The barkeep owes money to the thieves guild.",
      "type": "plot",
      "state": "Foreshadowed",
      "foreshadow_ctx": "The barkeep keeps glancing nervously at the door.",
      "active_ctx": "The barkeep is frightened; debt collectors are expected soon.",
      "entities": ["barkeep", "guild"],
      "known_by": ["barkeep"]
    }
  ]
}
```

The `nodes` array was added post-MVP. Each node seeds the NodePool at scenario load.

## End-to-end turn flow (current)

```
Vue → WebSocket → FastAPI → run_in_executor →
  SceneLoop.submit_input(text)
    ├── Director.tick() → Gemini (node management JSON)
    ├── prompt_callback → build_prompt(system + NPCs + director ctx + history)
    ├── llm_callback → Gemini (narrative prose)
    └── turn_complete → capture response
  Post-turn: memory.process_new_nodes() → local LLM + Chroma
  Post-turn: scene.save()
← WebSocket ← assistant_message
```

## Next targets

See [system overview — implementation status](system-overview.md) for the full roadmap. The immediate next targets are:

1. **Plot graph edges + trigger predicates** — transform the flat NodePool into a DAG
2. **Session layer** — shared graph across multiple concurrent SceneLoops
3. **World-background loop** — off-screen NPC events
