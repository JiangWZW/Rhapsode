#include "rhapsode/world.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace rhapsode {

namespace {

constexpr std::size_t kMonologueTakeCap = 2000;

void fill_missing_profile(Character& existing, Character& incoming) {
    if (existing.description.empty() && !incoming.description.empty())
        existing.description = std::move(incoming.description);
    if (existing.dialogue_instructions.empty() &&
        !incoming.dialogue_instructions.empty()) {
        existing.dialogue_instructions =
            std::move(incoming.dialogue_instructions);
    }
}

void remove_dynamic_characters(std::vector<Character>& characters,
                               int target_turn) {
    characters.erase(
        std::remove_if(characters.begin(), characters.end(),
            [target_turn](const Character& character) {
                return character.created_at >= target_turn;
            }),
        characters.end());
}

void remove_orphan_memories(
    std::unordered_map<std::string, CharacterMemory>& memories,
    const std::vector<Character>& characters) {
    for (auto it = memories.begin(); it != memories.end();) {
        const bool exists = std::any_of(
            characters.begin(), characters.end(),
            [&](const Character& character) {
                return character.name == it->first;
            });
        it = exists ? std::next(it) : memories.erase(it);
    }
}

}  // namespace

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
        fill_missing_profile(*existing, character);
        if (was_off)
            log() << "  [cast] " << existing->name
                  << " re-enters (was off-stage)\n";
        return *existing;
    }

    return add_new_character(scene_id, std::move(character));
}

Character& World::add_new_character(const std::string& scene_id,
                                    Character character) {
    if (!scene_id.empty()) character.join_scene(scene_id);
    characters_.push_back(std::move(character));
    Character& added = characters_.back();
    log() << "  [cast] NEW " << added.name
          << " | role=" << (added.role.empty() ? "?" : added.role)
          << " | \"" << added.description.substr(0, 80) << "\"\n";

    if (!added.is_player && character_memories_.find(added.name) == character_memories_.end()) {
        CharacterMemory memory(added.name);
        const std::string core_seed =
            !str::trim(added.core).empty() ? added.core : added.description;
        memory.ensure_bootstrap(core_seed);
        character_memories_.emplace(added.name, std::move(memory));
        log() << "  [cast]   memory created (core+bootstrap stream)\n";
    }
    return added;
}

bool World::leave_character(const std::string& scene_id, const std::string& name) {
    Character* character = find_character_mutable(name);
    if (!character || !character->in_scene(scene_id)) return false;
    character->leave_scene(scene_id);
    return true;
}

void World::add_scene_characters(
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
    if (const Character* character = find_character(memory.name())) {
        const std::string core_seed =
            !str::trim(character->core).empty() ? character->core
                                                 : character->description;
        memory.ensure_bootstrap(core_seed);
    }
    character_memories_.insert_or_assign(memory.name(), std::move(memory));
}

std::uint64_t World::seed_character_intention(
    const std::string& character,
    const std::string& intention,
    const std::vector<std::string>& subjects,
    int created_at) {
    const Character* canonical = find_character(character);
    if (!canonical) return 0;
    auto it = character_memories_.find(canonical->name);
    if (it == character_memories_.end()) return 0;
    return it->second.seed_belief(
        intention, subjects, created_at,
        CharacterMemory::kAuthoredSeedWeight, "intention");
}

bool World::expire_character_intention(const std::string& character,
                                       std::uint64_t node_id,
                                       int valid_until) {
    const Character* canonical = find_character(character);
    if (!canonical || node_id == 0) return false;
    auto it = character_memories_.find(canonical->name);
    return it != character_memories_.end() &&
        it->second.expire_intention(node_id, valid_until);
}

std::vector<std::uint64_t> World::revert_to_turn(int target_turn) {
    const auto removed_ids = world_graph_.revert_to_turn(target_turn);

    remove_dynamic_characters(characters_, target_turn);
    for (auto& character : characters_)
        if (!character.is_player) character.dead = false;

    remove_orphan_memories(character_memories_, characters_);
    return removed_ids;
}

void World::append_objective_takes(const std::string& scene_id,
                                   int turn,
                                   const std::string& take_text) {
    const std::string take = str::trim(take_text);
    if (take.empty()) {
        log() << "  [journal] take t=" << turn << " empty -- skipped\n"
              << std::flush;
        return;
    }
    log() << "  [journal] take t=" << turn << " chars=" << take.size()
          << "\n" << std::flush;
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        it->second.append_objective(turn, "take", take);
    }
}

void World::update_objective_journals(const std::string& scene_id,
                                      int turn,
                                      const LLMCallback& llm_callback) {
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        const std::string who =
            !str::trim(character.core).empty() ? character.core
                                               : character.description;
        it->second.update_objective_journal(turn, who, llm_callback);
    }
}

void World::update_monologues(const std::string& scene_id,
                              int turn,
                              const LLMCallback& llm_callback) {
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        std::string take;
        std::string seen;
        for (const auto& line : it->second.objective_journal()) {
            if (line.turn != turn) continue;
            if (line.kind == "take") {
                if (!take.empty()) take += '\n';
                take += line.text;
            } else if (line.kind == "seen") {
                if (!seen.empty()) seen += '\n';
                seen += line.text;
            }
        }
        if (take.size() > kMonologueTakeCap)
            take = truncate_utf8(take, kMonologueTakeCap);
        std::string tail = take;
        if (!seen.empty()) {
            if (!tail.empty()) tail += '\n';
            tail += seen;
        }
        it->second.update_monologues(
            turn,
            !str::trim(character.core).empty() ? character.core
                                               : character.description,
            tail, llm_callback,
            character.build_prompt__dialogue_voice());
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
    j["state_version"]  = state_version_;
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
    w.state_version_ = j.value("state_version", std::uint64_t{0});
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
    for (const auto& character : w.characters_) {
        if (!character.is_player &&
            w.character_memories_.find(character.name) ==
                w.character_memories_.end()) {
            CharacterMemory memory(character.name);
            const std::string core_seed =
                !str::trim(character.core).empty() ? character.core
                                                   : character.description;
            memory.ensure_bootstrap(core_seed);
            w.character_memories_.emplace(character.name, std::move(memory));
        } else if (!character.is_player) {
            auto it = w.character_memories_.find(character.name);
            if (it != w.character_memories_.end()) {
                const std::string core_seed =
                    !str::trim(character.core).empty() ? character.core
                                                       : character.description;
                it->second.ensure_bootstrap(core_seed);
            }
        }
    }
    return w;
}

} // namespace rhapsode
