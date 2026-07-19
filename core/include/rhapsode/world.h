#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

class MemorySystem;

// A character flagged as possibly dead by the keyword scan, with the graph
// facts that triggered it. Lives here because the scan is over the shared graph.
struct DeathCandidate {
    std::string character_name;
    std::vector<std::string> evidence;
};

// A lifecycle change a narrator staged during a beat, applied by Story only
// after the beat is accepted (see Story::apply_pending_ops). Kept as pure data
// so a decision tool never mutates the scene set mid-beat -- that would break
// the whole-graph snapshot/rollback the retry loop depends on.
enum class LifecycleKind { Fork, Conclude, Merge, Exit };

struct LifecycleOp {
    LifecycleKind kind;
    std::string source_scene_id;     // the beat's own scene
    std::string driving_intention;   // fork: the new storyline's drive
    std::string target_scene_id;     // merge: into this scene
    std::vector<std::string> cast;   // fork: cast to move onto the child; exit: who leaves
    std::string reason;              // conclude: why it ended
};

// The durable, shared substrate. One World is co-owned by every Scene that
// draws on it (via Scene's shared_ptr): the objective world graph, the
// per-character minds, and the character roster. Storylines fork and merge as
// separate Scenes over the same World; nothing here is per-scene/ephemeral.
class World {
public:
    WorldGraph world_graph;
    std::unordered_map<std::string, CharacterMemory> character_memories;
    std::vector<Character> characters;  // durable roster (membership via scene_ids)

    // -- Roster --
    Character* find_character(const std::string& name);
    const Character* find_character(const std::string& name) const;

    // -- Narrator tool-use queries over the shared substrate --
    /// Search world graph by entity name or free text. Returns entity-timeline
    /// chains (for entity matches) or matching nodes with chain predecessors
    /// (for text matches), as a JSON string.
    std::string tool_query_graph(const std::string& query) const;
    /// Get a character's thoughts, beliefs, and dialogue voice as JSON.
    std::string tool_query_mind(const std::string& character) const;

    /// Keyword-scan the graph for characters that may have died this run.
    std::vector<DeathCandidate> scan_death_candidates() const;

    // -- Staged lifecycle decisions (narrator decision tools) --------------
    // Each stage_* appends a LifecycleOp and returns a JSON ack string for the
    // tool caller. Ops accumulate across one beat's tool loop; the SceneLoop
    // clears them before each narrator attempt (so a rejected attempt's ops are
    // discarded) and Story drains + applies them after the beat is accepted.
    std::string stage_fork(const std::string& source_scene_id,
                           const std::string& driving_intention,
                           const std::vector<std::string>& cast);
    std::string stage_conclude(const std::string& source_scene_id,
                               const std::string& reason);
    std::string stage_merge(const std::string& source_scene_id,
                            const std::string& into_scene_id);
    /// Named characters leave `source_scene_id` for no storyline (a plain exit,
    /// not a fork): they simply stop being in that scene's cast.
    std::string stage_exit(const std::string& source_scene_id,
                           const std::vector<std::string>& cast);

    const std::vector<LifecycleOp>& pending_ops() const { return pending_ops_; }
    std::vector<LifecycleOp> take_pending_ops();
    void clear_pending_ops() { pending_ops_.clear(); }

    // -- System references --
    void set_memory(MemorySystem* mem) { memory_ = mem; }
    MemorySystem* memory() const { return memory_; }

    // -- Scenario bootstrap --
    /// Populate the graph (nodes/edges) and seed authored minds (initial_memory)
    /// from a scenario JSON. The roster itself is filled by Scene::from_json.
    void seed_from_scenario(const nlohmann::json& j);

    // -- Persistence (durable substrate -> world.json) --
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

    nlohmann::json to_json() const;
    static World from_json(const nlohmann::json& j);

private:
    MemorySystem* memory_ = nullptr;
    std::vector<LifecycleOp> pending_ops_;  // transient; never serialized
    static std::string save_path(const std::string& saves_dir);
};

} // namespace rhapsode
