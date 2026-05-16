---
sources:
  - core/include/rhapsode/scene_message.h
  - core/include/rhapsode/history.h
  - core/include/rhapsode/character.h
  - core/include/rhapsode/node.h
  - core/include/rhapsode/node_pool.h
  - core/include/rhapsode/director.h
  - core/include/rhapsode/scene_loop.h
  - core/include/rhapsode/memory_system.h
  - core/include/rhapsode/scene.h
  - bindings/bind_rhapsode.cpp
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/memory-system]]"
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

The `metadata` field is a pass-through JSON object — Python can attach arbitrary data without C++ needing to know the schema.

## History

Ordered collection of SceneMessages. Owns message storage for a scene.

```cpp
class History {
public:
    void append(SceneMessage msg);                              // sets timestamp via chrono UTC
    std::vector<SceneMessage> snapshot(std::optional<size_t> n) const;  // last n messages (or all)
    size_t size() const;
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
    bool is_player = false;
};
```

## Node

A plot node — a fact about the world that could become dramatically relevant.

```cpp
enum class NodeState { Dormant, Foreshadowed, Active, Resolved };

struct Node {
    uint64_t id = 0;
    std::string fact;                    // atomic assertion, max ~15 words
    std::string type;                    // "plot", "scene", "world", "relationship"
    NodeState state = NodeState::Dormant;
    std::string foreshadow_ctx;          // injected into prompt when foreshadowed
    std::string active_ctx;              // injected into prompt when active
    std::vector<std::string> entities;   // named entities involved
    std::vector<std::string> known_by;   // character IDs aware of this node
    int created_at = 0;                  // turn when created
    int resolved_at = -1;               // turn when resolved (-1 = unresolved)
};
```

Node states form a one-way progression: Dormant → Foreshadowed → Active → Resolved. The Director manages transitions; resolved nodes are removed from the pool.

`foreshadow_ctx` and `active_ctx` are directional hints for the narrative LLM. They are injected into the prompt as Director context blocks, not shown to the player.

## NodePool

Indexed container for all active nodes in a scene.

```cpp
class NodePool {
public:
    Node& add(Node node);               // assigns auto-incrementing ID
    Node* get(uint64_t id);
    void remove(uint64_t id);
    size_t size() const;

    std::vector<Node*> by_state(NodeState s);
    std::vector<Node*> by_entity(const std::string& entity_id);
    std::vector<Node*> by_known_by(const std::string& who);
    std::vector<Node*> wavefront();      // all Active nodes

    void for_each(std::function<void(const Node&)> fn) const;
    std::vector<Node> all_nodes() const;

    nlohmann::json to_json() const;      // includes next_id for save/load
    static NodePool from_json(const nlohmann::json& j);
};
```

The pool is a flat `unordered_map<uint64_t, Node>`. There are no edges between nodes in the current implementation — the full DAG with typed edges and trigger predicates is planned but not built. See [plot graph](plot-graph.md) for the design.

Indexing by state, entity, and `known_by` enables efficient queries without scanning all nodes.

## Director and DirectorOutput

The Director operates on a NodePool reference and produces a DirectorOutput each turn.

```cpp
struct DirectorOutput {
    std::vector<std::string> context_blocks;  // foreshadow_ctx + active_ctx strings
    std::vector<Node>        newly_resolved;  // nodes that transitioned to Resolved
    std::vector<Node>        new_nodes;       // nodes added by the LLM this turn
};

using DirectorLLMCallback = std::function<std::string(const std::string& prompt_json)>;
using RetrievalCallback   = std::function<std::string(const std::string& context_json)>;

class Director {
public:
    explicit Director(NodePool& pool);
    void set_llm_callback(DirectorLLMCallback cb);
    void set_retrieval_callback(RetrievalCallback cb);
    DirectorOutput tick(int turn_index, const std::string& scene_context);
};
```

`tick()` builds a JSON prompt from non-resolved nodes and optionally merges retrieval context. It calls the LLM, parses the JSON response, applies transitions, removes resolved nodes, and collects context blocks.

## SceneLoop

Finite state machine driving the turn cycle. See [scene loop](scene-loop.md) for full details.

```cpp
enum class LoopState { Idle, WaitingForInput, ProcessingInput, BuildingPrompt, RunningLLM, AppendingResult };

using PromptCallback       = std::function<std::string(const std::vector<SceneMessage>&, const Scene&, const DirectorOutput&)>;
using LLMCallback          = std::function<std::string(const std::string& prompt)>;
using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

class SceneLoop {
public:
    void load_scene(Scene& scene);
    void submit_input(const std::string& text);
    LoopState state() const;

    void set_prompt_callback(PromptCallback cb);
    void set_llm_callback(LLMCallback cb);
    void set_turn_complete_callback(TurnCompleteCallback cb);
    void set_director(Director* director);
    const DirectorOutput& last_director_output() const;

    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool v);
};
```

The prompt callback receives the `DirectorOutput` so it can include context blocks. History windowing defaults to 3 recent messages, or 10 on resume.

## MemorySystem

Callback-driven memory management. See [memory system](memory-system.md) for full details.

```cpp
class MemorySystem {
public:
    explicit MemorySystem(const std::string& scene_id);

    // 7 callbacks for Python services
    void set_embed_callback(EmbedCallback cb);
    void set_lemmatize_callback(LemmatizeCallback cb);
    void set_store_callback(StoreCallback cb);
    void set_query_callback(QueryCallback cb);
    void set_update_meta_callback(UpdateMetaCallback cb);
    void set_get_by_meta_callback(GetByMetaCallback cb);
    void set_local_llm_callback(LocalLLMCallback cb);

    // Storage
    std::string store_fact(/* fact, state, type, known_by, entities, turn */);
    std::string store_fact(/* same + pre-computed embedding_json */);

    // Retrieval
    std::string retrieve(const std::string& query, int top_k = 8) const;
    std::string retrieve_for_injection(const std::string& scene_context, int max_results = 8) const;

    // Post-turn processing
    void process_new_nodes(const std::vector<Node>& nodes, int turn);
};
```

## Scene

Top-level game state container.

```cpp
class Scene {
public:
    std::string scene_id;
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;
    History history;
    NodePool node_pool;
    int turn_index = 0;

    static Scene load_json(const std::string& path);   // from scenario file
    void save_json(const std::string& path) const;
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);

    void set_memory(MemorySystem* mem);

    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;
};
```

`load_json()` reads a scenario file: title, system_prompt, characters, seed_messages, seed nodes. `save()` / `load_save()` persist mutable game state to `saves/{scene_id}.json`. The save format includes `node_pool` with `next_id`, `history`, `turn_index`, and `memory_next_id`.

## Serialization contract

- All `to_json`/`from_json` are free functions in namespace `rhapsode`, following the nlohmann ADL pattern.
- Unknown JSON keys are ignored on deserialization (forward compatibility).
- `metadata` on SceneMessage is a pass-through — Python can attach data without C++ changes.
- File I/O uses `std::ifstream`/`std::ofstream` with UTF-8 encoding.

## pybind11 surface

| C++ type | Python name | Notes |
|----------|-------------|-------|
| `Role` | `_core.Role` | Enum: `.System`, `.User`, `.Assistant` |
| `SceneMessage` | `_core.SceneMessage` | Properties + `metadata` |
| `History` | `_core.History` | `append()`, `snapshot()`, `size()`, `clear()`, `messages()` |
| `Character` | `_core.Character` | `name`, `description`, `is_player` |
| `Node` | `_core.Node` | All fields read/write |
| `NodePool` | `_core.NodePool` | `add()`, `get()`, `remove()`, `by_state()`, `wavefront()`, etc. |
| `Director` | `_core.Director` | `tick()`, callback setters |
| `DirectorOutput` | `_core.DirectorOutput` | `context_blocks`, `newly_resolved`, `new_nodes` |
| `Scene` | `_core.Scene` | All fields + `load_json()`, `save()`, `load_save()`, etc. |
| `SceneLoop` | `_core.SceneLoop` | `submit_input()`, `state()`, `set_director()`, callback setters |
| `MemorySystem` | `_core.MemorySystem` | All callback setters, `store_fact()`, `retrieve()`, `process_new_nodes()` |
