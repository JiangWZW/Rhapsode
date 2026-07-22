#include "rhapsode/story.h"

#include <atomic>
#include <memory>
#include <unordered_map>

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/turn_executor.h"

namespace rhapsode {
namespace {

class ReadToolLease {
private:
    std::shared_ptr<std::atomic_bool> active_;

public:
    explicit ReadToolLease(ReadToolContext context)
        : active_(std::make_shared<std::atomic_bool>(true)),
          callback([context = std::move(context), active = active_](
                       const std::string& name,
                       const std::string& args_json) {
              if (!active->load()) {
                  throw std::runtime_error(
                      "Read tool callback is no longer active");
              }
              return dispatch_read_tool(context, name, args_json);
          }) {}

    ~ReadToolLease() { active_->store(false); }

    ReadToolLease(const ReadToolLease&) = delete;
    ReadToolLease& operator=(const ReadToolLease&) = delete;

    ReadToolCallback callback;
};

ReadToolContext make_read_tool_context(const Story& story,
                                       const std::string& scene_id) {
    const SceneData* scene = story.get_scene(scene_id);
    std::unordered_map<std::string, const std::vector<SceneMessage>*> histories;
    for (const auto& id : story.scene_ids()) {
        const SceneData* item = story.get_scene(id);
        if (item) histories.emplace(id, &item->history);
    }
    return ReadToolContext{
        &story.world(),
        scene ? &scene->history : nullptr,
        scene_id,
        story.tool_list_scenes(),
        std::move(histories),
    };
}

}  // namespace

std::string Story::pick_off_stage_scene() {
    if (!scheduler_cb_) return "";
    for (const auto& summary : summarize_scenes()) {
        log() << "  [scheduler] candidate " << summary.scene_id
              << ": charge=" << summary.charge
              << " staleness=" << summary.staleness
              << (summary.player_present ? " PLAYER" : " off-stage")
              << " intent=" << summary.driving_intention << "\n";
    }

    std::string pick;
    try {
        ReadToolLease read_tools(make_read_tool_context(*this, ""));
        pick = request_off_stage_scene(scheduler_cb_, read_tools.callback);
    } catch (const std::exception& error) {
        log() << "  [scheduler] call failed: " << error.what() << "\n";
        return "";
    }
    if (pick.empty()) {
        log() << "  [scheduler] chose to advance nothing this turn\n";
        return "";
    }
    if (pick == active_scene_id_) {
        log() << "  [scheduler] declined '" << pick
              << "' (it is the player's scene)\n";
        return "";
    }
    if (!get_scene(pick)) {
        log() << "  [scheduler] picked unknown scene '" << pick << "'\n";
        return "";
    }
    log() << "  [scheduler] picked off-stage scene '" << pick << "'\n";
    return pick;
}

BeatSummary Story::build_beat_summary(
    const SceneData& scene, const std::string& player_input) const {
    BeatSummary beat;
    beat.scene_id = scene.scene_id;
    beat.title = scene.title;
    beat.storylines = summarize_scenes();
    if (!player_input.empty()) beat.player_action = player_input;
    for (const auto& summary : beat.storylines) {
        if (summary.scene_id != scene.scene_id) continue;
        beat.cast = summary.cast;
        beat.player_present = summary.player_present;
        break;
    }
    for (auto it = scene.history.rbegin(); it != scene.history.rend(); ++it) {
        if (it->role == Role::Assistant) {
            beat.narration = it->content;
            break;
        }
    }
    const int completed_turn = scene.turn_index - 1;
    for (const auto& message : scene.dialogue) {
        if (message.metadata.value("turn", -1) == completed_turn)
            beat.dialogue.push_back(message);
    }
    return beat;
}

Story::LifecycleApplyResult Story::apply_lifecycle(
    const std::string& scene_id, const std::string& player_input) {
    LifecycleApplyResult result;
    if (!lifecycle_cb_) return result;
    SceneData* scene = get_scene(scene_id);
    if (!scene) return result;

    const BeatSummary beat = build_beat_summary(*scene, player_input);

    std::optional<LifecycleDecision> decision;
    try {
        ReadToolLease read_tools(make_read_tool_context(*this, scene_id));
        decision = request_lifecycle_decision(
            beat, lifecycle_cb_, read_tools.callback);
    } catch (const std::exception& error) {
        log() << "  [lifecycle] verdict call failed: " << error.what() << "\n";
        return result;
    }
    if (!decision) {
        log() << "  [lifecycle] verdict invalid or unparseable -- no-op\n";
        return result;
    }

    if (decision->conclude_reason) {
        log() << "  [lifecycle] verdict: conclude '" << scene_id << "'\n";
        if (conclude_scene(scene_id, *decision->conclude_reason))
            result.applied = 1;
        return result;
    }
    if (decision->merge_into) {
        log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
              << *decision->merge_into << "'\n";
        if (merge_scene(scene_id, *decision->merge_into)) {
            result.applied = 1;
            result.merged_into = *decision->merge_into;
        }
        return result;
    }

    std::optional<std::vector<std::string>> fork_cast;
    if (decision->fork) {
        fork_cast = resolve_fork_cast(
            scene_id, decision->fork->cast,
            decision->fork->driving_intention);
        if (!fork_cast) {
            log() << "  [lifecycle] invalid fork cast or intention -- no-op\n";
            return result;
        }
    }
    const auto exited = resolve_non_player_members(scene_id, decision->exited);
    if (!exited) {
        log() << "  [lifecycle] invalid exit cast -- no-op\n";
        return result;
    }

    std::unordered_map<std::string, bool> leaving;
    if (fork_cast) {
        for (const auto& name : *fork_cast) leaving.emplace(name, true);
    }
    for (const auto& name : *exited) {
        const Character* character = world_->find_character(name);
        if (!character || leaving.count(character->name) > 0) {
            log() << "  [lifecycle] overlapping fork/exit cast -- no-op\n";
            return result;
        }
        leaving.emplace(character->name, true);
    }
    if (!leaving.empty()) {
        bool cast_remains = false;
        for (const auto& character : world_->characters()) {
            if (!character.dead && character.in_scene(scene_id) &&
                leaving.count(character.name) == 0) {
                cast_remains = true;
                break;
            }
        }
        if (!cast_remains) {
            log() << "  [lifecycle] verdict would empty the scene -- no-op\n";
            return result;
        }
    }

    if (decision->fork) {
        const std::string& intention = decision->fork->driving_intention;
        const std::string new_id = scene_id + "_f" + std::to_string(beat_clock_)
                                 + "_" + std::to_string(result.applied);
        if (!fork_scene(scene_id, new_id, *fork_cast, intention)) return {};
        ++result.applied;
        log() << "  [lifecycle] verdict: fork from '" << scene_id
              << "' intent=" << intention << "\n";
    }
    if (!exited->empty()) {
        bool any = false;
        for (const auto& name : *exited) {
            if (world_->leave_character(scene_id, name)) {
                log() << "  [lifecycle] " << name << " exits '" << scene_id
                      << "' (no new storyline)\n";
                any = true;
            }
        }
        if (any) ++result.applied;
    }
    return result;
}

std::string Story::make_autonomous_cue(const std::string& scene_id) const {
    for (const auto& summary : summarize_scenes()) {
        if (summary.scene_id == scene_id) return build_autonomous_cue(summary);
    }
    return build_autonomous_cue({});
}

void Story::sync_memory(const TurnResult& result) {
    MemorySystem* memory = memory_.get();
    SceneData* scene = get_scene(result.scene_id);
    if (!memory || !scene) return;
    try {
        if (!result.effects.created_nodes.empty())
            memory->process_new_nodes(result.effects.created_nodes, scene->turn_index);
        if (!result.effects.expired_nodes.empty())
            memory->sync_expired(result.effects.expired_nodes);
    } catch (const std::exception& error) {
        log() << "  [sync] post-beat memory sync failed: " << error.what() << "\n";
    }
}

std::vector<SceneMessage> Story::advance_off_stage_scene() {
    if (scene_count() <= 1) return {};

    const std::string scene_id = pick_off_stage_scene();
    if (scene_id.empty()) return {};

    SceneData* scene = get_scene(scene_id);
    if (!scene) return {};

    try {
        TurnResult result = [&] {
            ReadToolLease read_tools(
                make_read_tool_context(*this, scene_id));
            return executor_->run_autonomous_turn(
                *scene, make_autonomous_cue(scene_id), read_tools.callback);
        }();
        const int completed_turn = result.completed_turn;
        std::vector<SceneMessage> outputs = std::move(result.outputs);
        sync_memory(result);
        const LifecycleApplyResult lifecycle = apply_lifecycle(scene_id, "");
        if (lifecycle.applied)
            log() << "  [lifecycle] applied " << lifecycle.applied
                  << " op(s) from off-stage beat\n";
        note_advanced(scene_id);
        log() << "[scheduler] advanced off-stage scene '" << scene_id
              << "' (turn " << completed_turn << ")\n";
        if (lifecycle.merged_into &&
            *lifecycle.merged_into == active_scene_id_) {
            return outputs;
        }
    } catch (const std::exception& error) {
        log() << "  [scheduler] off-stage beat failed for '" << scene_id
              << "': " << error.what() << "\n";
    }
    return {};
}

std::vector<SceneMessage> Story::advance_scene(const std::string& player_input) {
    SceneData* active = active_scene();
    if (!active) throw std::runtime_error("Story::advance_scene: no active scene");
    const std::string player_scene_id = active->scene_id;

    TurnResult player_result = [&] {
        ReadToolLease read_tools(
            make_read_tool_context(*this, player_scene_id));
        return executor_->run_player_turn(
            *active, player_input, read_tools.callback);
    }();
    sync_memory(player_result);
    const LifecycleApplyResult player_lifecycle =
        apply_lifecycle(player_scene_id, player_input);
    if (player_lifecycle.applied)
        log() << "  [lifecycle] applied " << player_lifecycle.applied
              << " op(s) from player beat\n";
    note_advanced(player_scene_id);
    std::vector<SceneMessage> outputs = std::move(player_result.outputs);

    auto merged = advance_off_stage_scene();
    outputs.insert(outputs.end(), merged.begin(), merged.end());

    if (!saves_dir_.empty()) save(saves_dir_);
    return outputs;
}

}  // namespace rhapsode
