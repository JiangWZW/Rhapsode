#include "rhapsode/story_lifecycle.h"

#include <algorithm>
#include <sstream>

#include "rhapsode/character_memory.h"
#include "rhapsode/log_util.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/turn_pipeline.h"

namespace rhapsode {

SceneData* fork_story_scene(
    StoryData& data, TurnServices& services, const std::string& parent_id,
    const std::string& new_id, const std::vector<std::string>& cast,
    const std::string& driving_intention) {
    SceneData* parent = find_scene(data, parent_id);
    if (!parent) {
        log() << "  [story] fork_scene: unknown parent '" << parent_id << "'\n";
        return nullptr;
    }
    if (new_id.empty() || find_scene(data, new_id)) {
        log() << "  [story] fork_scene: scene '" << new_id
              << "' already exists\n";
        return nullptr;
    }

    const auto resolved_cast = resolve_fork_cast(
        data.world, parent_id, cast, driving_intention);
    if (!resolved_cast) {
        log() << "  [story] fork_scene: invalid cast or intention for '"
              << parent_id << "'\n";
        return nullptr;
    }

    ReadToolLease read_tools = make_frozen_story_read_tools(data, parent_id);
    std::string fork_story_so_far;
    try {
        fork_story_so_far = synthesize_fork_context(
            data.world, services, *parent, *resolved_cast,
            str::trim(driving_intention), read_tools.callback());
        read_tools.close();
    } catch (const std::exception& error) {
        log() << "  [story] fork synthesis failed for '" << parent_id
              << "' -> '" << new_id << "': " << error.what() << "\n";
        return nullptr;
    }

    SceneData child;
    child.scene_id = new_id;
    child.title = parent->title;
    child.system_prompt = parent->system_prompt;
    child.driving_intention = str::trim(driving_intention);
    child.downsampling = text_downsampling_from_summary(
        std::move(fork_story_so_far), 0);
    child.last_advanced = data.turn_clock;

    World next_world = data.world;
    child.intention_owner = resolved_cast->front();
    child.intention_node_id = next_world.seed_character_intention(
        child.intention_owner, child.driving_intention,
        *resolved_cast, data.turn_clock);
    if (child.intention_node_id == 0) {
        log() << "  [story] fork_scene: could not seed intention for '"
              << child.intention_owner << "'\n";
        return nullptr;
    }
    child.charge = CharacterMemory::kAuthoredSeedWeight;
    next_world.move_scene_members(parent_id, new_id, *resolved_cast);

    SceneData* adopted = adopt_scene(data, std::move(child));
    data.world = std::move(next_world);
    ++data.transaction_version;
    std::ostringstream cast_line;
    for (size_t i = 0; i < resolved_cast->size(); ++i) {
        if (i) cast_line << ", ";
        cast_line << (*resolved_cast)[i];
    }
    log_info("story") << "fork from=" << parent_id << " to=" << new_id
                      << " cast=[" << cast_line.str() << "]\n";
    return adopted;
}

bool conclude_story_scene(
    StoryData& data, const std::string& id, const std::string& reason) {
    auto it = std::find_if(data.scenes.begin(), data.scenes.end(),
        [&](const auto& scene) { return scene->scene_id == id; });
    if (it == data.scenes.end()) {
        log() << "  [story] conclude_scene: unknown scene '" << id << "'\n";
        return false;
    }

    const std::string clean_reason = str::trim(reason);
    if (clean_reason.empty() || data.scenes.size() <= 1) {
        log() << "  [story] conclude_scene: refused '" << id
              << "' (empty reason or final live scene)\n";
        return false;
    }

    SceneClosure closure;
    closure.scene_id = id;
    closure.reason = clean_reason;
    closure.driving_intention = (*it)->driving_intention;
    closure.story_so_far = render_text_downsampling((*it)->downsampling);
    closure.concluded_at = data.turn_clock;
    for (const auto& character : data.world.characters()) {
        if (character.dead || !character.in_scene(id)) continue;
        if (character.is_player) {
            log() << "  [story] conclude_scene: refused '" << id
                  << "' because it contains the Player\n";
            return false;
        }
        closure.cast.push_back(character.name);
    }
    for (auto message = (*it)->history.rbegin();
         message != (*it)->history.rend(); ++message) {
        if (message->role != Role::Assistant) continue;
        closure.final_narration = message->content;
        break;
    }

    World next_world = data.world;
    if ((*it)->intention_node_id != 0 &&
        !next_world.expire_character_intention(
            (*it)->intention_owner, (*it)->intention_node_id,
            data.turn_clock)) {
        log() << "  [story] conclude_scene: owned intention missing for '"
              << id << "'\n";
        return false;
    }
    next_world.clear_scene_membership(id);

    data.scene_closures.push_back(std::move(closure));
    data.world = std::move(next_world);
    data.scenes.erase(it);
    if (data.active_scene_id == id)
        data.active_scene_id = data.scenes.front()->scene_id;
    ++data.transaction_version;
    log_info("story") << "conclude scene=" << id << ": "
                      << clean_reason << "\n";
    return true;
}

bool merge_story_scene(
    StoryData& data, TurnServices& services, const std::string& from_id,
    const std::string& into_id) {
    SceneData* source = find_scene(data, from_id);
    SceneData* target = find_scene(data, into_id);
    if (!source || !target || from_id == into_id) {
        log() << "  [story] merge_scene: bad ids '" << from_id << "' -> '"
              << into_id << "'\n";
        return false;
    }
    for (const auto& character : data.world.characters()) {
        if (character.is_player && character.in_scene(from_id)) {
            log() << "  [story] merge_scene: source '" << from_id
                  << "' contains the Player\n";
            return false;
        }
    }

    ReadToolLease read_tools = make_frozen_story_read_tools(data, into_id);
    std::string merged_story_so_far;
    try {
        merged_story_so_far = synthesize_merge_context(
            data.world, services, *source, *target, read_tools.callback());
        read_tools.close();
    } catch (const std::exception& error) {
        log() << "  [story] merge synthesis failed for '" << from_id
              << "' -> '" << into_id << "': " << error.what() << "\n";
        return false;
    }

    constexpr int kVerbatimTail = 6;
    const int summarized_up_to = std::max(
        0, static_cast<int>(target->history.size()) - kVerbatimTail);
    target->downsampling = text_downsampling_from_summary(
        std::move(merged_story_so_far), summarized_up_to);
    data.world.move_scene_members(from_id, into_id);
    auto it = std::find_if(data.scenes.begin(), data.scenes.end(),
        [&](const auto& scene) { return scene->scene_id == from_id; });
    data.scenes.erase(it);
    if (data.active_scene_id == from_id) data.active_scene_id = into_id;
    ++data.transaction_version;
    log_info("story") << "merge from=" << from_id << " into="
                      << into_id << "\n";
    return true;
}

LifecycleApplyResult apply_lifecycle_decision(
    StoryData& data, TurnServices& services,
    const std::string& advanced_scene_id,
    const LifecycleDecision& decision) {
    LifecycleApplyResult result;
    for (const auto& op : decision.ops) {
        switch (op.kind) {
        case LifecycleOp::Kind::Merge:
            if (merge_story_scene(data, services, op.from, op.into)) {
                ++result.applied;
                if (op.from == advanced_scene_id)
                    result.merged_into = op.into;
            }
            break;
        case LifecycleOp::Kind::Conclude:
            if (conclude_story_scene(data, op.scene_id, op.reason))
                ++result.applied;
            break;
        case LifecycleOp::Kind::Fork: {
            if (op.scene_id != advanced_scene_id) {
                log_info("lifecycle") << "skip fork parent=" << op.scene_id
                    << " (must be advanced=" << advanced_scene_id << ")\n";
                break;
            }
            const std::string new_id = op.scene_id + "_f" +
                std::to_string(data.turn_clock) + "_" +
                std::to_string(result.applied);
            if (fork_story_scene(data, services, op.scene_id, new_id,
                                 op.cast, op.driving_intention))
                ++result.applied;
            break;
        }
        case LifecycleOp::Kind::Exit: {
            const auto exited = resolve_non_player_members(
                data.world, op.scene_id, op.cast);
            if (!exited || !scene_retains_living_character(
                    data.world, op.scene_id, *exited)) {
                log_info("lifecycle")
                    << "skip exit -- invalid cast or empty scene\n";
                break;
            }
            bool changed = false;
            for (const auto& name : *exited)
                changed = data.world.leave_character(op.scene_id, name) || changed;
            if (changed) {
                ++data.transaction_version;
                ++result.applied;
            }
            break;
        }
        }
    }
    return result;
}

}  // namespace rhapsode
