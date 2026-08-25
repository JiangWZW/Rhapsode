#include "rhapsode/world.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace rhapsode {

namespace {

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

void World::apply_ready_perceptions(int turn, const MindReadyFn& ready)
{
    (void)turn;
    for (const auto& character : characters_) {
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        CharacterMemory& memory = it->second;
        const std::size_t handle = std::hash<std::string>{}(character.name);

        int applied_turn = -1;
        int applied_slot = -1;
        std::string raw;
        bool failed = false;
        bool got = false;
        for (int i = 0; i < CharacterMemory::kStagingBuffers; ++i) {
            const int slot =
                (memory.perception_head() - i + CharacterMemory::kStagingBuffers)
                % CharacterMemory::kStagingBuffers;
            if (!memory.perception_pending(slot)) continue;
            if (!ready(handle, slot, memory.perception_generation(slot),
                       raw, failed))
                continue;
            got = true;
            applied_slot = slot;
            applied_turn = memory.perception_claim_turn(slot);
            break;
        }
        if (!got) continue;
        if (failed || character.dead)
            memory.release_perception(applied_slot);
        else {
            memory.apply_perception_json(applied_turn, raw);
            memory.release_perception(applied_slot);
        }
        for (int slot = 0; slot < CharacterMemory::kStagingBuffers; ++slot) {
            if (!memory.perception_pending(slot)) continue;
            if (memory.perception_claim_turn(slot) < applied_turn)
                memory.kill_perception_slot(slot);
        }
    }
}

void World::submit_perceptions(
    const std::string& scene_id,
    int turn,
    const std::string& narration_window,
    const std::function<void(const std::vector<PromptJob>&)>& submit)
{
    if (narration_window.empty()) return;

    std::vector<PromptJob> jobs;
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        CharacterMemory& memory = it->second;
        if (memory.perception_turn() >= turn) continue;
        const int slot = memory.perception_slot_for(turn);
        if (memory.perception_pending(slot) &&
            memory.perception_claim_turn(slot) == turn)
            continue;
        const std::string who =
            !str::trim(character.core).empty() ? character.core
                                               : character.description;
        memory.take_perception_slot(slot, turn);
        jobs.push_back(PromptJob{
            std::hash<std::string>{}(character.name),
            memory.build_perception_prompt(narration_window, who),
            slot,
            memory.perception_generation(slot)});
    }
    if (!jobs.empty()) submit(jobs);
}

void World::poll_perceptions(
    const std::string& scene_id,
    int turn,
    const std::string& narration_window,
    const MindReadyFn& ready,
    const std::function<void(const std::vector<PromptJob>&)>& submit)
{
    apply_ready_perceptions(turn, ready);
    submit_perceptions(scene_id, turn, narration_window, submit);
}

void World::apply_ready_monologues(int turn, const MindReadyFn& ready) {
    (void)turn;
    for (const auto& character : characters_) {
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        CharacterMemory& memory = it->second;
        const std::size_t handle = std::hash<std::string>{}(character.name);

        int applied_turn = -1;
        int applied_slot = -1;
        std::string raw;
        bool failed = false;
        bool got = false;
        for (int i = 0; i < CharacterMemory::kStagingBuffers; ++i) {
            const int slot =
                (memory.monologue_head() - i + CharacterMemory::kStagingBuffers)
                % CharacterMemory::kStagingBuffers;
            if (!memory.monologue_pending(slot)) continue;
            if (!ready(handle, slot, memory.monologue_generation(slot),
                       raw, failed))
                continue;
            got = true;
            applied_slot = slot;
            applied_turn = memory.monologue_claim_turn(slot);
            break;
        }
        if (!got) continue;
        if (failed || character.dead)
            memory.release_monologue(applied_slot);
        else {
            memory.apply_monologue_json(applied_turn, raw);
            memory.release_monologue(applied_slot);
        }
        for (int slot = 0; slot < CharacterMemory::kStagingBuffers; ++slot) {
            if (!memory.monologue_pending(slot)) continue;
            if (memory.monologue_claim_turn(slot) < applied_turn)
                memory.kill_monologue_slot(slot);
        }
    }
}

void World::submit_catchup_monologues(
    const std::string& scene_id,
    const std::function<void(const std::vector<PromptJob>&)>& submit)
{
    std::vector<PromptJob> jobs;
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        CharacterMemory& memory = it->second;
        const int beat = memory.perception_turn();
        if (beat < 0) continue;
        if (memory.monologue_turn() >= beat) continue;
        const int slot = memory.monologue_slot_for(beat);
        if (memory.monologue_pending(slot) &&
            memory.monologue_claim_turn(slot) == beat)
            continue;

        const std::string who =
            !str::trim(character.core).empty() ? character.core
                                               : character.description;
        std::string prompt = memory.build_monologue_prompt(who);
        memory.take_monologue_slot(slot, beat);
        jobs.push_back(PromptJob{
            std::hash<std::string>{}(character.name),
            std::move(prompt),
            slot,
            memory.monologue_generation(slot)});
    }
    if (!jobs.empty()) submit(jobs);
}

void World::poll_monologues(
    const std::string& scene_id,
    int turn,
    const MindReadyFn& ready,
    const std::function<void(const std::vector<PromptJob>&)>& submit) {
    apply_ready_monologues(turn, ready);
    submit_catchup_monologues(scene_id, submit);
}

void World::end_mind_turn(const std::string& scene_id, int turn) {
    for (const auto& character : characters_) {
        if (character.is_player || character.dead ||
            !character.in_scene(scene_id)) {
            continue;
        }
        auto it = character_memories_.find(character.name);
        if (it == character_memories_.end()) continue;
        it->second.end_mind_turn(turn);
    }
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

void World::update_perceptions(const std::string& scene_id,
                               int turn,
                               const std::string& narration_window,
                               const LLMCallback& llm_callback) {
    if (narration_window.empty()) {
        log_info("perception") << "t=" << turn << " empty -- skipped\n"
              << std::flush;
        return;
    }
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
        it->second.update_perception(turn, who, narration_window, llm_callback);
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
        it->second.update_monologues(
            turn,
            !str::trim(character.core).empty() ? character.core
                                               : character.description,
            llm_callback);
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
