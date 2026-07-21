#include "rhapsode/story.h"

#include <atomic>
#include <memory>

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
    return ReadToolContext{
        &story.world(),
        scene ? &scene->history : nullptr,
        scene_id,
        story.tool_list_scenes(),
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
    return beat;
}

std::vector<std::string> Story::without_player_characters(
    const std::vector<std::string>& names) const {
    std::vector<std::string> filtered;
    filtered.reserve(names.size());
    for (const auto& name : names) {
        const Character* character = world_->find_character(name);
        if (!character || !character->is_player) filtered.push_back(name);
    }
    return filtered;
}

int Story::apply_lifecycle(const std::string& scene_id,
                           const std::string& player_input) {
    if (!lifecycle_cb_) return 0;
    SceneData* scene = get_scene(scene_id);
    if (!scene) return 0;

    const BeatSummary beat = build_beat_summary(*scene, player_input);

    std::optional<LifecycleDecision> decision;
    try {
        decision = request_lifecycle_decision(beat, lifecycle_cb_);
    } catch (const std::exception& error) {
        log() << "  [lifecycle] verdict call failed: " << error.what() << "\n";
        return 0;
    }
    if (!decision) {
        log() << "  [lifecycle] verdict unparseable -- no-op\n";
        return 0;
    }

    if (decision->conclude_reason) {
        if (beat.player_present) {
            log() << "  [lifecycle] verdict: conclude '" << scene_id
                  << "' refused -- the player is in this storyline\n";
        } else {
            log() << "  [lifecycle] verdict: conclude '" << scene_id << "'\n";
            return conclude_scene(scene_id, *decision->conclude_reason) ? 1 : 0;
        }
    }
    if (decision->merge_into) {
        if (beat.player_present) {
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << *decision->merge_into
                  << "' refused -- the player is in this storyline\n";
        } else {
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << *decision->merge_into << "'\n";
            return merge_scene(scene_id, *decision->merge_into) ? 1 : 0;
        }
    }

    int applied = 0;
    if (decision->fork) {
        const auto fork_cast = without_player_characters(decision->fork->cast);
        const std::string& intention = decision->fork->driving_intention;
        if (!fork_cast.empty()) {
            const std::string new_id = scene_id + "_f" + std::to_string(beat_clock_)
                                     + "_" + std::to_string(applied);
            if (fork_scene(scene_id, new_id, fork_cast, intention)) ++applied;
            log() << "  [lifecycle] verdict: fork from '" << scene_id
                  << "' intent=" << intention << "\n";
        }
    }
    if (!decision->exited.empty()) {
        bool any = false;
        for (const auto& name : without_player_characters(decision->exited)) {
            if (world_->leave_character(scene_id, name)) {
                log() << "  [lifecycle] " << name << " exits '" << scene_id
                      << "' (no new storyline)\n";
                any = true;
            }
        }
        if (any) ++applied;
    }
    return applied;
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

void Story::advance_off_stage_scene() {
    if (scene_count() <= 1) return;

    const std::string scene_id = pick_off_stage_scene();
    if (scene_id.empty()) return;

    SceneData* scene = get_scene(scene_id);
    if (!scene) return;

    try {
        TurnResult result = [&] {
            ReadToolLease read_tools(
                make_read_tool_context(*this, scene_id));
            return executor_->run_autonomous_turn(
                *scene, make_autonomous_cue(scene_id), read_tools.callback);
        }();
        const int completed_turn = result.completed_turn;
        sync_memory(result);
        const int lifecycle = apply_lifecycle(scene_id, "");
        if (lifecycle)
            log() << "  [lifecycle] applied " << lifecycle
                  << " op(s) from off-stage beat\n";
        note_advanced(scene_id);
        log() << "[scheduler] advanced off-stage scene '" << scene_id
              << "' (turn " << completed_turn << ")\n";
    } catch (const std::exception& error) {
        log() << "  [scheduler] off-stage beat failed for '" << scene_id
              << "': " << error.what() << "\n";
    }
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
    const int player_lifecycle = apply_lifecycle(player_scene_id, player_input);
    if (player_lifecycle)
        log() << "  [lifecycle] applied " << player_lifecycle
              << " op(s) from player beat\n";
    note_advanced(player_scene_id);
    std::vector<SceneMessage> outputs = std::move(player_result.outputs);

    advance_off_stage_scene();

    if (!saves_dir_.empty()) save(saves_dir_);
    return outputs;
}

}  // namespace rhapsode
