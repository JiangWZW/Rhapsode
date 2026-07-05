#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/history.h"
#include "rhapsode/text_downsampler.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {

class MemorySystem;

struct DeathCandidate {
    std::string character_name;
    std::vector<std::string> evidence;
};

class Scene {
public:
    // -- Static (from scenario file) --
    std::string scene_id;
    std::string title;
    std::string system_prompt;
    std::vector<Character> characters;

    // -- Mutable game state --
    History history;   // user + narrator only (narrator prompt thread)
    History dialogue;  // NPC speech_turn lines (UI replay)
    WorldGraph world_graph;
    TextDownsampler downsampler;
    std::unordered_map<std::string, CharacterMemory> character_memories;
    int turn_index = 0;

    static Scene load_json(const std::string& path);

    // -- Character lifecycle (theatre model) --
    Character& enter_character(Character ch);
    Character* find_on_stage(const std::string& name);
    const Character* find_on_stage(const std::string& name) const;
    bool exit_character(const std::string& name);
    std::vector<DeathCandidate> scan_death_candidates();

    /// Entity names/descriptions an on-stage NPC's mind may view this turn.
    /// Prompt text: `### Cast` section lines (header + per-NPC lines). Empty if none.
    std::vector<std::string> build_prompt__cast() const;

    /// Chronological merge of history + dialogue for UI replay (optional tail cap).
    std::vector<SceneMessage> display_timeline(std::optional<size_t> cap = std::nullopt) const;

    // -- Narrator tool-use queries (called from Python during tool-use loop) --
    /// Search world graph by entity name or free text. Returns entity-timeline
    /// chains (for entity matches) or matching nodes with chain predecessors
    /// (for text matches), as a JSON string.
    std::string tool_query_graph(const std::string& query) const;

    /// Get a character's thoughts, beliefs, and dialogue voice as JSON.
    std::string tool_query_mind(const std::string& character) const;

    /// Search raw history by keyword. Returns matching snippets as JSON.
    std::string tool_query_history(const std::string& query) const;

    // -- Undo --
    int revert_turns(int n);

    // -- System references --
    void set_memory(MemorySystem* mem) { memory_ = mem; }
    MemorySystem* memory() const { return memory_; }

    // -- Persistence (game state) --
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

    // -- Scenario-format serialization (existing, for tests) --
    void save_json(const std::string& path) const;
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);

private:
    MemorySystem* memory_ = nullptr;
    std::string save_path(const std::string& saves_dir) const;
};

} // namespace rhapsode
