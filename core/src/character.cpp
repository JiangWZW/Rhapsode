#include "rhapsode/character.h"

namespace rhapsode {

void to_json(nlohmann::json& j, const Character& c)
{
    j = nlohmann::json{
        {"name", c.name},
        {"description", c.description},
        {"is_player", c.is_player}
    };
    if (!c.dialogue_instructions.empty())
        j["dialogue_instructions"] = c.dialogue_instructions;
    if (!c.example_dialogue.empty())
        j["example_dialogue"] = c.example_dialogue;
    if (!c.role.empty())
        j["role"] = c.role;
    j["on_stage"] = c.on_stage;
    j["dead"] = c.dead;
    j["created_at"] = c.created_at;
}

void from_json(const nlohmann::json& j, Character& c)
{
    j.at("name").get_to(c.name);
    j.at("description").get_to(c.description);
    c.is_player = j.value("is_player", false);
    c.dialogue_instructions = j.value("dialogue_instructions", "");
    c.example_dialogue = j.value("example_dialogue", std::vector<std::string>{});
    c.role = j.value("role", "");
    c.on_stage = j.value("on_stage", c.is_player);
    c.dead = j.value("dead", false);
    c.created_at = j.value("created_at", 0);
}

} // namespace rhapsode
