#include "rhapsode/story_data_ops.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

constexpr int kMaxOffStagePerTurn = 2;
constexpr int kStarvationTurns = 3;

struct SceneDrive {
    std::string intention;
    float charge = 0.0f;
};

struct FrozenReadToolState {
    World world;
    WorldGraph observations;
    std::unordered_map<std::string, SceneData> scenes;
    std::string scene_summaries_json;

    explicit FrozenReadToolState(const StoryData& data)
        : world(data.world),
          observations(data.observations),
          scene_summaries_json(
              serialize_scene_summaries(summarize_story_scenes(data))) {
        for (const auto& scene : data.scenes)
            scenes.emplace(scene->scene_id, *scene);
    }

    ReadToolContext context_for(const std::string& scene_id) const {
        std::unordered_map<std::string, const SceneData*> scenes_by_id;
        for (const auto& entry : scenes)
            scenes_by_id.emplace(entry.first, &entry.second);
        return make_read_tool_context(
            world, observations, scene_id, scene_summaries_json,
            std::move(scenes_by_id));
    }
};

SceneDrive derive_scene_drive(
    const StoryData& data, const SceneData& scene) {
    SceneDrive drive{scene.driving_intention, scene.charge};
    for (const auto& character : data.world.characters()) {
        if (!character.in_scene(scene.scene_id)) continue;
        const auto it =
            data.world.character_memories().find(character.name);
        if (it == data.world.character_memories().end()) continue;
        it->second.beliefs().for_each([&](const Node& node) {
            if (node.type == "intention" && node.state == NodeState::Active &&
                node.weight > drive.charge) {
                drive.charge = node.weight;
                drive.intention = node.fact;
            }
        }, false);
    }
    return drive;
}

std::vector<std::string> split_scene_picks(const std::string& raw) {
    std::vector<std::string> picks;
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string id = str::trim(line);
        if (!id.empty()) picks.push_back(id);
    }
    if (picks.empty()) {
        const std::string id = str::trim(raw);
        if (!id.empty()) picks.push_back(id);
    }
    return picks;
}

}  // namespace

void import_world(StoryData& data, World world) {
    data.transaction_version = world.state_version();
    data.observations = world.take_graph();
    world.set_state_version(0);
    data.world = std::move(world);
}

World snapshot_world(const StoryData& data) {
    World world = data.world;
    world.set_graph(data.observations);
    world.set_state_version(data.transaction_version);
    return world;
}

SceneData* find_scene(StoryData& data, const std::string& id) {
    for (auto& scene : data.scenes)
        if (scene->scene_id == id) return scene.get();
    return nullptr;
}

const SceneData* find_scene(const StoryData& data, const std::string& id) {
    for (const auto& scene : data.scenes)
        if (scene->scene_id == id) return scene.get();
    return nullptr;
}

SceneData* adopt_scene(StoryData& data, SceneData scene) {
    data.scenes.push_back(std::make_unique<SceneData>(std::move(scene)));
    return data.scenes.back().get();
}

std::vector<std::string> collect_scene_ids(const StoryData& data) {
    std::vector<std::string> ids;
    ids.reserve(data.scenes.size());
    for (const auto& scene : data.scenes) ids.push_back(scene->scene_id);
    return ids;
}

void note_scene_advanced(StoryData& data, const std::string& scene_id) {
    ++data.turn_clock;
    if (SceneData* scene = find_scene(data, scene_id))
        scene->last_advanced = data.turn_clock;
}

std::optional<std::vector<std::string>> resolve_non_player_members(
    const World& world, const std::string& scene_id,
    const std::vector<std::string>& names) {
    std::vector<std::string> resolved;
    resolved.reserve(names.size());
    std::unordered_set<std::string> seen;
    for (const auto& requested : names) {
        if (str::trim(requested).empty()) return std::nullopt;
        const Character* character = world.find_in_scene(scene_id, requested);
        if (!character || character->is_player) return std::nullopt;
        const std::string key = str::to_lower(character->name);
        if (!seen.insert(key).second) return std::nullopt;
        resolved.push_back(character->name);
    }
    return resolved;
}

std::optional<std::vector<std::string>> resolve_fork_cast(
    const World& world, const std::string& parent_id,
    const std::vector<std::string>& names,
    const std::string& driving_intention) {
    if (str::trim(driving_intention).empty())
        return std::nullopt;
    auto resolved = resolve_non_player_members(world, parent_id, names);
    if (!resolved || resolved->empty()) return std::nullopt;

    std::unordered_set<std::string> departing;
    for (const auto& name : *resolved)
        departing.insert(str::to_lower(name));
    for (const auto& character : world.characters()) {
        if (character.dead || !character.in_scene(parent_id)) continue;
        if (departing.count(str::to_lower(character.name)) == 0)
            return resolved;
    }
    return std::nullopt;
}

bool scene_retains_living_character(
    const World& world, const std::string& scene_id,
    const std::vector<std::string>& leaving) {
    std::unordered_set<std::string> leaving_keys;
    for (const auto& name : leaving)
        leaving_keys.insert(str::to_lower(name));
    for (const auto& character : world.characters()) {
        if (!character.dead && character.in_scene(scene_id) &&
            leaving_keys.count(str::to_lower(character.name)) == 0) {
            return true;
        }
    }
    return false;
}

SceneSummary summarize_story_scene(
    const StoryData& data, const SceneData& scene) {
    SceneSummary summary;
    summary.scene_id = scene.scene_id;
    summary.title = scene.title;
    summary.active = scene.scene_id == data.active_scene_id;
    summary.turn_index = scene.turn_index;
    summary.staleness = data.turn_clock - scene.last_advanced;

    for (const auto& character : data.world.characters()) {
        if (!character.in_scene(scene.scene_id)) continue;
        summary.cast.push_back(character.name);
        if (character.is_player) summary.player_present = true;
    }

    SceneDrive drive = derive_scene_drive(data, scene);
    summary.driving_intention = std::move(drive.intention);
    summary.charge = drive.charge;

    for (auto it = scene.history.rbegin(); it != scene.history.rend(); ++it) {
        if (it->role != Role::Assistant) continue;
        summary.last_narration = truncate_utf8_ellipsis(it->content, 240);
        summary.recent_narration = truncate_utf8_ellipsis(it->content, 1200);
        break;
    }
    return summary;
}

std::vector<SceneSummary> summarize_story_scenes(const StoryData& data) {
    std::vector<SceneSummary> summaries;
    summaries.reserve(data.scenes.size());
    for (const auto& scene : data.scenes)
        summaries.push_back(summarize_story_scene(data, *scene));
    return summaries;
}

TurnSummary summarize_completed_turn(
    const StoryData& data, const SceneData& scene,
    const std::string& player_input) {
    TurnSummary summary;
    summary.scene_id = scene.scene_id;
    summary.title = scene.title;
    summary.storylines = summarize_story_scenes(data);
    if (!player_input.empty()) summary.player_action = player_input;
    for (const auto& storyline : summary.storylines) {
        if (storyline.scene_id != scene.scene_id) continue;
        summary.cast = storyline.cast;
        summary.player_present = storyline.player_present;
        break;
    }
    for (auto it = scene.history.rbegin(); it != scene.history.rend(); ++it) {
        if (it->role == Role::Assistant) {
            summary.narration = it->content;
            break;
        }
    }
    const int completed_turn = scene.turn_index;
    for (const auto& message : scene.dialogue) {
        if (message.metadata.value("turn", -1) == completed_turn)
            summary.dialogue.push_back(message);
    }
    return summary;
}

ReadToolContext make_story_read_tool_context(
    const StoryData& data, const std::string& scene_id) {
    std::unordered_map<std::string, const SceneData*> scenes_by_id;
    for (const auto& scene : data.scenes)
        scenes_by_id.emplace(scene->scene_id, scene.get());
    return make_read_tool_context(
        data.world, data.observations, scene_id,
        serialize_scene_summaries(summarize_story_scenes(data)),
        std::move(scenes_by_id));
}

ReadToolLease make_frozen_story_read_tools(
    const StoryData& data, const std::string& scene_id) {
    auto snapshot = std::make_shared<FrozenReadToolState>(data);
    return ReadToolLease(snapshot->context_for(scene_id), snapshot);
}

std::vector<std::string> select_off_stage_scenes(
    const StoryData& data, const SchedulerCallback& scheduler) {
    std::vector<std::string> selected;
    std::unordered_set<std::string> seen;

    auto try_add = [&](const std::string& id) {
        if (id.empty() || id == data.active_scene_id) return;
        if (!find_scene(data, id)) {
            log_debug("scheduler") << "skip unknown scene=" << id << "\n";
            return;
        }
        if (!seen.insert(id).second) return;
        if (static_cast<int>(selected.size()) >= kMaxOffStagePerTurn) return;
        selected.push_back(id);
    };

    std::vector<SceneSummary> starved;
    for (const auto& summary : summarize_story_scenes(data)) {
        log_debug("scheduler") << "candidate " << summary.scene_id
            << " charge=" << summary.charge
            << " staleness=" << summary.staleness
            << (summary.player_present ? " PLAYER" : " off-stage")
            << " intent=" << summary.driving_intention << "\n";
        if (!summary.player_present && summary.staleness >= kStarvationTurns)
            starved.push_back(summary);
    }
    std::sort(starved.begin(), starved.end(),
        [](const SceneSummary& left, const SceneSummary& right) {
            if (left.staleness != right.staleness)
                return left.staleness > right.staleness;
            return left.scene_id < right.scene_id;
        });
    for (const auto& summary : starved) try_add(summary.scene_id);

    if (!scheduler) {
        if (selected.empty())
            log_debug("scheduler") << "no pick (no callback)\n";
        return selected;
    }

    try {
        log_info("scheduler") << "calling LLM…\n" << std::flush;
        auto read_tools = make_frozen_story_read_tools(data, "");
        const std::string raw =
            request_off_stage_scene(scheduler, read_tools.callback());
        for (const auto& id : split_scene_picks(raw)) {
            if (id == data.active_scene_id) {
                log_debug("scheduler")
                    << "declined player scene=" << id << "\n";
                continue;
            }
            try_add(id);
        }
    } catch (const std::exception& error) {
        log_warn("scheduler") << "call failed: " << error.what() << "\n"
                              << std::flush;
        return selected;
    }

    if (selected.empty()) {
        log_debug("scheduler") << "advance nothing\n";
    } else {
        std::ostringstream picks;
        for (size_t index = 0; index < selected.size(); ++index) {
            if (index) picks << ',';
            picks << selected[index];
        }
        log_info("scheduler") << "pick " << picks.str() << "\n";
    }
    return selected;
}

std::string make_autonomous_turn_cue(
    const StoryData& data, const std::string& scene_id) {
    for (const auto& summary : summarize_story_scenes(data)) {
        if (summary.scene_id == scene_id) return build_autonomous_cue(summary);
    }
    return build_autonomous_cue({});
}

}  // namespace rhapsode
