#pragma once

#include <optional>
#include <string>
#include <vector>

#include "rhapsode/read_tools.h"
#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"

namespace rhapsode {

void import_world(StoryData& data, World world);
World snapshot_world(const StoryData& data);

SceneData* find_scene(StoryData& data, const std::string& id);
const SceneData* find_scene(const StoryData& data, const std::string& id);
SceneData* adopt_scene(StoryData& data, SceneData scene);
std::vector<std::string> collect_scene_ids(const StoryData& data);
void note_scene_advanced(StoryData& data, const std::string& scene_id);

std::optional<std::vector<std::string>> resolve_non_player_members(
    const World& world, const std::string& scene_id,
    const std::vector<std::string>& names);
std::optional<std::vector<std::string>> resolve_fork_cast(
    const World& world, const std::string& parent_id,
    const std::vector<std::string>& names,
    const std::string& driving_intention);
bool scene_retains_living_character(
    const World& world, const std::string& scene_id,
    const std::vector<std::string>& leaving);

SceneSummary summarize_story_scene(
    const StoryData& data, const SceneData& scene);
std::vector<SceneSummary> summarize_story_scenes(const StoryData& data);
TurnSummary summarize_completed_turn(
    const StoryData& data, const SceneData& scene,
    const std::string& player_input);

ReadToolContext make_story_read_tool_context(
    const StoryData& data, const std::string& scene_id);
ReadToolLease make_frozen_story_read_tools(
    const StoryData& data, const std::string& scene_id);

std::vector<std::string> select_off_stage_scenes(
    const StoryData& data, const SchedulerCallback& scheduler);
std::string make_autonomous_turn_cue(
    const StoryData& data, const std::string& scene_id);

}  // namespace rhapsode
