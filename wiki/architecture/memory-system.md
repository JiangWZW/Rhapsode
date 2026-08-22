---
sources:
  - core/include/rhapsode/memory_system.h
  - core/src/memory_system.cpp
  - core/include/rhapsode/character_memory.h
  - core/src/character_memory.cpp
  - server/rhapsode/memory.py
  - server/rhapsode/validator.py
last_updated: 2026-05-23
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/python-server]]"
  - "[[architecture/companion-system]]"
  - "[[research/memory-systems-survey]]"
  - "[[research/generative-agents-code-analysis]]"
tags:
  - cpp-core
  - python-server
  - cross-layer
---

# Memory system

Rhapsode has two complementary memory subsystems:

1. **MemorySystem** — scene-scoped semantic index of WorldGraph nodes in ChromaDB. Provides similarity-based node retrieval for the Director and Weaver.
2. **CharacterMemory** — per-character subjective memory with Generative Agents-style observe-reflect-plan cycle. Maintains a belief graph, importance-scored observations, and multi-level reflections.

Both share the same Python callback infrastructure (embedding model, ChromaDB client) but serve different purposes.

## Architecture split

| Concern | Language | Why |
|---------|----------|-----|
| Node storage/retrieval logic | C++ (`MemorySystem`) | Deterministic, no I/O |
| Belief graph, reflection, retrieval scoring | C++ (`CharacterMemory`) | Core cognitive logic, deterministic ranking |
| Embedding (BAAI/bge-base-en-v1.5) | Python | sentence-transformers is Python-native |
| Vector storage (ChromaDB) | Python | Chroma is Python-native |
| Local LLM calls (llama.cpp) | Python | HTTP client to localhost:8012 |

C++ owns both `MemorySystem` and `CharacterMemory` classes. Python registers callbacks at startup via `memory.py`.

## Callback signatures

```cpp
// Shared across both systems
using EmbedCallback      = std::function<std::string(const std::string& text)>;
using StoreCallback      = std::function<void(const std::string& collection,
                                              const std::string& id,
                                              const std::string& doc,
                                              const std::string& embedding_json,
                                              const std::string& metadata_json)>;
using QueryCallback      = std::function<std::string(const std::string& collection,
                                                     const std::string& embedding_json,
                                                     int n,
                                                     const std::string& where_json)>;

// MemorySystem-specific
using UpdateMetaCallback = std::function<void(const std::string& collection,
                                              const std::string& id,
                                              const std::string& metadata_json)>;
using GetByMetaCallback  = std::function<std::string(const std::string& collection,
                                                     const std::string& where_json)>;
using DeleteCallback     = std::function<void(const std::string& collection,
                                              const std::string& ids_json)>;
using LocalLLMCallback   = std::function<std::string(const std::string& prompt)>;

// CharacterMemory-specific
using ReflectionLLMCallback = std::function<std::string(const std::string& prompt)>;
```

## Python callback registration

`server/rhapsode/memory.py` provides two registration functions:

| Function | Target | Callbacks registered |
|----------|--------|---------------------|
| `register_callbacks(memory_system, scene_id)` | `MemorySystem` | embed, store, query, update_meta, get_by_meta, delete |
| `register_character_memory_callbacks(char_mem)` | `CharacterMemory` | embed, store, query |

Both share a single `SentenceTransformer` model and `chromadb.PersistentClient`. Collections are created lazily with cosine distance via `get_or_create_collection`.

The `ReflectionLLMCallback` for `CharacterMemory` is registered separately (typically using the same local LLM callback from `validator.py`).

---

## Part 1: MemorySystem (scene-scoped node index)

### Storage layout

Each scene gets a single ChromaDB collection:

| Collection | Contents |
|------------|----------|
| `{scene_id}_nodes` | Embedded facts from WorldGraph nodes. Metadata: `node_id`, `state`, `type`, `created_at` |

Embeddings use **BAAI/bge-base-en-v1.5** (768-dimensional). The model and the Chroma
persistent client are opened once at server startup (`warmup_model()`, `warmup_chroma()`).

### Store

`store_node(node_id, fact, state, type, turn)` embeds the fact text and stores it with metadata. Document ID format: `"node_{node_id}"`. Uses upsert semantics (same node re-stored overwrites).

### Search

`search_nodes(query, top_k)` embeds the query, queries ChromaDB with a `$ne: "dormant"` filter (dormant nodes are excluded), and returns matching node IDs ranked by cosine similarity.

### Delete

`delete_nodes(node_ids)` removes the specified nodes from ChromaDB by document ID.

### Post-turn pipeline

After each turn, two methods run:

- `process_new_nodes(nodes, turn)` — stores each new node (skipping nodes with `id == 0`).
- `sync_resolved(resolved_nodes, turn)` — updates metadata on resolved nodes (sets `state: "resolved"` and `resolved_at` timestamp) so they remain retrievable but are correctly labeled.

### Graceful degradation

| Condition | Behavior |
|-----------|----------|
| Embed callback available | Nodes stored and searchable |
| Embed or store callback missing | `store_node()` throws; system non-functional |
| Chroma empty | `search_nodes()` returns empty; Director operates without memory context |
| Local LLM unreachable | No impact on MemorySystem (LLM callback unused by current implementation) |

---

## Part 2: CharacterMemory (per-character cognitive layer)

Based on [Generative Agents](../research/papers/generative-agents.md) (Park et al., Stanford + Google, UIST 2023). Each character gets an independent memory instance stored in `Scene::character_memories`.

### Storage layout

Each character gets a ChromaDB collection: `charmem_{character_name}` (sanitized to alphanumeric + dots + hyphens).

Two types of documents are stored:
- **Observations**: `obs_{name}_{index}` — events the character witnessed
- **Beliefs**: `belief_{id}` — subjective beliefs and reflections

Both carry metadata: `type`, `turn`, `poignancy`, and (for beliefs) `belief_id`, `depth`, `source_node`.

### Belief graph

Internally, a Boost `adjacency_list<directedS, MemoryNode, EdgeData>` stores beliefs as vertices with weighted directed edges between related beliefs.

```cpp
struct MemoryNode {
    uint64_t id;
    std::string content;
    std::optional<uint64_t> source_node;  // link to WorldGraph node
    int created_at;                        // turn
    int poignancy;                         // importance (1-10)
    int depth;                             // 0 = base, 1+ = reflection
    std::string mem_type;                  // "belief", "observation", "reflection"
    std::vector<uint64_t> filling;         // evidence IDs for reflections
};
```

### Observation intake

`add_observation(text, turn)`:
1. Appends to the short-term context buffer
2. Scores importance (1-10) via the reflection LLM callback
3. Embeds and stores in ChromaDB with `type: "observation"` metadata
4. Decrements the importance accumulator (triggers meta-reflection when it crosses zero)

### Importance scoring

The LLM is prompted to rate event poignancy on a 1-10 scale specific to the character. On failure, defaults to 4. The score is used for both retrieval weighting and meta-reflection triggering.

### Retrieval

`retrieve_context(query, top_k)` implements three-signal composite scoring:

```
                ▼
        ┌── Distill verbose facts ──┐
Where:
- **Relevance** = normalized cosine distance (1.0 = closest, 0.0 = furthest in the result set)
- **Recency** = normalized turn distance (1.0 = most recent, 0.0 = oldest in the result set)
- **Importance** = poignancy / 10.0
        │   facts > threshold        │
Process:
1. Over-fetch from ChromaDB (2× `top_k`)
2. Normalize signals across the result set
3. Compute composite score per result
4. Sort descending, take top `top_k`
5. Append the last 3 observations as a short-term buffer (always visible regardless of scoring)
                    ▼
### Reflection
        │   Local LLM rates nodes   │
`process_reflection(dialogue, cue_json, narrator_beat, turn)`:
1. Builds a prompt presenting the character as their "inner mind"
2. Includes current beliefs, recent observations, and what just happened
3. Asks the LLM to produce JSON: `{ new_beliefs, updated_beliefs, observation }`
4. Applies the response: new beliefs are added to the graph, updated beliefs are modified in place, observations are stored
5. Calls `try_meta_reflection(turn)` afterwards
        └───────────┬───────────────┘
### Meta-reflection (importance-gated)
        ┌── Entity extraction ──────┐
Fires when `importance_trigger_curr_` drops below zero (accumulates from observation poignancy, resets at `importance_trigger_max_ = 150`). Multi-step:
        │   named entities          │
1. **Focal questions**: LLM generates 3 salient questions about what's happening in the character's life
2. **Evidence retrieval**: For each question, retrieves top-10 context
3. **Insight extraction**: LLM produces 3 high-level insights per question
4. **Storage**: Insights stored as `depth=1` reflection nodes with their own importance scores
                    ▼
This follows the Generative Agents architecture closely — higher-order reflections that compound over time.
        │   ├── Embed fact          │
### Self-state (persistent first-person) — added 2026-06-06
        │   │   — semantic proximity    │
Distinct from query-driven `retrieve()`/`briefing()`, each `CharacterMemory` carries a persistent
first-person inner monologue, `self_state_`:
        │   └── store_fact()        │
- `update_self_state(recent_events, turn)` folds the **previous** state forward with what just
  happened: `new_state = LLM(previous_state + recent_events)`. Because it does not depend on
  cue/semantic similarity, an emotional thread persists across topic shifts (sidestepping the
  retrieval-ratchet for the self-state specifically). No-op if no reflection LLM; never blanks an
  existing state on failure.
- `self_state()` / `set_self_state()` read/seed it. At scenario load, `Scene::load_json` seeds it
  from the authored first-person `initial_memory.context`.
- The retrieval/reflection prompts (`briefing`, `reflect`, `distill`, `score_importance`) are written
  in the **first person** so all memory text is in the character's own voice.
```
It is advanced once per on-stage NPC per turn in `SceneLoop::advance` (Phase 1, via
`build_inner_states`) and surfaced both in the decision prompt (`### Inner states`) and the actor
prompt (`Inner state` section). See [[scene-loop]] and [[character-system]].
Each step uses the local LLM callback at llama.cpp port 8012. If unreachable, each step returns an empty string and the pipeline continues with reduced quality. Facts are stored without distillation, scoring, or enriched entity links.
### Serialization
### Distill
`to_json()` / `from_json()` serialize the full belief graph (nodes + edges), context buffer, importance accumulator state, the `self_state` string, and ID counter. Stored as part of the Scene save file under `character_memories`.

---

### Entity extraction

The LLM identifies named entities from each fact: characters, locations, factions, objects. These populate the `entities` field and are linked in the entities collection for entity-boosted retrieval.

### Conflict detection

Before storing, each fact is embedded and compared to existing facts by semantic proximity. High-similarity matches are checked for contradiction. Detected conflicts can be flagged or resolved — current behavior: store anyway, log the conflict.

## Duplicate detection

Every fact is MD5-hashed before storage. If a fact with the same hash already exists in the collection, it is skipped. This prevents identical facts from accumulating across turns.

## Graceful degradation

The memory system is designed to work at multiple quality levels:

| Condition | Behavior |
|-----------|----------|
| Local LLM available | Full pipeline: distill, score, entity extract, conflict check |
| Local LLM unreachable | Facts stored raw, entities from node metadata only, no quality scoring |
| Chroma empty | Retrieval returns nothing; Director operates without established facts |
| spaCy unavailable | BM25 falls back to lowercased input tokens |

## Configuration

| Setting | Default | Source |
|---------|---------|--------|
| Embedding model | BAAI/bge-base-en-v1.5 | `memory.py:EMBEDDING_MODEL` |
| Chroma path | `./chroma` | `register_callbacks()` parameter |
| Local LLM URL | `http://127.0.0.1:8012` | `validator.py:LLAMA_URL` (env: `RHAPSODE_LOCAL_LLM_URL`) |
| Local LLM timeout | 120 seconds | `validator.py:LLAMA_TIMEOUT` (env: `RHAPSODE_LOCAL_LLM_TIMEOUT`) |
| MemorySystem search top_k | 10 | `search_nodes()` default parameter |
| CharacterMemory retrieve top_k | 5 | `retrieve_context()` default parameter |
| Retrieval weights | 0.5 / 3.0 / 2.0 | Hardcoded (recency / relevance / importance) |
| Meta-reflection threshold | 150 | `CharacterMemory::importance_trigger_max_` |
| Short-term context buffer | last 3 observations | Always appended to retrieval output |
