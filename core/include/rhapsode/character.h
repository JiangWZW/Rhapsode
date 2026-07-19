#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

namespace rhapsode {

struct Character
{
    std::string name;
    std::string description;
    std::string dialogue_instructions;
    std::vector<std::string> example_dialogue;
    std::string role;
    bool is_player = false;
    // The storylines (scene ids) this character is currently part of. A
    // character can inhabit several scenes at once, and this set mutates as the
    // story forks and merges. Empty == not on stage anywhere.
    std::vector<std::string> scene_ids;
    bool dead = false;
    // Authored/bootstrap characters predate turn 0. Dynamic NPCs use the
    // zero-based turn on which they entered, so undo/rollback can prune them.
    int created_at = -1;

    Character() = default;
    Character(std::string n, std::string d, bool player = false)
        : name(std::move(n)), description(std::move(d)), is_player(player) {}

    /// Is this character part of the given storyline right now?
    bool in_scene(const std::string& scene_id) const {
        return std::find(scene_ids.begin(), scene_ids.end(), scene_id) != scene_ids.end();
    }
    /// Active in at least one storyline. Prefer in_scene() for scene-scoped checks.
    bool on_stage() const { return !scene_ids.empty(); }
    /// Add this character to a storyline (idempotent).
    void join_scene(const std::string& scene_id) {
        if (!in_scene(scene_id)) scene_ids.push_back(scene_id);
    }
    /// Remove this character from a storyline (no-op if absent).
    void leave_scene(const std::string& scene_id) {
        scene_ids.erase(std::remove(scene_ids.begin(), scene_ids.end(), scene_id),
                        scene_ids.end());
    }

    /// Prompt text: dialogue_instructions + up to two example lines. Empty if none.
    std::string build_prompt__dialogue_voice() const;
};

void to_json(nlohmann::json& j, const Character& c);
void from_json(const nlohmann::json& j, Character& c);

} // namespace rhapsode
