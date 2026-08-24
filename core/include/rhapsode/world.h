#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/llm_callback.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

class World {
public:
    WorldGraph& graph() { return world_graph_; }
    const WorldGraph& graph() const { return world_graph_; }
    const std::vector<Character>& characters() const { return characters_; }
    const std::unordered_map<std::string, CharacterMemory>& character_memories() const {
        return character_memories_;
    }
    std::uint64_t state_version() const { return state_version_; }
    std::uint64_t advance_state_version() { return ++state_version_; }
    void set_state_version(std::uint64_t version) { state_version_ = version; }
    WorldGraph take_graph() {
        WorldGraph graph = std::move(world_graph_);
        world_graph_ = {};
        return graph;
    }
    void set_graph(WorldGraph graph) { world_graph_ = std::move(graph); }

    // -- Roster --
    const Character* find_character(const std::string& name) const;
    const Character* find_in_scene(const std::string& scene_id,
                                   const std::string& name) const;
    Character& enter_character(const std::string& scene_id, Character character);
    bool leave_character(const std::string& scene_id, const std::string& name);
    void add_scene_characters(const std::string& scene_id,
                              const std::vector<std::string>& canonical_names);
    void move_scene_members(const std::string& from_scene_id,
                            const std::string& into_scene_id,
                            const std::vector<std::string>& names = {});
    void clear_scene_membership(const std::string& scene_id);
    void set_character_memory(CharacterMemory memory);
    std::uint64_t seed_character_intention(
        const std::string& character,
        const std::string& intention,
        const std::vector<std::string>& subjects,
        int created_at);
    bool expire_character_intention(const std::string& character,
                                    std::uint64_t node_id,
                                    int valid_until);
    std::vector<std::uint64_t> revert_to_turn(int target_turn);

    void update_perceptions(const std::string& scene_id,
                            int turn,
                            const std::string& narration_window,
                            const LLMCallback& llm_callback);
    void update_monologues(const std::string& scene_id,
                           int turn,
                           const LLMCallback& llm_callback);
    void apply_ready_perceptions(
        int turn,
        const std::function<bool(std::size_t, int, std::string&, bool&)>& ready);
    void apply_ready_monologues(
        int turn,
        const std::function<bool(std::size_t, int, std::string&, bool&)>& ready);
    void poll_perceptions(
        const std::string& scene_id,
        int turn,
        const std::string& narration_window,
        const std::function<bool(std::size_t, int, std::string&, bool&)>& ready,
        const std::function<void(const std::vector<PromptJob>&)>& submit);
    void poll_monologues(
        const std::string& scene_id,
        int turn,
        const std::function<bool(std::size_t, int, std::string&, bool&)>& ready,
        const std::function<void(const std::vector<PromptJob>&)>& submit);
    bool mark_character_dead(const std::string& name);

    nlohmann::json to_json() const;
    static World from_json(const nlohmann::json& j);

private:
    Character* find_character_mutable(const std::string& name);
    Character& add_new_character(const std::string& scene_id,
                                 Character character);

    WorldGraph world_graph_;
    std::unordered_map<std::string, CharacterMemory> character_memories_;
    std::vector<Character> characters_;  // membership via Character::scene_ids
    std::uint64_t state_version_ = 0;
};

} // namespace rhapsode
