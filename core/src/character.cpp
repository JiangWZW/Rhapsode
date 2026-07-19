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
    j["scene_ids"] = c.scene_ids;
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
    // New format stores explicit scene membership. The legacy authored/save
    // `on_stage` bool carries no scene id here; Scene::load_json resolves that
    // hint into membership once the scene id is known.
    c.scene_ids = j.value("scene_ids", std::vector<std::string>{});
    c.dead = j.value("dead", false);
    c.created_at = j.value("created_at", -1);
}

std::string Character::build_prompt__dialogue_voice() const {
    std::string out;
    if (!dialogue_instructions.empty())
        out += "  Voice: " + dialogue_instructions + "\n";
    if (!example_dialogue.empty()) {
        out += "  Example lines:\n";
        int shown = 0;
        for (const auto& ex : example_dialogue) {
            out += "    - " + ex + "\n";
            if (++shown >= 2) break;
        }
    }
    return out;
}

} // namespace rhapsode
