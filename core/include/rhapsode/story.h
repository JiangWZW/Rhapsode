#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/llm_callback.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/storyline_policy.h"
#include "rhapsode/world.h"

namespace rhapsode {

class TurnExecutor;
class Director;
class Weaver;
class MemorySystem;
struct TurnResult;
struct WeaveResult;

// Runtime hub for one playthrough. Story exclusively owns one World, a stable
// collection of World-free SceneData records, and the TurnExecutor that executes
// turns over them.
class Story {
public:
    Story();
    ~Story();

    Story(const Story&) = delete;
    Story& operator=(const Story&) = delete;
    Story(Story&&) noexcept;
    Story& operator=(Story&&) noexcept;

    static Story load_scenario(const std::string& path);
    static Story from_scenario_json(const nlohmann::json& scenario,
                                    const std::string& scene_id = {});
    static Story from_data(SceneData root, World world = {});
    nlohmann::json to_scenario_json(const std::string& scene_id) const;

    World& world() { return *world_; }
    const World& world() const { return *world_; }

    SceneData* get_scene(const std::string& id);
    const SceneData* get_scene(const std::string& id) const;
    std::vector<std::string> scene_ids() const;
    size_t scene_count() const { return scenes_.size(); }

    const std::string& active_scene_id() const { return active_scene_id_; }
    void set_active_scene(const std::string& id);
    SceneData* active_scene() { return get_scene(active_scene_id_); }
    const SceneData* active_scene() const { return get_scene(active_scene_id_); }

    SceneData* fork_scene(const std::string& parent_id,
                          const std::string& new_id,
                          const std::vector<std::string>& cast,
                          const std::string& driving_intention = "");
    bool conclude_scene(const std::string& id, const std::string& reason);
    bool merge_scene(const std::string& from_id, const std::string& into_id);

    void note_advanced(const std::string& scene_id);
    int beat_clock() const { return beat_clock_; }
    std::string tool_list_scenes() const;
    std::string dispatch_tool(const std::string& scene_id,
                              const std::string& name,
                              const std::string& args_json);

    std::vector<SceneMessage> display_timeline(
        const std::string& scene_id,
        std::optional<size_t> cap = std::nullopt) const;

    // Runtime configuration. Story forwards turn-service configuration to its
    // owned TurnExecutor; Python never owns or wires those services separately.
    void set_llm_callback(LLMCallback cb);
    void set_narrator_llm_callback(NarratorLLMCallback cb);
    void set_weaver_llm_callback(LLMCallback cb);
    void set_weaver_local_llm_callback(LLMCallback cb);
    void set_weaver_interval(int turns);
    WeaveResult weave_scene(const std::string& scene_id);
    void set_history_window(size_t normal, size_t resume);
    void set_resuming(bool value);
    void set_scheduler_callback(SchedulerCallback cb) { scheduler_cb_ = std::move(cb); }
    void set_lifecycle_callback(LifecycleCallback cb) { lifecycle_cb_ = std::move(cb); }
    void set_downsampler_callback(LLMCallback cb);
    void set_reflection_llm_callback(LLMCallback cb);
    void set_memory(std::shared_ptr<MemorySystem> memory) {
        memory_ = std::move(memory);
    }
    void set_saves_dir(const std::string& dir) { saves_dir_ = dir; }

    std::vector<SceneMessage> advance_scene(const std::string& player_input);
    int revert_active_turns(int count);

    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

private:
    struct SceneDrive {
        std::string intention;
        float charge = 0.0f;
    };

    SceneData* adopt_scene(SceneData scene);
    SceneDrive derive_intention(const SceneData& scene) const;
    SceneSummary summarize_scene(const SceneData& scene) const;
    std::vector<SceneSummary> summarize_scenes() const;
    BeatSummary build_beat_summary(const SceneData& scene,
                                   const std::string& player_input) const;
    std::vector<std::string> without_player_characters(
        const std::vector<std::string>& names) const;
    std::string pick_off_stage_scene();
    int apply_lifecycle(const std::string& scene_id, const std::string& player_input);
    std::string make_autonomous_cue(const std::string& scene_id) const;
    void sync_memory(const TurnResult& result);
    void advance_off_stage_scene();
    int revert_scene_turns(SceneData& scene, int count);

    // Stable allocation keeps TurnExecutor's World reference valid through Story
    // moves and save loads. SceneData does not participate in this ownership.
    std::unique_ptr<World> world_;
    std::vector<std::unique_ptr<SceneData>> scenes_;
    std::string active_scene_id_;
    int beat_clock_ = 0;

    SchedulerCallback scheduler_cb_;
    LifecycleCallback lifecycle_cb_;
    std::string saves_dir_;
    std::shared_ptr<MemorySystem> memory_;

    // Stable allocations keep graph-service references valid through Story moves.
    // Declaration order destroys the loop before its injected services and World.
    std::unique_ptr<Director> director_;
    std::unique_ptr<Weaver> weaver_;
    std::unique_ptr<TurnExecutor> executor_;
};

}  // namespace rhapsode
