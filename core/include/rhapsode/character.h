#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace rhapsode {

struct Character
{
    std::string name;
    std::string description;
    bool is_player = false;
};

void to_json(nlohmann::json& j, const Character& c);
void from_json(const nlohmann::json& j, Character& c);

} // namespace rhapsode
