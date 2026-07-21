#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"

namespace rhapsode {

using SchedulerCallback = std::function<std::string(
    const std::string& instructions, const std::string& user,
    const ReadToolCallback& read_tool)>;
using LifecycleCallback = std::function<std::string(
    const std::string& instructions, const std::string& user)>;

struct SceneSummary {
    std::string scene_id;
    std::string title;
    bool active = false;
    int turn_index = 0;
    int staleness = 0;
    std::vector<std::string> cast;
    bool player_present = false;
    std::string driving_intention;
    float charge = 0.0f;
    std::string last_narration;
};

struct BeatSummary {
    std::string scene_id;
    std::string title;
    std::vector<std::string> cast;
    bool player_present = false;
    std::optional<std::string> player_action;
    std::string narration;
    std::vector<SceneSummary> storylines;
};

struct LifecycleDecision {
    struct Fork {
        std::vector<std::string> cast;
        std::string driving_intention;
    };

    std::optional<std::string> conclude_reason;
    std::optional<std::string> merge_into;
    std::optional<Fork> fork;
    std::vector<std::string> exited;
};

std::string serialize_scene_summaries(
    const std::vector<SceneSummary>& summaries);
std::string request_off_stage_scene(const SchedulerCallback& callback,
                                    const ReadToolCallback& read_tool);
std::optional<LifecycleDecision> request_lifecycle_decision(
    const BeatSummary& summary, const LifecycleCallback& callback);
std::string build_autonomous_cue(const SceneSummary& summary);

}  // namespace rhapsode
