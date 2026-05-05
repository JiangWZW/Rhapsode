#include "rhapsode/scene.h"
#include <fstream>
#include <stdexcept>

namespace rhapsode {

nlohmann::json Scene::to_json() const {
    nlohmann::json j;
    j["title"] = title;
    j["system_prompt"] = system_prompt;
    j["characters"] = characters;
    j["history"] = history;
    return j;
}

Scene Scene::from_json(const nlohmann::json& j) {
    Scene s;
    j.at("title").get_to(s.title);
    j.at("system_prompt").get_to(s.system_prompt);
    j.at("characters").get_to(s.characters);
    if (j.contains("history")) {
        s.history = j.at("history").get<History>();
    }
    if (j.contains("seed_messages")) {
        for (const auto& msg : j.at("seed_messages")) {
            s.history.append(msg.get<SceneMessage>());
        }
    }
    return s;
}

Scene Scene::load_json(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Cannot open scene file: " + path);
    }
    nlohmann::json j;
    in >> j;
    return Scene::from_json(j);
}

void Scene::save_json(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot write scene file: " + path);
    }
    out << to_json().dump(2);
}

} // namespace rhapsode
