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
#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"
#include "rhapsode/turn_pipeline.h"
#include "rhapsode/world.h"

namespace rhapsode {

class MemorySystem;
struct TurnResult;
struct WeaveResult;

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

    const World& world() const { return data_.world; }
    const WorldGraph& observations() const { return data_.observations; }
    std::uint64_t state_version() const {
        return data_.transaction_version;
    }
    World world_snapshot() const;

    SceneData* get_scene(const std::string& id);
    const SceneData* get_scene(const std::string& id) const;
    std::vector<std::string> scene_ids() const;
    size_t scene_count() const { return data_.scenes.size(); }

    const std::string& active_scene_id() const { return data_.active_scene_id; }
    void set_active_scene(const std::string& id);
    SceneData* active_scene() { return get_scene(data_.active_scene_id); }
    const SceneData* active_scene() const {
        return get_scene(data_.active_scene_id);
    }

    SceneData* fork_scene(const std::string& parent_id,
                          const std::string& new_id,
                          const std::vector<std::string>& cast,
                          const std::string& driving_intention = "");
    bool conclude_scene(const std::string& id, const std::string& reason);
    bool merge_scene(const std::string& from_id, const std::string& into_id);

    void note_advanced(const std::string& scene_id);
    int turn_clock() const { return data_.turn_clock; }
    int beat_clock() const { return turn_clock(); }
    std::string tool_list_scenes() const;
    std::string dispatch_tool(const std::string& scene_id,
                              const std::string& name,
                              const std::string& args_json);

    std::vector<SceneMessage> display_timeline(
        const std::string& scene_id,
        std::optional<size_t> cap = std::nullopt) const;

    std::string render_transcript() const;

    void set_llm_callback(LLMCallback cb);
    void set_narrator_llm_callback(NarratorLLMCallback cb);
    void set_weaver_llm_callback(LLMCallback cb);
    void set_weaver_interval(int turns);
    WeaveResult weave_scene(const std::string& scene_id);
    void set_history_window(size_t normal, size_t resume);
    void set_scheduler_callback(SchedulerCallback cb) {
        services_.scheduler = std::move(cb);
    }
    void set_lifecycle_callback(LifecycleCallback cb) {
        services_.lifecycle = std::move(cb);
    }
    void set_downsampler_callback(LLMCallback cb);
    void set_reflection_llm_callback(LLMCallback cb);
    void set_observation_llm_callback(LLMCallback cb);
    void set_memory(std::shared_ptr<MemorySystem> memory) {
        services_.memory = std::move(memory);
    }
    void set_saves_dir(const std::string& dir) { services_.saves_dir = dir; }

    std::vector<SceneMessage> advance_player(const std::string& player_input);
    std::vector<SceneMessage> complete_turn();
    int revert_active_turns(int count);

    bool has_save(const std::string& saves_dir) const;
    void load_save(const std::string& saves_dir);
    void save(const std::string& saves_dir) const;
    void delete_save(const std::string& saves_dir) const;

private:
    struct PendingTurn {
        std::string scene_id;
        std::string player_input;
        int post_turn_index = -1;
    };

    int revert_scene_turns(SceneData& scene, int count);

    StoryData data_;
    TurnServices services_;
    std::optional<PendingTurn> pending_turn_;
};

}  // namespace rhapsode
