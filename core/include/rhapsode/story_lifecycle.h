#pragma once

#include <optional>
#include <string>
#include <vector>

#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"

namespace rhapsode {

struct TurnServices;

struct LifecycleApplyResult {
    int applied = 0;
    std::optional<std::string> merged_into;
};

SceneData* fork_story_scene(
    StoryData& data, TurnServices& services, const std::string& parent_id,
    const std::string& new_id, const std::vector<std::string>& cast,
    const std::string& driving_intention);
bool conclude_story_scene(
    StoryData& data, const std::string& id, const std::string& reason);
bool merge_story_scene(
    StoryData& data, TurnServices& services, const std::string& from_id,
    const std::string& into_id);

LifecycleApplyResult apply_lifecycle_decision(
    StoryData& data, TurnServices& services,
    const std::string& advanced_scene_id,
    const LifecycleDecision& decision);

}  // namespace rhapsode
