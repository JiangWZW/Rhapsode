#include "rhapsode/scene.h"
#include "rhapsode/memory_system.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace rhapsode {

// ── Scenario-format serialization (unchanged) ──

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
    if (!in.is_open())
        throw std::runtime_error("Cannot open scene file: " + path);

    nlohmann::json j;
    in >> j;

    Scene s = Scene::from_json(j);

    s.scene_id = std::filesystem::path(path).stem().string();

    nlohmann::json graph_j;
    graph_j["next_id"] = 1;
    graph_j["nodes"] = j.value("nodes", nlohmann::json::array());
    graph_j["edges"] = j.value("edges", nlohmann::json::array());
    s.world_graph = WorldGraph::from_json(graph_j);

    return s;
}

void Scene::save_json(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open())
        throw std::runtime_error("Cannot write scene file: " + path);
    out << to_json().dump(2);
}

// ── Persistence (game state format) ──

std::string Scene::save_path(const std::string& saves_dir) const {
    return saves_dir + "/" + scene_id + ".json";
}

bool Scene::has_save(const std::string& saves_dir) const {
    return std::filesystem::exists(save_path(saves_dir));
}

void Scene::load_save(const std::string& saves_dir) {
    std::ifstream in(save_path(saves_dir));
    if (!in.is_open())
        throw std::runtime_error("No save for scene: " + scene_id);

    nlohmann::json j;
    in >> j;

    turn_index = j.value("turn_index", 0);
    if (j.contains("world_graph")) {
        world_graph = WorldGraph::from_json(j.at("world_graph"));
    } else if (j.contains("node_pool")) {
        world_graph = WorldGraph::from_legacy_node_pool_json(j.at("node_pool"));
    } else {
        throw std::runtime_error("Save is missing world_graph/node_pool data");
    }
    history    = j.at("history").get<History>();

    if (memory_)
        memory_->set_next_id(j.value("memory_next_id", 0));
}

void Scene::save(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);

    nlohmann::json j;
    j["scene_id"]       = scene_id;
    j["turn_index"]     = turn_index;
    j["memory_next_id"] = memory_ ? memory_->get_next_id() : 0;
    j["world_graph"]    = world_graph.to_json();
    j["history"]        = history;

    std::ofstream out(save_path(saves_dir));
    if (!out.is_open())
        throw std::runtime_error("Cannot write save for: " + scene_id);
    out << j.dump(2);
}

void Scene::delete_save(const std::string& saves_dir) const {
    std::filesystem::remove(save_path(saves_dir));
}

} // namespace rhapsode
