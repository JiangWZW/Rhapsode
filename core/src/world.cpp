#include "rhapsode/world.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace rhapsode {

// -- Roster ------------------------------------------------------------------

Character* World::find_character_mutable(const std::string& name) {
    for (auto& c : characters_)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* World::find_character(const std::string& name) const {
    for (const auto& c : characters_)
        if (str::iequals(c.name, name)) return &c;
    return nullptr;
}

const Character* World::find_in_scene(const std::string& scene_id,
                                      const std::string& name) const {
    const Character* character = find_character(name);
    return character && !character->dead && character->in_scene(scene_id)
        ? character : nullptr;
}

Character& World::enter_character(const std::string& scene_id, Character character) {
    if (Character* existing = find_character_mutable(character.name)) {
        if (existing->dead) {
            log() << "  [cast] " << existing->name
                  << " is dead -- ignoring re-entry\n";
            return *existing;
        }
        const bool was_off = !existing->in_scene(scene_id);
        existing->join_scene(scene_id);
        if (existing->description.empty() && !character.description.empty())
            existing->description = std::move(character.description);
        if (existing->dialogue_instructions.empty() &&
            !character.dialogue_instructions.empty())
            existing->dialogue_instructions = std::move(character.dialogue_instructions);
        if (was_off)
            log() << "  [cast] " << existing->name
                  << " re-enters (was off-stage)\n";
        return *existing;
    }

    if (!scene_id.empty()) character.join_scene(scene_id);
    characters_.push_back(std::move(character));
    Character& added = characters_.back();
    log() << "  [cast] NEW " << added.name
          << " | role=" << (added.role.empty() ? "?" : added.role)
          << " | \"" << added.description.substr(0, 80) << "\"\n";

    if (!added.is_player && character_memories_.find(added.name) == character_memories_.end()) {
        character_memories_.emplace(added.name, CharacterMemory(added.name));
        log() << "  [cast]   memory created (empty -- learns by perception)\n";
    }
    return added;
}

bool World::leave_character(const std::string& scene_id, const std::string& name) {
    Character* character = find_character_mutable(name);
    if (!character || !character->in_scene(scene_id)) return false;
    character->leave_scene(scene_id);
    return true;
}

void World::ensure_characters_present(
    const std::string& scene_id,
    const std::vector<std::string>& canonical_names) {
    std::unordered_set<std::string> resolved_names;
    resolved_names.reserve(canonical_names.size());
    for (const auto& name : canonical_names)
        resolved_names.insert(str::to_lower(name));

    for (auto& character : characters_) {
        if (character.is_player || character.dead) continue;
        if (resolved_names.count(str::to_lower(character.name)) > 0 &&
            !character.in_scene(scene_id)) {
            character.join_scene(scene_id);
            log() << "  [cast] " << character.name << " enters (in active_cast)\n";
        }
    }
}

void World::move_scene_members(const std::string& from_scene_id,
                               const std::string& into_scene_id,
                               const std::vector<std::string>& names) {
    std::unordered_set<std::string> selected;
    for (const auto& name : names) selected.insert(str::to_lower(name));
    for (auto& character : characters_) {
        if (!character.in_scene(from_scene_id)) continue;
        if (!selected.empty() && selected.count(str::to_lower(character.name)) == 0) continue;
        if (!names.empty() && character.is_player) continue;
        character.join_scene(into_scene_id);
        character.leave_scene(from_scene_id);
    }
}

void World::clear_scene_membership(const std::string& scene_id) {
    for (auto& character : characters_) character.leave_scene(scene_id);
}

void World::set_character_memory(CharacterMemory memory) {
    character_memories_.insert_or_assign(memory.name(), std::move(memory));
}

bool World::seed_character_intention(const std::string& character,
                                     const std::string& intention,
                                     const std::vector<std::string>& subjects,
                                     int created_at) {
    const Character* canonical = find_character(character);
    if (!canonical) return false;
    auto it = character_memories_.find(canonical->name);
    if (it == character_memories_.end()) return false;
    it->second.seed_belief(intention, subjects, created_at,
                           CharacterMemory::kAuthoredSeedWeight, "intention");
    return true;
}

std::vector<std::uint64_t> World::revert_to_turn(int target_turn) {
    const auto removed_ids = world_graph_.revert_to_turn(target_turn);

    characters_.erase(
        std::remove_if(characters_.begin(), characters_.end(),
            [target_turn](const Character& c) { return c.created_at >= target_turn; }),
        characters_.end());
    for (auto& character : characters_)
        if (!character.is_player) character.dead = false;

    for (auto it = character_memories_.begin(); it != character_memories_.end();) {
        const bool exists = std::any_of(characters_.begin(), characters_.end(),
            [&](const Character& c) { return c.name == it->first; });
        it = exists ? std::next(it) : character_memories_.erase(it);
    }
    return removed_ids;
}

void World::route_perceptions(const std::string& scene_id,
                              const std::vector<Node>& nodes,
                              int turn) {
    int deliveries = 0;
    std::unordered_set<std::string> minds;
    auto route_to = [&](const std::string& name, const Node& node) {
        auto it = character_memories_.find(name);
        if (it != character_memories_.end()) {
            it->second.route_fact(node.fact, node.entities, turn);
            ++deliveries;
            minds.insert(it->first);
        }
    };

    for (const auto& node : nodes) {
        if (!node.audience.empty()) {
            for (const auto& name : node.audience) {
                route_to(name, node);
            }
        } else {
            for (const auto& character : characters_) {
                if (character.is_player || character.dead || !character.in_scene(scene_id)) {
                    continue;
                }
                route_to(character.name, node);
            }
        }
    }

    log() << "  [perceive] " << nodes.size() << " new_node(s) -> " << deliveries
          << " perception(s) routed to " << minds.size() << " mind(s)\n" << std::flush;
}

void World::reflect_perceptions(int turn, const LLMCallback& llm_callback) {
    for (auto& [name, memory] : character_memories_) {
        std::string description;
        for (const auto& character : characters_) {
            if (character.name == name) {
                description = character.description;
                break;
            }
        }
        memory.reflect_perceptions(turn, description, llm_callback);
    }
}

bool World::mark_character_dead(const std::string& name) {
    for (auto& character : characters_) {
        if (character.name == name) {
            character.dead = true;
            character.scene_ids.clear();
            return true;
        }
    }
    return false;
}

nlohmann::json World::to_json() const {
    nlohmann::json j;
    j["world_graph"]    = world_graph_.to_json();
    j["characters"]     = characters_;

    nlohmann::json cm_j;
    for (const auto& [name, mem] : character_memories_)
        cm_j[name] = mem.to_json();
    j["character_memories"] = std::move(cm_j);
    return j;
}

World World::from_json(const nlohmann::json& j) {
    World w;
    if (j.contains("world_graph")) {
        w.world_graph_ = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        w.world_graph_ = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    }
    if (j.contains("characters") && j["characters"].is_array())
        w.characters_ = j["characters"].get<std::vector<Character>>();
    if (j.contains("character_memories") && j["character_memories"].is_object()) {
        for (auto& [name, cm_j] : j["character_memories"].items())
            w.character_memories_.insert_or_assign(name, CharacterMemory::from_json(cm_j));
    }
    return w;
}

} // namespace rhapsode
