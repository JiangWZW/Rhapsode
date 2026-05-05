#include "rhapsode/character.h"

namespace rhapsode {

void to_json(nlohmann::json& j, const Character& c)
{
    j = nlohmann::json{
        {"name", c.name},
        {"description", c.description},
        {"is_player", c.is_player}
    };
}

void from_json(const nlohmann::json& j, Character& c)
{
    j.at("name").get_to(c.name);
    j.at("description").get_to(c.description);
    if (j.contains("is_player")) {
        j.at("is_player").get_to(c.is_player);
    } else {
        c.is_player = false;
    }
}

} // namespace rhapsode
