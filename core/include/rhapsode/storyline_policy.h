#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
#include "rhapsode/scene_message.h"

namespace rhapsode {

using SchedulerCallback = std::function<std::string(
    const std::string& instructions, const std::string& user,
    const ReadToolCallback& read_tool)>;
using LifecycleCallback = std::function<std::string(
    const std::string& instructions, const std::string& user,
    const ReadToolCallback& read_tool)>;

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
    std::string recent_narration;
};

struct TurnSummary {
    std::string scene_id;
    std::string title;
    std::vector<std::string> cast;
    bool player_present = false;
    std::optional<std::string> player_action;
    std::string narration;
    std::vector<SceneMessage> dialogue;
    std::vector<SceneSummary> storylines;
};

struct LifecycleOp {
    enum class Kind { Merge, Conclude, Fork, Exit };

    Kind kind = Kind::Exit;
    std::string from;                 // merge
    std::string into;                 // merge
    std::string scene_id;             // conclude / exit / fork parent
    std::string reason;               // conclude / merge
    std::vector<std::string> cast;    // fork / exit
    std::string driving_intention;    // fork
};

struct LifecycleDecision {
    std::vector<LifecycleOp> ops;
};

std::string serialize_scene_summaries(
    const std::vector<SceneSummary>& summaries);
std::string format_live_storylines_board(
    const std::vector<SceneSummary>& summaries);
std::string request_off_stage_scene(const SchedulerCallback& callback,
                                    const ReadToolCallback& read_tool);
std::optional<LifecycleDecision> request_lifecycle_decision(
    const TurnSummary& summary, const LifecycleCallback& callback,
    const ReadToolCallback& read_tool);
std::string build_autonomous_cue(const SceneSummary& summary);

}  // namespace rhapsode
