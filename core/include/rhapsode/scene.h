#pragma once
#include <string>
#include <vector>
#include "rhapsode/character.h"
#include "rhapsode/history.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

class MemorySystem;

class Scene {
public:
    // ── Static (from scenario file) ──
    std::string scene_id;
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;

    // ── Mutable game state ──
    History history;
    WorldGraph world_graph;
    int turn_index = 0;

    static Scene load_json(const std::string& path);

    // ── System references ──
    void set_memory(MemorySystem* mem) { memory_ = mem; }

    // ── Persistence (game state) ──
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

    // ── Scenario-format serialization (existing, for tests) ──
    void save_json(const std::string& path) const;
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);

private:
    MemorySystem* memory_ = nullptr;
    std::string save_path(const std::string& saves_dir) const;
};

} // namespace rhapsode
