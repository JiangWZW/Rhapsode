#include "rhapsode/story.h"

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/story_lifecycle.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/turn_pipeline.h"

namespace rhapsode {
namespace {

void sync_memory(
    StoryData& data, TurnServices& services, const std::string& scene_id,
    const std::vector<Node>& created, const std::vector<Node>& expired) {
    SceneData* scene = find_scene(data, scene_id);
    if (!services.memory || !scene) return;
    try {
        if (!created.empty())
            services.memory->process_new_nodes(created, scene->turn_index);
        if (!expired.empty())
            services.memory->sync_expired(expired);
    } catch (...) {
        log_warn("memory") << "post-turn sync failed\n" << std::flush;
    }
}

LifecycleApplyResult complete_scene_maintenance(
    StoryData& data, TurnServices& services, const std::string& scene_id,
    const std::string& player_input, int post_turn_index) {
    const std::vector<Node> expired = process_post_turn(
        data, services, scene_id, post_turn_index);
    sync_memory(data, services, scene_id, {}, expired);

    LifecycleApplyResult applied;
    SceneData* scene = find_scene(data, scene_id);
    if (services.lifecycle && scene) {
        const TurnSummary summary = summarize_completed_turn(
            data, *scene, player_input);
        try {
            ReadToolLease read_tools =
                make_frozen_story_read_tools(data, scene_id);
            const auto decision = request_lifecycle_decision(
                summary, services.lifecycle, read_tools.callback());
            if (decision && !decision->ops.empty()) {
                applied = apply_lifecycle_decision(
                    data, services, scene_id, *decision);
            }
        } catch (...) {
            log_warn("lifecycle") << "verdict call failed\n" << std::flush;
        }
    }

    note_scene_advanced(data, scene_id);
    return applied;
}

}  // namespace

std::vector<SceneMessage> Story::advance_player(
    const std::string& player_input) {
    if (pending_turn_)
        throw std::runtime_error(
            "Story::advance_player: complete_turn pending from prior turn");
    SceneData* active = active_scene();
    if (!active)
        throw std::runtime_error("Story::advance_player: no active scene");

    TurnResult result = execute_turn(
        data_, services_,
        {TurnInput::Kind::Player, active->scene_id, player_input});
    sync_memory(data_, services_, result.scene_id,
                result.effects.created_nodes, result.effects.expired_nodes);
    pending_turn_ = PendingTurn{
        result.scene_id, player_input, result.post_turn_index};
    return std::move(result.outputs);
}

std::vector<SceneMessage> Story::complete_turn() {
    if (!pending_turn_)
        throw std::runtime_error(
            "Story::complete_turn: no pending advance_player");
    PendingTurn pending = std::move(*pending_turn_);
    pending_turn_.reset();

    complete_scene_maintenance(
        data_, services_, pending.scene_id, pending.player_input,
        pending.post_turn_index);

    std::vector<SceneMessage> outputs;
    const auto picks = select_off_stage_scenes(data_, services_.scheduler);
    for (const auto& scene_id : picks) {
        if (!find_scene(data_, scene_id)) continue;
        try {
            TurnResult off_stage = execute_turn(
                data_, services_,
                {TurnInput::Kind::Autonomous, scene_id,
                 make_autonomous_turn_cue(data_, scene_id)});
            sync_memory(data_, services_, scene_id,
                        off_stage.effects.created_nodes,
                        off_stage.effects.expired_nodes);
            const LifecycleApplyResult lifecycle = complete_scene_maintenance(
                data_, services_, scene_id, {}, off_stage.post_turn_index);
            if (lifecycle.merged_into &&
                *lifecycle.merged_into == data_.active_scene_id) {
                outputs.insert(outputs.end(), off_stage.outputs.begin(),
                               off_stage.outputs.end());
            }
            log_info("scheduler") << "advanced scene=" << scene_id
                                  << " turn=" << off_stage.post_turn_index
                                  << "\n";
        } catch (...) {
            log_error("turn") << "step failed scene=" << scene_id << "\n"
                              << std::flush;
        }
    }

    if (!services_.saves_dir.empty()) save(services_.saves_dir);
    return outputs;
}

}  // namespace rhapsode
