#pragma once
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
    bool on_stage = false;
    bool dead = false;
    int created_at = 0;

    Character() = default;
    Character(std::string n, std::string d, bool player = false)
        : name(std::move(n)), description(std::move(d)), is_player(player) {}

    /// Prompt text: dialogue_instructions + up to two example lines. Empty if none.
    std::string build_prompt__dialogue_voice() const;
};

void to_json(nlohmann::json& j, const Character& c);
void from_json(const nlohmann::json& j, Character& c);

} // namespace rhapsode
