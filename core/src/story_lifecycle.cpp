#include "rhapsode/story_lifecycle.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "rhapsode/character_memory.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/turn_pipeline.h"

namespace rhapsode {
namespace {

nlohmann::json scene_continuity_context(const SceneData& scene,
                                        const World& world) {
    nlohmann::json context;
    context["scene_id"] = scene.scene_id;
    context["title"] = scene.title;
    context["driving_intention"] = scene.driving_intention;
    context["story_so_far"] = render_text_downsampling(scene.downsampling);

    nlohmann::json cast = nlohmann::json::array();
    for (const auto& character : world.characters()) {
        if (!character.dead && character.in_scene(scene.scene_id))
            cast.push_back(character.name);
    }
    context["cast"] = std::move(cast);

    std::vector<const SceneMessage*> timeline;
    timeline.reserve(scene.history.size() + scene.dialogue.size());
    for (const auto& message : scene.history) timeline.push_back(&message);
    for (const auto& message : scene.dialogue) timeline.push_back(&message);
    std::stable_sort(timeline.begin(), timeline.end(),
        [](const SceneMessage* left, const SceneMessage* right) {
            return left->timestamp < right->timestamp;
        });
    constexpr std::size_t kRecentMessages = 8;
    const std::size_t start = timeline.size() > kRecentMessages
        ? timeline.size() - kRecentMessages : 0;
    nlohmann::json recent = nlohmann::json::array();
    for (std::size_t index = start; index < timeline.size(); ++index) {
        const SceneMessage& message = *timeline[index];
        nlohmann::json row{
            {"role", message.role},
            {"content", truncate_utf8(message.content, 600)},
        };
        if (message.metadata.contains("speaker"))
            row["speaker"] = message.metadata["speaker"];
        recent.push_back(std::move(row));
    }
    context["recent_timeline"] = std::move(recent);
    return context;
}

std::string parse_synthesized_story_so_far(const std::string& response,
                                           const char* field,
                                           const char* operation) {
    const std::string safe = sanitize_utf8(response);
    const auto left = safe.find('{');
    const auto right = safe.rfind('}');
    if (left == std::string::npos || right == std::string::npos || right < left)
        throw std::runtime_error(
            std::string(operation) + " narrator returned no JSON object");

    const nlohmann::json value = nlohmann::json::parse(
        safe.substr(left, right - left + 1), nullptr, false);
    if (!value.is_object())
        throw std::runtime_error(
            std::string(operation) + " narrator returned invalid JSON");
    const auto it = value.find(field);
    if (it == value.end() || !it->is_string() ||
        str::trim(it->get<std::string>()).empty())
        throw std::runtime_error(
            std::string(operation) + " narrator omitted " + field);
    return sanitize_utf8(str::trim(it->get<std::string>()));
}

SceneClosure archive_merged_storyline(
    const StoryData& data, const SceneData& source, const std::string& into_id) {
    SceneClosure closure;
    closure.scene_id = source.scene_id;
    closure.reason = "merged into " + into_id;
    closure.merged_into = into_id;
    closure.driving_intention = source.driving_intention;
    closure.story_so_far = format_visible_transcript(source);
    closure.concluded_at = data.turn_clock;
    for (const auto& character : data.world.characters()) {
        if (character.dead || !character.in_scene(source.scene_id)) continue;
        closure.cast.push_back(character.name);
    }
    for (auto message = source.history.rbegin();
         message != source.history.rend(); ++message) {
        if (message->role != Role::Assistant) continue;
        closure.final_narration = message->content;
        break;
    }
    return closure;
}

}  // namespace

std::string synthesize_merge_context(
    World& world, TurnServices& services,
    const SceneData& source, const SceneData& target,
    ReadToolCallback read_tool) {
    const std::string instructions =
        "You reconcile two converging storyline contexts into one factual, "
        "compact story-so-far for the destination scene. Preserve established "
        "causality, character state, unresolved goals, and the immediate "
        "situation. Incorporate source details only when they remain relevant "
        "after convergence. Do not advance time, invent events, write dialogue, "
        "or narrate a new turn. Use tools to resolve ambiguity. "
        "Return only JSON: {\"merged_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["source"] = scene_continuity_context(source, world);
    payload["destination"] = scene_continuity_context(target, world);
    payload["instruction"] =
        "Fold source continuity into destination continuity. The destination's "
        "recent transcript remains available separately on its next turn.";

    const std::string response = services.narrator(
        target.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "merged_story_so_far", "Merge");
}

std::string synthesize_fork_context(
    World& world, TurnServices& services,
    const SceneData& parent, const std::vector<std::string>& cast,
    const std::string& driving_intention,
    ReadToolCallback read_tool) {
    const std::string instructions =
        "You prepare the starting context for a new parallel storyline that "
        "has just split from its parent scene. Isolate the departing cast's "
        "relevant established situation, relationships, unresolved facts, and "
        "the immediate reason for their stated intention. Do not advance time, "
        "invent events, write dialogue, or narrate a new turn. Use tools to "
        "resolve ambiguity. Return only JSON: "
        "{\"fork_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["parent"] = scene_continuity_context(parent, world);
    payload["fork"] = {
        {"cast", cast},
        {"driving_intention", driving_intention},
    };
    payload["instruction"] =
        "Write only the continuity needed for this departing cast's first "
        "autonomous turn. The parent keeps its own transcript.";

    const std::string response = services.narrator(
        parent.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "fork_story_so_far", "Fork");
}

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
    } catch (...) {
        log() << "  [story] fork synthesis failed for '" << parent_id
              << "' -> '" << new_id << "'\n";
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
    std::ostringstream note;
    note << "fork from=" << parent_id << " to=" << new_id << " cast=[";
    for (size_t i = 0; i < resolved_cast->size(); ++i) {
        if (i) note << ", ";
        note << (*resolved_cast)[i];
    }
    note << "]";
    const std::string intention = str::trim(driving_intention);
    if (!intention.empty()) note << " intention=" << intention;
    append_lifecycle_note(*parent, "fork", note.str());
    log_info("story") << note.str() << "\n";
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
    } catch (...) {
        log() << "  [story] merge synthesis failed for '" << from_id
              << "' -> '" << into_id << "'\n";
        return false;
    }

    constexpr int kVerbatimTail = 6;
    const int summarized_up_to = std::max(
        0, static_cast<int>(target->history.size()) - kVerbatimTail);
    target->downsampling = text_downsampling_from_summary(
        std::move(merged_story_so_far), summarized_up_to);
    SceneClosure closure = archive_merged_storyline(data, *source, into_id);
    append_lifecycle_note(*target, "merge",
                          "merge from=" + from_id + " into=" + into_id);
    data.world.move_scene_members(from_id, into_id);
    data.scene_closures.push_back(std::move(closure));
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
