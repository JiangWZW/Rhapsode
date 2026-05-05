#pragma once
#include <string>
#include <vector>
#include "rhapsode/character.h"
#include "rhapsode/history.h"

namespace rhapsode {

class Scene {
public:
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;
    History history;

    static Scene load_json(const std::string& path);
    void save_json(const std::string& path) const;

    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);
};

} // namespace rhapsode
