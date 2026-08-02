#include "rhapsode/story.h"

#include <algorithm>
#include <atomic>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/str_util.h"
#include "rhapsode/turn_executor.h"

namespace rhapsode {
namespace {

constexpr int kMaxOffStagePerTurn = 2;
constexpr int kStarvationTurns = 3;

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

/// True if at least one living character stays in `scene_id` after `leaving` goes.
bool cast_remains_after_leaving(const World& world, const std::string& scene_id,
                                const std::unordered_set<std::string>& leaving) {
    for (const auto& character : world.characters()) {
        if (!character.dead && character.in_scene(scene_id) &&
            leaving.count(character.name) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

std::vector<std::string> Story::pick_off_stage_scenes() {
    std::vector<std::string> selected;
    std::unordered_set<std::string> seen;

    auto try_add = [&](const std::string& id) {
        if (id.empty() || id == active_scene_id_) return;
        if (!get_scene(id)) {
            log_debug("scheduler") << "skip unknown scene=" << id << "\n";
            return;
        }
        if (!seen.insert(id).second) return;
        if (static_cast<int>(selected.size()) >= kMaxOffStagePerTurn) return;
        selected.push_back(id);
    };

    std::vector<SceneSummary> starved;
    for (const auto& summary : summarize_scenes()) {
        log_debug("scheduler") << "candidate " << summary.scene_id
              << " charge=" << summary.charge
              << " staleness=" << summary.staleness
              << (summary.player_present ? " PLAYER" : " off-stage")
              << " intent=" << summary.driving_intention << "\n";
        if (summary.player_present) continue;
        if (summary.staleness >= kStarvationTurns)
            starved.push_back(summary);
    }
    std::sort(starved.begin(), starved.end(),
        [](const SceneSummary& a, const SceneSummary& b) {
            if (a.staleness != b.staleness) return a.staleness > b.staleness;
            return a.scene_id < b.scene_id;
        });
    for (const auto& summary : starved) try_add(summary.scene_id);

    if (!scheduler_cb_) {
        if (selected.empty())
            log_debug("scheduler") << "no pick (no callback)\n";
        return selected;
    }

    std::string raw;
    try {
        log_info("scheduler") << "calling LLM…\n" << std::flush;
        ReadToolLease read_tools(make_read_tool_context(*this, ""));
        raw = request_off_stage_scene(scheduler_cb_, read_tools.callback);
    } catch (const std::exception& error) {
        log_warn("scheduler") << "call failed: " << error.what() << "\n"
                              << std::flush;
        return selected;
    }

    for (const auto& id : split_scene_picks(raw)) {
        if (id == active_scene_id_) {
            log_debug("scheduler") << "declined player scene=" << id << "\n";
            continue;
        }
        try_add(id);
    }

    if (selected.empty())
        log_debug("scheduler") << "advance nothing\n";
    else {
        std::ostringstream picks;
        for (size_t i = 0; i < selected.size(); ++i) {
            if (i) picks << ',';
            picks << selected[i];
        }
        log_info("scheduler") << "pick " << picks.str() << "\n";
    }
    return selected;
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
        log_info("lifecycle") << "calling LLM scene=" << scene_id << "\n"
                              << std::flush;
        ReadToolLease read_tools(make_read_tool_context(*this, scene_id));
        decision = request_lifecycle_decision(
            beat, lifecycle_cb_, read_tools.callback);
    } catch (const std::exception& error) {
        log_warn("lifecycle") << "verdict call failed: " << error.what() << "\n"
                              << std::flush;
        return result;
    }
    if (!decision) {
        log_debug("lifecycle") << "verdict invalid or unparseable -- no-op\n";
        return result;
    }
    if (decision->ops.empty()) return result;

    for (const auto& op : decision->ops) {
        switch (op.kind) {
        case LifecycleOp::Kind::Merge: {
            if (!get_scene(op.from) || !get_scene(op.into)) {
                log_info("lifecycle") << "skip merge from=" << op.from
                      << " into=" << op.into << " (missing scene)\n";
                break;
            }
            log_info("lifecycle") << "merge from=" << op.from
                  << " into=" << op.into << "\n";
            if (merge_scene(op.from, op.into)) {
                ++result.applied;
                if (op.from == scene_id) result.merged_into = op.into;
            } else {
                log_warn("lifecycle") << "merge failed from=" << op.from
                      << " into=" << op.into << "\n" << std::flush;
            }
            break;
        }
        case LifecycleOp::Kind::Conclude: {
            if (!get_scene(op.scene_id)) {
                log_info("lifecycle") << "skip conclude scene=" << op.scene_id
                      << " (missing)\n";
                break;
            }
            log_info("lifecycle") << "conclude scene=" << op.scene_id << "\n";
            if (conclude_scene(op.scene_id, op.reason))
                ++result.applied;
            else
                log_warn("lifecycle") << "conclude failed scene=" << op.scene_id
                      << "\n" << std::flush;
            break;
        }
        case LifecycleOp::Kind::Fork: {
            if (op.scene_id != scene_id) {
                log_info("lifecycle") << "skip fork parent=" << op.scene_id
                      << " (must be advanced=" << scene_id << ")\n";
                break;
            }
            if (!get_scene(op.scene_id)) {
                log_info("lifecycle") << "skip fork scene=" << op.scene_id
                      << " (missing)\n";
                break;
            }
            const auto fork_cast = resolve_fork_cast(
                op.scene_id, op.cast, op.driving_intention);
            if (!fork_cast) {
                log_info("lifecycle") << "skip fork -- invalid cast/intention\n";
                break;
            }
            std::unordered_set<std::string> leaving(
                fork_cast->begin(), fork_cast->end());
            if (!cast_remains_after_leaving(*world_, op.scene_id, leaving)) {
                log_info("lifecycle") << "skip fork -- would empty the scene\n";
                break;
            }
            const std::string new_id = op.scene_id + "_f" +
                std::to_string(beat_clock_) + "_" +
                std::to_string(result.applied);
            log_info("lifecycle") << "fork from=" << op.scene_id
                  << " intent=" << op.driving_intention << "\n";
            if (fork_scene(op.scene_id, new_id, *fork_cast, op.driving_intention))
                ++result.applied;
            else
                log_warn("lifecycle") << "fork failed from=" << op.scene_id
                      << "\n" << std::flush;
            break;
        }
        case LifecycleOp::Kind::Exit: {
            if (!get_scene(op.scene_id)) {
                log_info("lifecycle") << "skip exit scene=" << op.scene_id
                      << " (missing)\n";
                break;
            }
            const auto exited =
                resolve_non_player_members(op.scene_id, op.cast);
            if (!exited) {
                log_info("lifecycle") << "skip exit -- invalid cast\n";
                break;
            }
            std::unordered_set<std::string> leaving(
                exited->begin(), exited->end());
            if (!cast_remains_after_leaving(*world_, op.scene_id, leaving)) {
                log_info("lifecycle") << "skip exit -- would empty the scene\n";
                break;
            }
            bool any = false;
            for (const auto& name : *exited) {
                if (world_->leave_character(op.scene_id, name)) {
                    log_info("lifecycle") << "exit name=" << name
                          << " scene=" << op.scene_id << "\n";
                    any = true;
                }
            }
            if (any) ++result.applied;
            break;
        }
        }
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
        log_warn("memory") << "post-beat sync failed: " << error.what() << "\n"
                           << std::flush;
    }
}

Story::StepResult Story::step_scene(const std::string& scene_id,
                                    const std::string& input,
                                    bool autonomous) {
    StepResult step;
    SceneData* scene = get_scene(scene_id);
    if (!scene) return step;

    try {
        executor_->set_live_storylines_board(
            format_live_storylines_board(summarize_scenes()));
        ReadToolLease read_tools(make_read_tool_context(*this, scene_id));
        TurnResult result = autonomous
            ? executor_->run_autonomous_turn(*scene, input, read_tools.callback)
            : executor_->run_player_turn(*scene, input, read_tools.callback);
        const int completed_turn = result.completed_turn;
        step.outputs = std::move(result.outputs);
        sync_memory(result);
        step.lifecycle = apply_lifecycle(
            scene_id, autonomous ? std::string{} : input);
        if (step.lifecycle.applied) {
            log_debug("lifecycle") << "applied " << step.lifecycle.applied
                  << " op(s) after scene=" << scene_id << "\n";
        }
        note_advanced(scene_id);
        if (autonomous) {
            log_info("scheduler") << "advanced scene=" << scene_id
                  << " turn=" << completed_turn << "\n";
        }
        step.ok = true;
    } catch (const std::exception& error) {
        log_error("turn") << "step failed scene=" << scene_id
              << ": " << error.what() << "\n" << std::flush;
    }
    return step;
}

std::vector<SceneMessage> Story::advance_scene(const std::string& player_input) {
    SceneData* active = active_scene();
    if (!active) throw std::runtime_error("Story::advance_scene: no active scene");
    const std::string player_scene_id = active->scene_id;

    StepResult player = step_scene(player_scene_id, player_input, false);
    if (!player.ok)
        throw std::runtime_error("Story::advance_scene: player step failed");
    std::vector<SceneMessage> outputs = std::move(player.outputs);

    const auto picks = pick_off_stage_scenes();
    for (const auto& scene_id : picks) {
        if (!get_scene(scene_id)) {
            log_debug("scheduler") << "skip retired scene=" << scene_id << "\n";
            continue;
        }
        StepResult off = step_scene(
            scene_id, make_autonomous_cue(scene_id), true);
        if (!off.ok) continue;
        if (off.lifecycle.merged_into &&
            *off.lifecycle.merged_into == active_scene_id_) {
            outputs.insert(outputs.end(),
                off.outputs.begin(), off.outputs.end());
        }
    }

    if (!saves_dir_.empty()) save(saves_dir_);
    return outputs;
}

}  // namespace rhapsode
