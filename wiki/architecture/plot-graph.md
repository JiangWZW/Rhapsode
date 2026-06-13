---
sources:
  - core/include/rhapsode/node.h
  - core/include/rhapsode/world_graph.h
  - core/src/world_graph.cpp
  - core/include/rhapsode/character_memory.h
  - core/src/character_memory.cpp
  - core/include/rhapsode/director.h
  - core/src/director.cpp
  - core/include/rhapsode/weaver.h
  - core/src/weaver.cpp
last_updated: 2026-05-20
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/memory-system]]"
  - "[[concepts/narrative-philosophy]]"
  - "[[research/literature-review]]"
tags:
  - cpp-core
  - design
---

# Plot graph & unified memory architecture

The plot graph is Rhapsode's core narrative data structure. It tracks **latent facts about the world** that the Director manages across turns. Characters maintain their own subjective **CharacterMemory** — a per-agent belief graph inspired by Generative Agents.

## Node

A node is an atomic world fact. The Director transitions it through states based on LLM analysis.

### Node structure

```cpp
enum class NodeState { Dormant, Foreshadowed, Active, Resolved };

struct Node {
    uint64_t id = 0;
    std::string fact;              // atomic assertion, ~15 words max
    std::string type;              // "plot", "scene", "world", "relationship"
    NodeState state = NodeState::Dormant;
    std::string foreshadow_ctx;    // hint for prompt when foreshadowed
    std::string active_ctx;        // directive for prompt when active
    std::vector<std::string> entities;
    std::string trigger;           // F-T-P trigger condition
    std::string arc_position;      // narrative arc position
    std::vector<uint64_t> related_to;
    int created_at = 0;
    int resolved_at = -1;
};
```

### Node states

| State | Meaning | Prompt effect |
|-------|---------|---------------|
| **Dormant** | Known to the system, not yet surfaced | None — invisible to narrative |
| **Foreshadowed** | Hints being seeded | `foreshadow_ctx` injected as subtle context |
| **Active** | Directly relevant to current scene | `active_ctx` injected as explicit directive |
| **Resolved** | Completed, consequences applied | Excluded from active graph |

### Foreshadow-Trigger-Payoff (F-T-P)

Each node maps to an (F, T, P) triple. The `trigger` field captures the activation condition; `arc_position` tracks narrative progress:

| State | F-T-P status |
|-------|-------------|
| Dormant | F registered, T not checked, P suppressed |
| Foreshadowed | F injected as hint, T monitored, P suppressed |
| Active | T satisfied, P injected as constraint |
| Resolved | P realized, triple removed from active set |

## WorldGraph — Humean edges

The WorldGraph is a **Boost.Graph directed graph** with simplified, typeless ("Humean") edges. Edge meaning is derived from node content and timestamps rather than explicit type labels.

```cpp
struct EdgeData {
    float weight = 1.0f;
    int created_at = 0;
    bool active = true;
};
```

Direction is enforced temporally: the older node (by `created_at`, with ID as tiebreaker) is always the source. This makes the graph a DAG by construction. Only one edge is allowed between any pair of nodes.

### API

```cpp
class WorldGraph {
public:
    Node& add_node(Node node);
    Node* get_node(uint64_t node_id);
    bool mark_resolved(uint64_t node_id, int resolved_at);
    size_t size() const;
    std::vector<Node> all_nodes(bool include_resolved = false) const;

    bool add_relation(uint64_t from_id, uint64_t to_id,
                      float weight = 1.0f, int created_at = 0);
    std::vector<uint64_t> neighbors(uint64_t node_id) const;
    std::vector<uint64_t> neighbors_within(uint64_t source_id, int max_hops,
                                           bool active_only = true) const;

    // Derived plot threads via connected components
    std::vector<uint64_t> thread_containing(uint64_t seed_id) const;
    std::vector<std::vector<uint64_t>> all_threads() const;

    nlohmann::json to_json() const;
    static WorldGraph from_json(const nlohmann::json& j);
};
```

### Thread extraction

`thread_containing()` and `all_threads()` use `boost::connected_components` on an undirected view of active edges. Each connected component is a plot thread — a group of related facts that form a narrative strand.

### Serialization

```json
{
  "next_id": 7,
  "nodes": [{ "id": 1, "fact": "barkeep owes thieves guild 200g", "state": "foreshadowed", ... }],
  "edges": [{ "from": 1, "to": 3, "weight": 1.0, "created_at": 2, "active": true }]
}
```

## MemorySystem — ChromaDB as semantic index

The `MemorySystem` is now a thin semantic index over WorldGraph nodes:

- **`store_node(node_id, fact, state, type, turn)`** — embeds and stores in ChromaDB with `node_id` in metadata
- **`search_nodes(query, top_k)`** — semantic search returning WorldGraph node IDs
- **`process_new_nodes()`** / **`sync_resolved()`** — bulk operations for the turn pipeline

The heavy pipeline (BM25 re-ranking, quality scoring, entity extraction, conflict detection) has been removed. ChromaDB is purely a retrieval index; the WorldGraph is the source of truth.

## CharacterMemory — per-agent subjective memory

Inspired by [Generative Agents](https://arxiv.org/abs/2304.03442), each NPC has its own `CharacterMemory` containing subjective beliefs that may differ from objective world facts.

### Architecture

```
CharacterMemory
├── Belief graph (boost::adjacency_list<MemoryNode, EdgeData>)
├── Text context buffer (std::vector<std::string>)
└── ChromaDB index (per-character namespace)
```

### MemoryNode

```cpp
struct MemoryNode {
    uint64_t id = 0;
    std::string content;                   // subjective belief text
    std::optional<uint64_t> source_node;   // WorldGraph ref, if any
    int created_at = 0;
};
```

A `MemoryNode` with `source_node` tracks the WorldGraph fact it derives from, but the `content` is subjective — the character's interpretation, which may be wrong or biased.

### API (all logic in C++)

```cpp
class CharacterMemory {
public:
    // Belief graph
    MemoryNode& add_belief(MemoryNode node);
    MemoryNode* find_by_source(uint64_t source_node_id);

    // Observations
    void add_observation(const std::string& text, int turn);

    // Retrieval (C++ logic, ChromaDB via callback)
    std::string retrieve_context(const std::string& query, int top_k = 5) const;

    // Prompt building (all in C++)
    std::string build_actor_prompt(const Character& character,
                                   const std::string& cue_json,
                                   const std::string& narrator_beat,
                                   const std::string& scene_history) const;

    // Reflection (C++ logic, LLM via callback)
    void process_reflection(const std::string& dialogue,
                            const std::string& cue_json,
                            const std::string& narrator_beat, int turn);

    // Callbacks (set from Python)
    void set_embed_callback(EmbedCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_reflection_llm_callback(ReflectionLLMCallback cb);
};
```

### Reflection cycle

After each dialogue turn:

1. C++ builds a reflection prompt with current beliefs, recent observations, and what just happened
2. The LLM (via callback) returns `{new_beliefs, updated_beliefs, observation}`
3. C++ applies the response: adds/updates beliefs in the graph and indexes them in ChromaDB

### Scenario bootstrapping

Characters in scenario JSON can define `initial_memory`:

```json
{
  "name": "Barkeep",
  "role": "major_npc",
  "initial_memory": {
    "beliefs": [
      {"content": "I owe the guild a dangerous sum", "source_fact": "The barkeep owes money to the thieves guild."}
    ],
    "context": ["Another slow evening at the Flagon."]
  }
}
```

On `Scene::load_json()`, beliefs are matched to WorldGraph nodes by `source_fact` text.

## Director

The Director operates on the WorldGraph each turn, providing context for the narrator prompt and applying the LLM's graph updates.

### Per-turn flow

1. **`focus_payload_json()`** — builds JSON with all non-resolved nodes + 2-hop BFS context
2. Narrator LLM returns prose + `<<<RHAPSODE_JSON>>>` + graph instructions
3. **`apply_planned_turn()`** — applies transitions, new nodes, invariants

### Invariant enforcement

- **Terminal facts** (death, destruction) auto-resolve related nodes sharing entities
- **Superseding** — new Active node with same type/entities resolves the old one

### Python wiring

Python is a thin orchestration layer:

- `app.py` registers ChromaDB/LLM callbacks on `MemorySystem` and `CharacterMemory`
- `character_agent.py` calls `CharacterMemory.build_actor_prompt()` and `process_reflection()`
- Minor NPCs without persistent memory fall back to a stateless prompt

## Weaver — LLM edge audit

The Weaver is a periodic maintenance component that audits and repairs WorldGraph edges using the cloud LLM. It addresses three classes of defects created by the Director's `shares_entity()` heuristic:

| Defect | Cause | Weaver fix |
|--------|-------|------------|
| **Missing connections** | Narratively related nodes have different entity strings | `connect` — add edge with appropriate weight |
| **Wrong connections** | Coincidental entity overlap (e.g. shared location name) | `disconnect` — deactivate the spurious edge |
| **Flat weights** | All auto-generated edges default to 1.0 | `reweight` — assign meaningful weight based on narrative strength |

### Why cloud LLM

The Weaver's task requires narrative reasoning — understanding thematic causality, character arcs, and distinguishing meaningful from coincidental relationships. Local 8B models handle JSON extraction well but lack the reasoning depth for reliable disconnect/reweight decisions. The cloud LLM (Gemini 2.0 Flash) runs every 3 turns to amortize cost.

### Turn pipeline placement

The Weaver runs as **Phase 0** in `SceneLoop::advance()`, before the Director builds its prompt. This ensures the Director sees the cleaned graph.

```
Phase 0: Weave (if turn % interval == 0)  → cloud LLM
Phase 1: Build merged Director+Narrator prompt
Phase 2: Call narrative LLM
Phase 3: Apply graph mutations (Director)
Phase 4: Actor synthesis
```

### Interval

Default: every 3 turns (configurable via `Weaver::set_interval()`). The Weaver fires on turns 0, 3, 6, 9, ... Skips graphs with fewer than 2 live nodes.

### Prompt design

The prompt presents all live nodes with their current active edges and asks the LLM to output a JSON object with three arrays:

```json
{
  "connect":    [{"from": 7, "to": 22, "weight": 0.6, "reason": "..."}],
  "disconnect": [{"from": 1, "to": 7,  "reason": "..."}],
  "reweight":   [{"from": 3, "to": 5,  "weight": 0.3, "reason": "..."}]
}
    Director / extraction pass → structured nodes
        ↓
Weight guidance: 0.3 = weak thematic, 0.6 = moderate, 1.0 = strong causal.

### API

```cpp
struct GraphAnalysis { int live_node_count, active_edge_count, orphan_count; };

GraphAnalysis analyze(const WorldGraph& graph);
```
class Weaver {
public:
    explicit Weaver(WorldGraph& graph);
    void set_llm_callback(WeaverLLMCallback cb);
    void set_interval(int turns);            // default 3
    bool should_weave(int turn_index) const; // turn % interval == 0
    WeaveResult weave(int turn_index, const std::string& scene_context = "");
};
```
| Scenario initialization | Once, before the player starts |
### Debug endpoints
| Reactive spawning | After a major node resolves |
- `GET /analyze` — returns current `GraphAnalysis` as JSON
- `POST /weave` — triggers an on-demand weave on the saved graph
3. **NPC autonomy.** NPCs have goals and act on them off-screen. The player encounters them mid-action.
## Design philosophy
5. **Reputation propagation.** Player actions spread through NPC awareness via memory.
1. **Humean edges** — edge meaning emerges from node content and temporal ordering, not explicit labels
2. **Unified memory pattern** — WorldGraph (objective) and CharacterMemory (subjective) share the same graph + text + ChromaDB structure
3. **Most logic in C++** — retrieval, prompt building, and reflection are C++ methods; Python handles callbacks
4. **Subjective beliefs** — characters can hold false or biased beliefs, enabling dramatic irony and surprise
3. **Multi-dimensional arrival.** When the player reaches decision points in multiple threads simultaneously, how should the Director prioritize?
4. **Freeform edge-breaking.** What happens when a freeform action invalidates the destination node?
5. **Visual editor.** The plot graph is the natural candidate for a node inspection/editing tool.
- [system overview](system-overview.md) — how the Director uses the WorldGraph
- [memory system](memory-system.md) — ChromaDB indexing
- [[concepts/narrative-philosophy]] — design principles
- [[research/literature-review]] — CFPG, Generative Agents, and others
- [memory system](memory-system.md) — where resolved node facts are stored
- [[concepts/narrative-philosophy]] — the design principles behind the graph
- [[research/literature-review]] — CFPG, IBSEN, StoryVerse, and others
