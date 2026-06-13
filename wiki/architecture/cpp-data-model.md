---
sources:
  - core/include/rhapsode/scene_message.h
  - core/include/rhapsode/history.h
  - core/include/rhapsode/character.h
  - core/include/rhapsode/character_memory.h
  - core/include/rhapsode/node.h
  - core/include/rhapsode/world_graph.h
  - core/include/rhapsode/director.h
  - core/include/rhapsode/validator.h
  - core/include/rhapsode/weaver.h
  - core/include/rhapsode/annotator.h
  - core/include/rhapsode/text_downsampler.h
  - core/include/rhapsode/scene_loop.h
  - core/include/rhapsode/memory_system.h
  - core/include/rhapsode/scene.h
  - bindings/bind_rhapsode.cpp
last_updated: 2026-05-23
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/memory-system]]"
  - "[[architecture/companion-system]]"
tags:
  - cpp-core
---

# C++ data model

All core data types live in `core/include/rhapsode/`. JSON serialization uses `nlohmann/json` with ADL `to_json`/`from_json` free functions. Every type listed here is exposed to Python via pybind11 in `bindings/bind_rhapsode.cpp`.

## SceneMessage

A single message in the conversation history.

```cpp
enum class Role { System, User, Assistant };

struct SceneMessage {
    Role role;
    std::string content;
    std::string timestamp;           // ISO 8601 UTC, set on append
    nlohmann::json metadata = {};    // extensible pass-through for Python
};
```

JSON representation:

```json
{ "role": "user", "content": "I approach the bar.", "timestamp": "2026-05-05T14:30:00Z", "metadata": {} }
```

The `metadata` field is a pass-through JSON object — Python can attach arbitrary data (e.g. `scene_kind`, `speaker`) without C++ needing to know the schema.

## History

Ordered collection of SceneMessages. Owns message storage for a scene.

```cpp
class History {
public:
    void append(SceneMessage msg);                              // sets timestamp via chrono UTC
    std::vector<SceneMessage> snapshot(std::optional<size_t> n) const;  // last n messages (or all)
    size_t size() const;
    void truncate(size_t new_size);
    void clear();
    const std::vector<SceneMessage>& messages() const;
};
```

`snapshot(n)` returns a copy — the prompt builder receives an immutable view. No max-length enforcement; the prompt builder is responsible for truncation.

## Character

Metadata about a scene participant.

```cpp
struct Character {
    std::string name;
    std::string description;
    std::string dialogue_instructions;
    std::vector<std::string> example_dialogue;
    std::string role;                  // e.g. "companion" — routes memory/prompt pipelines
    bool is_player = false;
    bool on_stage = false;             // visible in current scene
    bool dead = false;
    int created_at = 0;                // turn when introduced
};
```

`role` is a free-form tag used for routing: a `"companion"` character gets a richer prompt path and per-character memory. `on_stage` is managed by the `Scene` theatre model (enter/exit lifecycle). Serialization omits empty optional fields for compact scenario files.

## Node

A plot node — a fact about the world that could become dramatically relevant.

```cpp
enum class NodeState { Dormant, Foreshadowed, Active, Resolved };

struct Node {
    uint64_t id = 0;
    std::string fact;                    // atomic assertion, ~15 words
    std::string type;                    // "plot", "scene", "world", "relationship"
    NodeState state = NodeState::Dormant;
    std::string foreshadow_ctx;          // injected into prompt when foreshadowed
    std::string active_ctx;              // injected into prompt when active
    std::vector<std::string> entities;   // named entities involved
    std::string trigger;                 // condition that activates this node
    std::string arc_position;            // "foreshadow", "trigger", or "payoff"
    std::vector<uint64_t> related_to;    // IDs of graph-linked nodes
    int created_at = 0;                  // turn when created
    int resolved_at = -1;               // turn when resolved (-1 = unresolved)
};
```

Node states form a one-way progression: Dormant → Foreshadowed → Active → Resolved. The Director manages transitions.

`trigger` and `arc_position` support Foreshadow-Trigger-Payoff tracking — nodes progress through narrative arcs with explicit arc labeling.

## WorldGraph

Directed graph with weighted edges, built on Boost.Graph. Replaces the earlier flat NodePool.

```cpp
struct EdgeData {
    float weight = 1.0f;
    int created_at = 0;
    bool active = true;
};

struct EdgeInfo {
    uint64_t from_id;
    uint64_t to_id;
    EdgeData data;
};

class WorldGraph {
public:
    using Graph = boost::adjacency_list<vecS, vecS, directedS, Node, EdgeData>;

    Node& add_node(Node node);               // assigns auto-incrementing ID
    void upsert_node(const Node& node);
    bool has_node(uint64_t node_id) const;
    Node* get_node(uint64_t node_id);
    bool mark_resolved(uint64_t node_id, int resolved_at);
    size_t size() const;
    std::vector<Node> all_nodes(bool include_resolved = false) const;
    void for_each(std::function<void(const Node&)> fn, bool include_resolved = false) const;

    bool add_relation(uint64_t from_id, uint64_t to_id,
                      float weight = 1.0f, int created_at = 0);
    bool set_edge_active(uint64_t from_id, uint64_t to_id, bool active);
    bool set_edge_weight(uint64_t from_id, uint64_t to_id, float weight);
    std::vector<EdgeInfo> all_edges() const;

    std::vector<uint64_t> neighbors(uint64_t node_id) const;
    std::vector<uint64_t> neighbors_within(uint64_t source_id, int max_hops,
                                           bool active_only = true) const;

    std::vector<uint64_t> thread_containing(uint64_t seed_id) const;
    std::vector<std::vector<uint64_t>> all_threads() const;

    std::vector<uint64_t> revert_to_turn(int turn);

    nlohmann::json to_json() const;
    std::string to_dot() const;
    static WorldGraph from_json(const nlohmann::json& j);
    static WorldGraph from_legacy_node_pool_json(const nlohmann::json& j);
};
```

Node lookup uses an `unordered_map<uint64_t, Vertex>` for O(1) access by ID. `size()` returns the count of non-resolved nodes. `for_each()` and `all_nodes()` skip resolved nodes by default.

Edge direction is enforced temporally (older node = source). `add_relation()` deduplicates (same from/to pair rejected). `neighbors()` returns IDs reachable via one active edge. `neighbors_within()` performs BFS up to `max_hops`, filtering by active edges.

Thread queries (`thread_containing`, `all_threads`) use connected components to find related narrative chains. `revert_to_turn(turn)` removes all nodes created after the given turn and returns their IDs.

Serialization includes both nodes and edges arrays. `from_legacy_node_pool_json()` loads old saves that had no edges. `to_dot()` exports a Graphviz representation for debugging.

See [plot graph](plot-graph.md) for the full design.

## Director and DirectorOutput

The Director operates on a WorldGraph reference. It does not call the LLM directly during normal play — instead it provides context for the merged narrator prompt and applies graph mutations from the LLM's JSON response.

```cpp
struct Rejection {
    std::string fact;
    std::string reason;
};

struct DirectorOutput {
    std::vector<std::string> context_blocks;  // foreshadow_ctx + active_ctx strings
    std::vector<Node>        newly_resolved;  // nodes that transitioned to Resolved
    std::vector<Node>        new_nodes;       // nodes added by the LLM this turn
    std::vector<Rejection>   rejections;      // nodes rejected by the Validator
};

using DirectorLLMCallback = std::function<std::string(const std::string& prompt_json)>;

class Director {
public:
    explicit Director(WorldGraph& graph);
    void set_llm_callback(DirectorLLMCallback cb);
    void set_validator(Validator* v);
    DirectorOutput tick(int turn_index, const std::string& scene_context);
    std::string focus_payload_json(int turn_index, const std::string& scene_context) const;
    std::string focus_payload_text(int turn_index, const std::string& scene_context) const;
    DirectorOutput apply_planned_turn(int turn_index, const nlohmann::json& response);
};
```

- `focus_payload_json()` builds the full context payload (nodes + 2-hop BFS context) as a JSON object for embedding in the merged narrator prompt.
- `focus_payload_text()` produces a compact text-format payload — one line per node (Active + Foreshadowed only) with BFS-seeded focus IDs in the header.
- `apply_planned_turn()` applies `transitions` and `new_nodes` from the LLM's JSON response. New nodes are validated via the attached `Validator` (if set); rejected nodes are collected in `DirectorOutput::rejections`. Accepted nodes are auto-linked via entity matching.
- `tick()` exists for standalone Director operation (calls `llm_cb_` then `apply_planned_turn`).

## Validator

Validates candidate nodes against the existing WorldGraph for consistency. Used by the Director to reject contradictions before they enter the graph.

```cpp
struct Verdict {
    bool accepted = true;
    std::string reason;
};

using ValidatorLLMCallback = std::function<std::string(const std::string&)>;
using SearchCallback = std::function<std::vector<uint64_t>(const std::string&, int)>;
using DeadCheckCallback = std::function<std::vector<std::string>()>;

class Validator {
public:
    explicit Validator(const WorldGraph& graph);
    void set_llm_callback(ValidatorLLMCallback cb);
    void set_search_callback(SearchCallback cb);
    void set_dead_check(DeadCheckCallback cb);
    Verdict check(const Node& candidate) const;
};
```

`check()` gathers context (semantically related nodes + dead entity list), builds a prompt asking the LLM whether the candidate node contradicts established facts, and returns a Verdict with acceptance status and reasoning.

## Weaver

Periodic graph maintenance — discovers missing edges between semantically related nodes, deactivates stale edges, and reweights connections based on narrative evolution.

```cpp
struct GraphAnalysis {
    int live_node_count = 0;
    int active_edge_count = 0;
    int orphan_count = 0;
};

GraphAnalysis analyze(const WorldGraph& graph);

struct WeaveOp {
    uint64_t from_id = 0;
    uint64_t to_id = 0;
    float weight = 1.0f;
    std::string reason;
};

struct WeaveResult {
    std::vector<WeaveOp> connected;
    std::vector<WeaveOp> disconnected;
    std::vector<WeaveOp> reweighted;
    GraphAnalysis analysis;
};

class Weaver {
public:
    explicit Weaver(WorldGraph& graph);
    void set_llm_callback(WeaverLLMCallback cb);
    void set_local_llm_callback(WeaverLLMCallback cb);
    void set_interval(int turns);
    bool should_weave(int turn_index) const;
    WeaveResult weave(int turn_index, const std::string& scene_context = "");
    WeaveResult weave_local(int turn_index, const std::string& scene_context = "");
};
```

The Weaver runs on a configurable interval (default: every 3 turns). `weave()` uses the cloud LLM for high-quality edge decisions. `weave_local()` uses the local LLM for lightweight maintenance on off-turns. Both produce a `WeaveResult` reporting which edges were added, removed, or reweighted.

`analyze()` is a free function providing graph health metrics (live nodes, active edges, orphans).

## Annotator

Named entity recognition for prose text, combining game-state knowledge with optional NER model output.

```cpp
struct EntitySpan {
    int start;
    int end;
    std::string text;
    std::string category;   // "character", "location", "faction", etc.
};

using NERCallback = std::function<std::string(const std::string& text)>;

class Annotator {
public:
    explicit Annotator(const Scene& scene);
    void set_ner_callback(NERCallback cb);
    std::vector<EntitySpan> annotate(const std::string& text) const;
};
```

`annotate()` first matches known character names from the Scene's character list, then merges results with the NER callback output (if set). Game-state matches take priority over model predictions in case of overlap.

## TextDownsampler

Multi-level summarization of conversation history — implements a mipmap-like hierarchy where older content is progressively compressed into summaries.

```cpp
struct Snippet {
    std::string text;
    int turn_start = 0;
    int turn_end = 0;
    double timestamp = 0.0;
    bool promoted = false;
    int source_mip = -1;
    int merged_count = 1;
};

struct MipLevel {
    std::vector<Snippet> snippets;
    int max_snippets = 10;
};

class TextDownsampler {
public:
    void set_llm_callback(DownsamplerLLMCallback cb);
    bool has_llm_callback() const;
    void process_turn(const std::vector<SceneMessage>& messages, int verbatim_tail = 6);
    std::string render() const;
    int summarized_up_to() const;
    nlohmann::json to_json() const;
    static TextDownsampler from_json(const nlohmann::json& j);
};
```

`process_turn()` ingests new messages; the most recent `verbatim_tail` turns remain verbatim while older content cascades through progressively coarser summary levels. `render()` produces the full context string combining all levels. The LLM callback is used for summarization; if unset, no compression occurs.

## CharacterMemory

Per-character subjective memory system based on the Generative Agents observe-reflect-plan architecture. Uses a Boost directed graph of `MemoryNode` beliefs backed by ChromaDB for semantic retrieval.

```cpp
struct MemoryNode {
    uint64_t id = 0;
    std::string content;
    std::optional<uint64_t> source_node;  // link to WorldGraph node that spawned this
    int created_at = 0;
    int poignancy = 0;                     // importance score (1-10)
    int depth = 0;                         // 0 = base belief, 1+ = reflection
    std::string mem_type;                  // "belief", "observation", "reflection"
    std::vector<uint64_t> filling;         // evidence IDs for reflections
};

class CharacterMemory {
public:
    explicit CharacterMemory(std::string name);

    // Callbacks (set from Python)
    void set_embed_callback(EmbedCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_reflection_llm_callback(ReflectionLLMCallback cb);

    // Belief graph CRUD
    MemoryNode& add_belief(MemoryNode node);
    MemoryNode* get_belief(uint64_t id);
    std::vector<MemoryNode> all_beliefs() const;
    MemoryNode* find_by_source(uint64_t source_node_id);
    bool add_link(uint64_t from_id, uint64_t to_id, float weight = 1.0f, int created_at = 0);

    // Observation intake
    void add_observation(const std::string& text, int turn);

    // Retrieval (three-signal: recency + relevance + importance)
    std::string retrieve_context(const std::string& query, int top_k = 5) const;

    // Reflection (LLM-driven belief synthesis)
    void process_reflection(const std::string& dialogue, const std::string& cue_json,
                            const std::string& narrator_beat, int turn);
    void try_meta_reflection(int turn);   // importance-gated higher-order reflection

    // ChromaDB sync
    void sync_to_chroma();

    // Serialization
    nlohmann::json to_json() const;
    static CharacterMemory from_json(const nlohmann::json& j);

    const std::string& name() const;
    size_t size() const;
};
```

Retrieval uses a composite score: `0.5 * recency + 3.0 * relevance + 2.0 * importance` (Generative Agents weights). Over-fetches from ChromaDB at 2× `top_k`, re-ranks locally, truncates.

Meta-reflection triggers when the importance accumulator drops below zero (accumulates poignancy from observations, resets at threshold of 150). The meta-reflection generates focal questions, retrieves evidence, and synthesizes high-level insights as `depth=1` reflection nodes.

See [companion system](companion-system.md) for the full design context.

## MemorySystem

Scene-scoped node indexing in ChromaDB. Simplified interface — stores nodes as embeddings, retrieves by semantic similarity.

```cpp
class MemorySystem {
public:
    explicit MemorySystem(const std::string& scene_id);

    void set_embed_callback(EmbedCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_update_meta_callback(UpdateMetaCallback cb);
    void set_get_by_meta_callback(GetByMetaCallback cb);
    void set_delete_callback(DeleteCallback cb);
    void set_local_llm_callback(LocalLLMCallback cb);

    void store_node(uint64_t node_id, const std::string& fact,
                    const std::string& state, const std::string& type, int turn);
    std::vector<uint64_t> search_nodes(const std::string& query, int top_k = 10) const;
    void delete_nodes(const std::vector<uint64_t>& node_ids);

    void process_new_nodes(const std::vector<Node>& nodes, int turn);
    void sync_resolved(const std::vector<Node>& resolved_nodes, int turn);

    int get_next_id() const;
    void set_next_id(int id);
};
```

Uses a single ChromaDB collection (`{scene_id}_nodes`). `store_node()` embeds the fact text and stores with metadata (node_id, state, type, turn). `search_nodes()` filters out dormant nodes and returns matching node IDs. `sync_resolved()` updates metadata on resolved nodes.

See [memory system](memory-system.md) for the full architecture.

## SceneLoop

Finite state machine driving the turn cycle. See [scene loop](scene-loop.md) for full details.

```cpp
enum class LoopState {
    Idle, WaitingForInput, ProcessingInput,
    Weaving, BuildingPrompt, RunningLLM, AppendingResult
};

using PromptCallback =
    std::function<std::pair<std::string, std::string>(
        const std::vector<SceneMessage>&,
        const Scene&,
        const DirectorOutput&,
        const std::string& director_focus_text)>;
using LLMCallback          = std::function<std::string(const std::string& prompt)>;
using NarratorLLMCallback  = std::function<std::string(const std::string& system_msg,
                                                        const std::string& user_msg)>;
using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

class SceneLoop {
public:
    void load_scene(Scene& scene);
    void submit_input(const std::string& text);
    LoopState state() const;

    void set_prompt_callback(PromptCallback cb);
    void set_llm_callback(LLMCallback cb);
    void set_narrator_llm_callback(NarratorLLMCallback cb);
    void set_turn_complete_callback(TurnCompleteCallback cb);
    void set_actor_llm_callback(LLMCallback cb);
    void set_director(Director* director);
    const DirectorOutput& last_director_output() const;

    void set_weaver(Weaver* weaver);
    const WeaveResult& last_weave_result() const;

    std::vector<SceneMessage> take_last_turn_outputs();
    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool v);
};
```

The prompt callback returns a `pair<system_msg, user_msg>` for the narrator LLM. It receives the `DirectorOutput` and the director's text-format focus payload. The `NarratorLLMCallback` accepts system and user messages separately; if unset, falls back to the single-string `LLMCallback` with concatenation. `set_actor_llm_callback` provides the LLM endpoint for NPC dialogue synthesis (separate from the narrator model).

The Weaver is invoked in the `Weaving` state (between ProcessingInput and BuildingPrompt) on turns where `should_weave()` returns true.

History windowing defaults to 8 recent messages, or 12 on resume. `take_last_turn_outputs()` returns all SceneMessages produced by the last turn (narrator prose + character dialogue lines), cleared on consume.

## Scene

Top-level game state container.

```cpp
class Scene {
public:
    // Static (from scenario file)
    std::string scene_id;
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;

    // Mutable game state
    History history;
    WorldGraph world_graph;
    TextDownsampler downsampler;
    std::unordered_map<std::string, CharacterMemory> character_memories;
    int turn_index = 0;

    static Scene load_json(const std::string& path);

    // Character lifecycle (theatre model)
    Character& enter_character(Character ch);
    Character* find_on_stage(const std::string& name);
    bool exit_character(const std::string& name);
    std::vector<std::string> exit_stale_characters();

    // Undo
    int revert_turns(int n);

    // System references
    void set_memory(MemorySystem* mem);

    // Persistence
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

    // Scenario-format serialization
    void save_json(const std::string& path) const;
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);
};
```

`load_json()` reads a scenario file: title, system_prompt, characters, seed_messages, seed nodes, and per-character initial memories. `save()` / `load_save()` persist mutable game state including `world_graph`, `history`, `turn_index`, `character_memories`, and `downsampler`. `revert_turns(n)` rolls back the last N turns by truncating history, reverting the WorldGraph, and resetting turn_index.

The theatre model manages character visibility: `enter_character()` adds/marks a character as `on_stage`, `exit_character()` marks them off-stage, `exit_stale_characters()` cleans up characters introduced dynamically that are no longer referenced.

## Serialization contract

- All `to_json`/`from_json` are free functions in namespace `rhapsode`, following the nlohmann ADL pattern.
- Unknown JSON keys are ignored on deserialization (forward compatibility).
- `metadata` on SceneMessage is a pass-through — Python can attach data without C++ changes.
- File I/O uses `std::ifstream`/`std::ofstream` with UTF-8 encoding.

## pybind11 surface

| C++ type | Python name | Notes |
|----------|-------------|-------|
| `Role` | `_core.Role` | Enum: `.System`, `.User`, `.Assistant` |
| `SceneMessage` | `_core.SceneMessage` | Properties + `metadata` as string get/set |
| `History` | `_core.History` | `append()`, `snapshot()`, `size()`, `truncate()`, `clear()`, `messages()` |
| `Character` | `_core.Character` | All 9 fields read/write: `name`, `description`, `dialogue_instructions`, `example_dialogue`, `role`, `is_player`, `on_stage`, `dead`, `created_at` |
| `NodeState` | `_core.NodeState` | Enum: `.Dormant`, `.Foreshadowed`, `.Active`, `.Resolved` |
| `Node` | `_core.Node` | All fields read/write including `trigger`, `arc_position`, `related_to` |
| `EdgeData` | `_core.EdgeData` | Read-only: `weight`, `created_at`, `active` |
| `EdgeInfo` | `_core.EdgeInfo` | Read-only: `from_id`, `to_id`, `data` |
| `WorldGraph` | `_core.WorldGraph` | `add_node()`, `get_node()`, `has_node()`, `mark_resolved()`, `add_relation()`, `set_edge_active()`, `set_edge_weight()`, `all_edges()`, `neighbors()`, `neighbors_within()`, `thread_containing()`, `all_threads()`, `size()`, `all_nodes()`, `all_nodes_including_resolved()`, `to_json_str()`, `to_dot()`, `from_json_str()`, `__len__` |
| `Rejection` | `_core.Rejection` | `fact`, `reason` |
| `DirectorOutput` | `_core.DirectorOutput` | `context_blocks`, `newly_resolved`, `new_nodes`, `rejections` |
| `Director` | `_core.Director` | `Director(WorldGraph&)`, `set_llm_callback()`, `set_validator()`, `tick()`, `focus_payload_json()`, `focus_payload_text()`, `apply_planned_turn()` |
| `Verdict` | `_core.Verdict` | `accepted`, `reason` |
| `Validator` | `_core.Validator` | `Validator(WorldGraph&)`, `set_llm_callback()`, `set_search_callback()`, `set_dead_check()`, `check()` |
| `GraphAnalysis` | `_core.GraphAnalysis` | Read-only: `live_node_count`, `active_edge_count`, `orphan_count` |
| `WeaveOp` | `_core.WeaveOp` | Read-only: `from_id`, `to_id`, `weight`, `reason` |
| `WeaveResult` | `_core.WeaveResult` | Read-only: `connected`, `disconnected`, `reweighted`, `analysis` |
| `Weaver` | `_core.Weaver` | `Weaver(WorldGraph&)`, `set_llm_callback()`, `set_local_llm_callback()`, `set_interval()`, `should_weave()`, `weave()`, `weave_local()` |
| `EntitySpan` | `_core.EntitySpan` | Read-only: `start`, `end_`, `text`, `category` |
| `Annotator` | `_core.Annotator` | `Annotator(Scene&)`, `set_ner_callback()`, `annotate()` |
| `Snippet` | `_core.Snippet` | All fields read/write: `text`, `turn_start`, `turn_end`, `timestamp`, `promoted`, `source_mip`, `merged_count` |
| `TextDownsampler` | `_core.TextDownsampler` | `set_llm_callback()`, `has_llm_callback()`, `process_turn()`, `render()`, `summarized_up_to()`, `to_json_str()`, `from_json_str()` |
| `MemoryNode` | `_core.MemoryNode` | All fields read/write: `id`, `content`, `source_node`, `created_at`, `poignancy`, `depth`, `mem_type`, `filling` |
| `CharacterMemory` | `_core.CharacterMemory` | `CharacterMemory(name)`, callback setters, `add_belief()`, `get_belief()`, `all_beliefs()`, `find_by_source()`, `add_link()`, `sync_to_chroma()`, `add_observation()`, `retrieve_context()`, `process_reflection()`, `try_meta_reflection()`, `to_json_str()`, `from_json_str()`, `name`, `size()`, `__len__` |
| `LoopState` | `_core.LoopState` | Enum: `.Idle`, `.WaitingForInput`, `.ProcessingInput`, `.Weaving`, `.BuildingPrompt`, `.RunningLLM`, `.AppendingResult` |
| `SceneLoop` | `_core.SceneLoop` | `load_scene()`, `submit_input()`, `state()`, `set_prompt_callback()`, `set_llm_callback()`, `set_narrator_llm_callback()`, `set_turn_complete_callback()`, `set_actor_llm_callback()`, `take_last_turn_outputs()`, `set_director()`, `last_director_output()`, `set_weaver()`, `last_weave_result()`, `set_history_window()`, `set_resuming()` |
| `Scene` | `_core.Scene` | All fields + `world_graph` property + `character_memories` + `downsampler` + `enter_character()`, `find_on_stage()`, `exit_character()`, `exit_stale_characters()`, `revert_turns()`, `set_memory()`, `has_save()`, `load_save()`, `save()`, `delete_save()`, `load_json()`, `save_json()`, JSON serialization |
| `MemorySystem` | `_core.MemorySystem` | `MemorySystem(scene_id)`, all callback setters, `store_node()`, `search_nodes()`, `delete_nodes()`, `process_new_nodes()`, `sync_resolved()`, `get_next_id()`, `set_next_id()` |
| `analyze_graph` | `_core.analyze_graph` | Free function: `analyze_graph(WorldGraph) -> GraphAnalysis` |
