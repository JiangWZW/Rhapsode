---
sources:
  - core/include/rhapsode/scene.h
  - core/include/rhapsode/director.h
  - core/include/rhapsode/scene_loop.h
  - core/include/rhapsode/memory_system.h
  - server/rhapsode/app.py
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[concepts/narrative-philosophy]]"
  - "[[architecture/stack]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/memory-system]]"
  - "[[architecture/python-server]]"
  - "[[architecture/mvp-v0]]"
tags:
  - cross-layer
---

# System overview

Engineering summary of the Rhapsode architecture. This page translates the [[concepts/narrative-philosophy|narrative philosophy]] into concrete subsystems, data structures, and control flows.

The system has two layers: **what is built** (the working engine) and **what is planned** (the full vision). This page covers both, clearly marked.

## Architecture diagram (current)

```
┌─────────────────────────────────────────────────────────────┐
│  Frontend (Vue 3)                                           │
│  ChatView → MessageList + InputBar                          │
│  WebSocket store (Pinia)                                    │
└──────────────────────────┬──────────────────────────────────┘
                           │ WebSocket /ws
┌──────────────────────────▼──────────────────────────────────┐
│  Server (FastAPI)                                           │
│                                                             │
│  app.py ── WebSocket endpoint                               │
│    ├── gemini.py ── Gemini LLM client                       │
│    ├── prompt.py ── prompt builder                          │
│    ├── memory.py ── Chroma + embeddings callbacks           │
│    ├── validator.py ── local llama.cpp client                │
│    └── lemmatization.py ── spaCy BM25 lemmas                │
│                                                             │
│  Registers Python callbacks on C++ objects via pybind11     │
└──────────────────────────┬──────────────────────────────────┘
                           │ pybind11 (_core.pyd)
┌──────────────────────────▼──────────────────────────────────┐
│  Core (C++17)                                               │
│                                                             │
│  SceneLoop (FSM)                                            │
│    ├── Scene (state: History + NodePool + Characters)        │
│    ├── Director (LLM callback → node transitions + new)     │
│    └── MemorySystem (hybrid retrieval, quality pipeline)     │
│                                                             │
│  All game state serialized as JSON                          │
└─────────────────────────────────────────────────────────────┘
```

## Subsystems

### 1. Scene — game state container

`Scene` holds all per-game state:

- `scene_id`, `title`, `system_prompt` — static scenario data
- `characters` — NPCs and the player
- `history` — ordered conversation messages with timestamps
- `node_pool` — all plot nodes: dormant, foreshadowed, active, resolved
- `turn_index` — current turn counter
- Optional `MemorySystem*` reference

Scene supports save/load to a `saves/` directory. A save persists the node pool (including `next_id`), full history, `turn_index`, and `memory_next_id`. Loading a save restores game state exactly.

### 2. Director — narrative arranger

The Director operates on the NodePool each turn. Its `tick()` method:

1. **Builds a JSON prompt** from all non-resolved nodes plus scene context
2. **Calls the LLM** (via callback) requesting transitions and new nodes
3. **Applies transitions** — updates node states along dormant → foreshadowed → active → resolved
4. **Applies new nodes** — adds LLM-generated nodes to the pool
5. **Removes resolved nodes** from the pool (keeps the active set small)
6. **Collects context blocks** — gathers `foreshadow_ctx` and `active_ctx` strings from foreshadowed/active nodes for the narrative prompt

The Director uses a retrieval callback to fetch established facts from the memory system and inject them as constraints into its own LLM prompt. This prevents contradictions.

The Director's LLM prompt enforces strict fact format: one atomic assertion per node, max 15 words, no hedging, no compound sentences, at least one named entity. Node types are `plot`, `scene`, `world`, `relationship`.

See [cpp-data-model](cpp-data-model.md) for the `Director` and `DirectorOutput` structures.

### 3. MemorySystem — emotional backbone

The MemorySystem is implemented as a C++ class with Python callbacks for external services: embedding, vector storage, and local LLM. This split keeps scoring logic in C++ while storage backends stay in Python.

**Storage:** ChromaDB persistent collections per scene, named `{scene_id}_facts` and `{scene_id}_entities`, with BAAI/bge-base-en-v1.5 sentence embeddings.

**Retrieval:** Hybrid three-signal approach:

| Signal | Mechanism |
|--------|-----------|
| Semantic similarity | Cosine distance via Chroma query |
| BM25 keyword matching | Okapi BM25 on spaCy-lemmatized tokens (C++ scoring) |
| Entity boosting | Facts linked to entities mentioned in the query rank higher |

**Post-turn pipeline** (`process_new_nodes`): After each turn, newly resolved and created nodes are processed:

1. **Distill** — verbose facts are shortened via local LLM
2. **Quality scoring** — batch score new nodes against existing pool via local LLM
3. **Entity extraction** — extract entity names via local LLM
4. **Conflict detection** — check if a new fact contradicts an existing one (semantic proximity check)
5. **Store** — embed and persist to Chroma with metadata

The local LLM (llama.cpp on port 8012) handles the heavy processing steps. If unreachable, the pipeline degrades gracefully — facts are stored without enrichment.

See [memory system](memory-system.md) for the full breakdown.

### 4. SceneLoop — turn FSM

The SceneLoop is a finite state machine driving the turn cycle:

```
Idle → WaitingForInput → ProcessingInput → BuildingPrompt → RunningLLM → AppendingResult → WaitingForInput
```

Each turn:

1. Player input arrives via `submit_input(text)`
2. Director ticks — evaluates nodes, calls the LLM, returns context blocks
3. Prompt callback assembles the full prompt from system prompt, NPC characterization, director context blocks, and the history window
4. LLM callback sends the prompt to Gemini and returns the response
5. Assistant message is appended to history
6. Turn-complete callback notifies the server

The loop supports configurable history windows — by default three messages, ten on resume — and a resume flag for session restoration.

See [scene loop](scene-loop.md) for the full FSM and callback details.

## Control flow per turn

```
Player types message
    │
    ▼
WebSocket receives JSON { type: "player_message", content: "..." }
    │
    ▼
asyncio.run_in_executor → SceneLoop.submit_input(text)
    │
    ├── Director.tick(turn_index, scene_context)
    │     ├── build_prompt(non-resolved nodes + context)
    │     ├── retrieval_callback → memory.retrieve_for_injection()
    │     ├── LLM callback → Gemini — director system prompt + established facts
    │     ├── parse JSON → apply transitions + new nodes
    │     └── collect context_blocks from foreshadowed/active nodes
    │
    ├── Prompt callback → build_prompt — history_window, scene, director_output
    │     └── system_prompt + NPC names + director context blocks + recent messages
    │
    ├── LLM callback → Gemini (narrative prompt)
    │     └── returns assistant prose
    │
    └── Append assistant message → turn_complete callback
         │
         ▼
    Post-turn: memory.process_new_nodes(output.new_nodes, turn_index)
    Post-turn: scene.save(saves_dir)
         │
         ▼
    WebSocket pushes { type: "assistant_message", content: "..." }
```

## Engineering constraints

1. **Two LLM calls per turn.** The Director calls the LLM once (JSON node management). The narrative callback calls it once (prose generation). Both use Gemini via the same client.
2. **Memory pipeline is async-safe.** The post-turn `process_new_nodes` runs after the turn completes and the response is sent. If it fails, the turn still succeeds.
3. **Scene is serializable.** `Scene::save()` / `load_save()` round-trips cleanly through JSON. Game state persists across server restarts.
4. **Callbacks cross the language boundary cleanly.** C++ owns the control flow; Python provides I/O via LLM HTTP, embeddings, and vector storage. No business logic duplicated.
5. **Graceful degradation.** If the local LLM is unreachable, memory pipeline steps return empty strings and facts are stored without enrichment. If Chroma is empty, retrieval returns nothing and the Director operates without established facts.

## Implementation status

| Component | Status | Notes |
|-----------|--------|-------|
| SceneMessage, History, Character | Done | JSON round-trip, pybind11 exposed |
| Scene (with save/load) | Done | Persists node_pool, history, turn_index |
| Node, NodePool | Done | Flat pool with state-based indexing, no edges |
| Director | Done | LLM-driven transitions + new node creation |
| SceneLoop | Done | Director integration, history windows, resume |
| MemorySystem | Done | Hybrid retrieval, quality pipeline, MD5 dedupe |
| FastAPI + WebSocket | Done | Single-endpoint architecture |
| Gemini client | Done | google-genai SDK, configurable model/base URL |
| Chroma + embeddings | Done | BAAI/bge-base-en-v1.5, persistent per-scene |
| Local LLM (validator) | Done | llama.cpp HTTP, graceful fallback |
| Vue 3 frontend | Done | Chat view, WebSocket store |
| **Plot graph edges + triggers** | **Planned** | DAG structure, predicate-based triggers |
| **Session (multi-scene)** | **Planned** | Shared PlotGraph across concurrent SceneLoops |
| **GitStore** | **Planned** | libgit2-backed graph history |
| **World-background loop** | **Planned** | Off-screen NPC events |
| **Input mode spectrum** | **Planned** | Constrained choices at graph nodes |
| **Per-NPC memory** | **Planned** | Subjective memory slices per character |
| **Visual editor** | **Planned** | Plot graph inspection/editing UI |

## Planned architecture (future)

The full vision introduces a **Session** layer that owns the PlotGraph, GitStore, and Director. Multiple SceneLoops share one Session. Each loop has a `resolution` parameter: resolution 1 = interactive (every turn), resolution N = background world scene (every N turns).

The NodePool evolves into a full directed acyclic graph with typed edges. Trigger predicates fire on player action tags, turn counts, world conditions, or other node states. The Director traverses edges deterministically — no LLM needed for structure.

See [plot graph](plot-graph.md) for the full DAG design and the generation pipeline vision.

## References

- [[concepts/narrative-philosophy|narrative philosophy]] — the six design principles
- [plot graph](plot-graph.md) — node/pool design and DAG vision
- [scene loop](scene-loop.md) — the C++ FSM
- [memory system](memory-system.md) — hybrid retrieval and quality pipeline
- [[research/literature-review|literature review]] — 8 papers surveyed
