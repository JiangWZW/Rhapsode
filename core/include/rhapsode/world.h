#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/llm_callback.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

// A character flagged as possibly dead by the keyword scan, with the graph
// facts that triggered it. Lives here because the scan is over the shared graph.
struct DeathCandidate {
    std::string character_name;
    std::vector<std::string> evidence;
};

// The durable substrate for one Story: objective graph, character minds, roster,
// and membership. World knows scene identity only as string ids carried by the
// roster; it does not own or reference the Story's per-scene records.
class World {
public:
    WorldGraph& graph() { return world_graph_; }
    const WorldGraph& graph() const { return world_graph_; }
    const std::vector<Character>& characters() const { return characters_; }
    const std::unordered_map<std::string, CharacterMemory>& character_memories() const {
        return character_memories_;
    }

    // -- Roster --
    const Character* find_character(const std::string& name) const;
    const Character* find_in_scene(const std::string& scene_id,
                                   const std::string& name) const;
    Character& enter_character(const std::string& scene_id, Character character);
    bool leave_character(const std::string& scene_id, const std::string& name);
    void ensure_characters_present(const std::string& scene_id,
                                   const std::vector<std::string>& canonical_names);
    void move_scene_members(const std::string& from_scene_id,
                            const std::string& into_scene_id,
                            const std::vector<std::string>& names = {});
    void clear_scene_membership(const std::string& scene_id);
    void set_character_memory(CharacterMemory memory);
    bool seed_character_intention(const std::string& character,
                                  const std::string& intention,
                                  const std::vector<std::string>& subjects,
                                  int created_at);
    std::vector<std::uint64_t> revert_to_turn(int target_turn);

    // -- Narrator tool-use queries over the shared substrate --
    /// Search world graph by entity name or free text. Returns entity-timeline
    /// chains (for entity matches) or matching nodes with chain predecessors
    /// (for text matches), as a JSON string.
    std::string tool_query_graph(const std::string& query) const;
    /// Get a character's thoughts, beliefs, and dialogue voice as JSON.
    std::string tool_query_mind(const std::string& character) const;

    /// Keyword-scan the graph for characters that may have died this run.
    std::vector<DeathCandidate> scan_death_candidates() const;

    /// Route objective nodes into the minds that perceived them in one scene.
    void route_perceptions(const std::string& scene_id,
                           const std::vector<Node>& nodes,
                           int turn);
    /// Reflect every mind that has pending perceptions. Minds with none are a no-op.
    void reflect_perceptions(int turn, const LLMCallback& llm_callback);
    /// Mark a roster character dead and remove all scene memberships.
    bool mark_character_dead(const std::string& name);

    // -- Scenario bootstrap --
    /// Populate graph, roster, membership, and authored minds from a scenario.
    void seed_from_scenario(const nlohmann::json& j,
                            const std::string& root_scene_id = {});

    nlohmann::json to_json() const;
    static World from_json(const nlohmann::json& j);

private:
    Character* find_character_mutable(const std::string& name);

    WorldGraph world_graph_;
    std::unordered_map<std::string, CharacterMemory> character_memories_;
    std::vector<Character> characters_;  // membership via Character::scene_ids
};

} // namespace rhapsode
