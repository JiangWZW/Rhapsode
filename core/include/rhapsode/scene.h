#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "rhapsode/character.h"
#include "rhapsode/history.h"
#include "rhapsode/text_downsampler.h"
#include "rhapsode/world.h"

namespace rhapsode {

class MemorySystem;

// One storyline. A Scene owns only ephemeral, per-storyline state (its prose
// threads, downsampler, and clock) and shares the durable substrate through a
// World it co-owns. Forking a storyline makes a new Scene over the same World;
// today there is a single Scene, so it default-creates its own World.
class Scene {
public:
    // -- Static (from scenario file) --
    std::string scene_id;
    std::string title;
    std::string system_prompt;

    // -- Mutable ephemeral state --
    History history;   // user + narrator only (narrator prompt thread)
    History dialogue;  // NPC speech_turn lines (UI replay)
    TextDownsampler downsampler;
    int turn_index = 0;

    // -- Storyline drive (ephemeral; set when this scene is forked) --
    // The authored line this storyline exists to pursue, and its charge. The
    // scheduler reads both through `Story::tool_list_scenes`. A forked scene sets
    // them; the root scene leaves them empty and derives its drive from the
    // highest-charge intention among its cast's minds.
    std::string driving_intention;
    float charge = 0.0f;
    // Story-beat clock at which this scene last advanced; feeds `staleness`.
    int last_advanced = 0;

    Scene() : world_(std::make_shared<World>()) {}

    // -- Shared substrate --
    World& world() { return *world_; }
    const World& world() const { return *world_; }
    std::shared_ptr<World> world_ptr() const { return world_; }
    void set_world(std::shared_ptr<World> w) { world_ = std::move(w); }

    static Scene load_json(const std::string& path);

    // -- Storyline lifecycle --
    /// Fork a new storyline from this one over the SAME World. The child shares
    /// the durable substrate (graph, minds, roster) via the shared_ptr and
    /// starts with its own empty prose threads and a zero clock. Each name in
    /// `cast` joins the child's membership; they stay in this (parent) scene too
    /// unless a later beat moves them out. `title`/`system_prompt` are inherited
    /// and may be overridden by the caller.
    Scene fork(const std::string& new_scene_id,
               const std::vector<std::string>& cast) const;

    // -- Character lifecycle (theatre model, scene-scoped over shared roster) --
    Character& enter_character(Character ch);
    Character* find_on_stage(const std::string& name);
    const Character* find_on_stage(const std::string& name) const;
    bool exit_character(const std::string& name);
    std::vector<DeathCandidate> scan_death_candidates() { return world_->scan_death_candidates(); }

    /// Entity names/descriptions an on-stage NPC's mind may view this turn.
    /// Prompt text: `### Cast` section lines (header + per-NPC lines). Empty if none.
    std::vector<std::string> build_prompt__cast() const;

    /// Chronological merge of history + dialogue for UI replay (optional tail cap).
    std::vector<SceneMessage> display_timeline(std::optional<size_t> cap = std::nullopt) const;

    // -- Narrator tool-use queries --
    std::string tool_query_graph(const std::string& query) const { return world_->tool_query_graph(query); }
    std::string tool_query_mind(const std::string& character) const { return world_->tool_query_mind(character); }
    /// Search this scene's raw history by keyword. Returns matching snippets as JSON.
    std::string tool_query_history(const std::string& query) const;

    // -- Undo --
    int revert_turns(int n);

    // -- System references (forward to the shared World) --
    void set_memory(MemorySystem* mem) { world_->set_memory(mem); }
    MemorySystem* memory() const { return world_->memory(); }

    // -- Persistence (game state: world.json + per-scene blob) --
    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

    // Per-scene ephemeral blob only (no world.json). Used by Story, which saves
    // the shared World once and each scene's blob alongside it.
    void save_ephemeral(const std::string& saves_dir) const;
    void load_ephemeral(const std::string& saves_dir);
    bool has_ephemeral_save(const std::string& saves_dir) const;

    // -- Scenario-format serialization (existing, for tests) --
    void save_json(const std::string& path) const;
    nlohmann::json to_json() const;
    static Scene from_json(const nlohmann::json& j);

private:
    std::shared_ptr<World> world_;
    std::string save_path(const std::string& saves_dir) const;
};

} // namespace rhapsode
