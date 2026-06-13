---
sources:
  - core/include/rhapsode/scene.h
  - core/include/rhapsode/director.h
  - core/include/rhapsode/scene_loop.h
  - core/include/rhapsode/memory_system.h
  - server/rhapsode/app.py
last_updated: 2026-05-17
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
- `history` — ordered conversation messages with timestamps and metadata
- `world_graph` — directed graph of plot nodes with typed edges (Boost.Graph)
│                                                             │
│  app.py ── WebSocket endpoint                               │
│    ├── gemini.py ── Gemini LLM client                       │
Scene supports save/load to a `saves/` directory. A save persists the world graph (nodes + edges + `next_id`), full history, `turn_index`, and `memory_next_id`. Loading a save restores game state exactly.
│    ├── memory.py ── Chroma + embeddings callbacks           │
│    ├── validator.py ── local llama.cpp client                │
│    └── lemmatization.py ── spaCy BM25 lemmas                │
The Director operates on the WorldGraph each turn. In the current merged-prompt architecture, the Director does **not** make its own LLM call. Instead:
│  Registers Python callbacks on C++ objects via pybind11     │
1. **`focus_payload_json()`** builds a JSON context blob from all non-resolved nodes plus scene context, including a 2-hop BFS neighborhood from entity-matched seed nodes.
│    ├── Scene (state: History + NodePool + Characters)        │
2. This JSON is embedded in the merged narrator prompt alongside narrative instructions, graph rules, and speech cue rules.
│    └── MemorySystem (hybrid retrieval, quality pipeline)     │
3. The LLM returns prose + `<<<RHAPSODE_JSON>>>` + structured JSON.
│  All game state serialized as JSON                          │
4. **`apply_planned_turn()`** processes the JSON:
   - **Applies transitions** — updates node states along dormant → foreshadowed → active → resolved
   - **Applies new nodes** — adds LLM-generated nodes, auto-links them to existing nodes sharing entities via `Related` edges
   - **Enforces invariants** — auto-resolves nodes that are superseded (same type+entity, different fact) or contradicted (terminal facts like death)
   - **Collects context blocks** — gathers `foreshadow_ctx` and `active_ctx` strings for the next turn's prompt

The Director's prompt enforces strict fact format: one atomic assertion per node, max 15 words, no hedging, no compound sentences, at least one named entity. Node types are `plot`, `scene`, `world`, `relationship`.

See [plot-graph](plot-graph.md) for the WorldGraph and Director details.
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

**Post-turn pipeline:** After each turn, newly created and resolved nodes are processed:

- `process_new_nodes()` — distill, quality-score, extract entities, conflict-detect, and store new facts
- `sync_resolved()` — store resolved node facts as established memories for future retrieval

The local LLM (llama.cpp on port 8012) handles the heavy processing steps. If unreachable, the pipeline degrades gracefully — facts are stored without enrichment.

See [memory system](memory-system.md) for the full breakdown.

### 4. SceneLoop — turn FSM

The SceneLoop is a finite state machine driving the turn cycle with a **single merged LLM call** per turn:

```
Idle → WaitingForInput → ProcessingInput → BuildingPrompt → RunningLLM → AppendingResult → WaitingForInput
```

Each turn:

1. Player input arrives via `submit_input(text)`
2. Director provides `focus_payload_json()` — the graph context for the merged prompt
3. Prompt callback assembles the full merged prompt: system prompt, narrative frame, graph rules, speech rules, active characters, established facts, plot pressures, graph snapshot, and history window
4. LLM callback sends the prompt and returns a single response containing prose + JSON
5. Response is split at `<<<RHAPSODE_JSON>>>` into prose and structured plan
6. Director applies graph updates from the JSON plan
7. Narrator message appended to history
8. Character synthesis callback generates NPC dialogue from speech cues in the JSON
9. Character dialogue messages appended to history
10. All turn outputs collected for WebSocket delivery

The loop supports configurable history windows — by default 8 messages, 12 on resume — and a resume flag for session restoration.

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
    ├── Director.focus_payload_json(turn_index, scene_context)
    →     ├── serialize non-resolved nodes
    →     ├── entity-match seed nodes → BFS 2-hop context
    →     └── return JSON blob
    → +    ├── Prompt callback → build_merged_prompt
    →     ├── system_prompt + narrative_frame + graph_rules + speech_rules
    →     ├── active characters (from WorldGraph active nodes)
    →     ├── established facts (from memory.retrieve_for_injection)
    →     ├── active plot pressures (Director context_blocks)
    →     ├── graph snapshot JSON (Director focus payload)
    →     └── conversation backlog (history window)
    → +    ├── LLM callback → Gemini/DeepSeek (single merged prompt)
    →     └── returns prose + <<<RHAPSODE_JSON>>> + JSON
    → +    ├── Split response → prose chunk + turn plan JSON
    → +    ├── Director.apply_planned_turn(turn_index, turn_plan)
    →     ├── apply transitions + new nodes + auto-link edges
    →     ├── enforce invariants (auto-resolve superseded/contradicted)
    →     └── collect context blocks for next turn
    │     ├── parse JSON → apply transitions + new nodes
    ├── Append narrator SceneMessage (scene_kind: "narrator")
    │
    ├── Character synthesis → local llama.cpp
    →     ├── for each speech_turn cue: generate in-character line
    →     └── append character SceneMessages (scene_kind: "character", speaker: name)
    │     └── system_prompt + NPC names + director context blocks + recent messages
    └── take_last_turn_outputs() → all messages from this turn
    │
    Post-turn: memory.sync_resolved(output.newly_resolved, turn_index)
    ├── LLM callback → Gemini (narrative prompt)
    │     └── returns assistant prose
    WebSocket pushes each output as { type: "scene_message", content, scene_kind, speaker? }
    │
    └── Append assistant message → turn_complete callback
         │
         ▼
1. **One LLM call per turn.** The merged prompt produces both narrative prose and graph instructions in a single response. The Director provides context but does not call the LLM itself.
2. **Memory pipeline is async-safe.** The post-turn `process_new_nodes` and `sync_resolved` run after the turn completes and the response is sent. If they fail, the turn still succeeds.
3. **Scene is serializable.** `Scene::save()` / `load_save()` round-trips cleanly through JSON, including the WorldGraph with edges. Game state persists across server restarts.
         ▼
5. **Graceful degradation.** If the local LLM is unreachable, memory pipeline steps return empty strings and facts are stored without enrichment. If Chroma is empty, retrieval returns nothing. If character synth fails, fallback placeholder text is used.
```

## Engineering constraints

1. **Two LLM calls per turn.** The Director calls the LLM once (JSON node management). The narrative callback calls it once (prose generation). Both use Gemini via the same client.
2. **Memory pipeline is async-safe.** The post-turn `process_new_nodes` runs after the turn completes and the response is sent. If it fails, the turn still succeeds.
| Scene (with save/load) | Done | Persists world_graph (nodes + edges), history, turn_index |
| Node, WorldGraph | Done | Directed graph with typed edges (Related, Supersedes, Contradicts, CausedBy), BFS traversal |
| Director | Done | Focus payload + apply_planned_turn; auto-link edges; invariant enforcement |
| SceneLoop | Done | Merged prompt, single LLM call, character synth, output collection |
| MemorySystem | Done | Hybrid retrieval, quality pipeline, MD5 dedupe, sync_resolved |
| FastAPI + WebSocket | Done | Single-endpoint, scene_message protocol |
| LLM abstraction | Done | Multi-provider (Gemini, DeepSeek) via llm.py |
| Merged prompt | Done | Narrator + graph + speech cues in one prompt |
| Character synthesis | Done | Local llama.cpp NPC dialogue from speech cues |
|-----------|--------|-------|
| SceneMessage, History, Character | Done | JSON round-trip, pybind11 exposed |
| Vue 3 frontend | Done | Panel-based layout, story/status/conversation panels, scene text parser |
| **Trigger predicates** | **Planned** | Edge triggers for deterministic transitions |
| **Session (multi-scene)** | **Planned** | Shared WorldGraph across concurrent SceneLoops |
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

The full vision introduces a **Session** layer that owns the WorldGraph, GitStore, and Director. Multiple SceneLoops share one Session. Each loop has a `resolution` parameter: resolution 1 = interactive (every turn), resolution N = background world scene (every N turns).

The WorldGraph gains trigger predicates on edges. The Director traverses edges deterministically each turn — checking which triggers are satisfied and firing transitions without an LLM call. The LLM would only be involved in generation (creating new subgraphs) and prose rendering.

See [plot graph](plot-graph.md) for the full planned design.

## References

- [[concepts/narrative-philosophy|narrative philosophy]] — the six design principles
- [plot graph](plot-graph.md) — WorldGraph, Director, and planned features
- [scene loop](scene-loop.md) — the C++ FSM
- [memory system](memory-system.md) — hybrid retrieval and quality pipeline
- [[research/literature-review|literature review]] — 8 papers surveyed
