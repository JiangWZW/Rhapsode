#include "rhapsode/story.h"

#include <nlohmann/json.hpp>

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/turn_executor.h"
#include "rhapsode/str_util.h"

namespace rhapsode {
namespace {

const std::string kSchedulerInstructions =
    "You are the scene scheduler for a parallel-storyline engine. The player's "
    "scene has just advanced. Exactly one OFF-STAGE storyline may advance this "
    "turn while the others rest.\n"
    "Call list_scenes to see every live storyline. Weigh each off-stage row by "
    "its charge (how urgent its driving intention is), staleness (how many beats "
    "since it last advanced -- higher means overdue), and whether it is "
    "converging on the player. Drill in with query_graph or query_mind if a row "
    "is ambiguous.\n"
    "Then call advance_scene exactly once with the scene_id of the single most "
    "deserving off-stage storyline. Never pick the scene where player_present is "
    "true. If nothing off-stage deserves a beat, call advance_scene with an empty "
    "scene_id.";

const std::string kLifecycleInstructions =
    "You are the lifecycle director of a story that runs PARALLEL storylines. "
    "After each beat you decide whether that beat changed the set of live "
    "storylines for the scene just advanced.\n"
    "- FORK: one or more NON-PLAYER characters leave the scene carrying an ongoing "
    "goal or unresolved stake -- a party splits, a subgroup is dispatched, someone "
    "storms off with a purpose, or the player walks away leaving companions behind "
    "(fork the ones LEFT BEHIND). The player NEVER leaves their own storyline; "
    "never put \"Player\" in a fork.\n"
    "- MERGE: this scene's cast has physically reunited with another live "
    "storyline (co-presence, not mere pursuit) -- fold this scene into that one.\n"
    "- CONCLUDE: this scene's driving intention is fulfilled or dead -- end it.\n"
    "- EXIT: a character simply leaves with no ongoing thread worth following (a "
    "passing NPC the group walked away from). They leave the cast; no new "
    "storyline.\n"
    "If the cast includes \"Player\", this is the player's OWN storyline: it can "
    "never be concluded or merged away -- the player is always in it, so it always "
    "continues. Only a fork (companions leaving) or an exit may apply to it; never "
    "conclude or merge it even if a shared goal just ended.\n"
    "Most beats change NOTHING. Act only on a concrete change in THIS beat -- never "
    "on mood, a passing aside, or characters merely staying together.\n"
    "Reply with ONLY JSON (use null / [] for anything that does not apply):\n"
    "{\"fork\": {\"cast\": [names], \"driving_intention\": \"one sentence\"} | null, "
    "\"merge_into\": \"scene_id\" | null, \"conclude\": \"reason\" | null, "
    "\"exited\": [names]}";

}  // namespace

std::string Story::pick_off_stage_scene() {
    if (!scheduler_cb_) return "";
    const auto rows = nlohmann::json::parse(tool_list_scenes(), nullptr, false);
    if (rows.is_array()) {
        for (const auto& row : rows) {
            if (!row.is_object()) continue;
            log() << "  [scheduler] candidate " << row.value("scene_id", std::string{})
                  << ": charge=" << row.value("charge", 0.0f)
                  << " staleness=" << row.value("staleness", 0)
                  << (row.value("player_present", false) ? " PLAYER" : " off-stage")
                  << " intent=" << row.value("driving_intention", std::string{}) << "\n";
        }
    }

    std::string pick;
    try {
        pick = str::trim(scheduler_cb_(
            kSchedulerInstructions, "Pick the next off-stage scene to advance."));
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

int Story::decide_lifecycle(const std::string& scene_id,
                            const std::string& player_input) {
    if (!lifecycle_cb_) return 0;
    SceneData* scene = get_scene(scene_id);
    if (!scene) return 0;

    nlohmann::json context;
    context["scene_id"] = scene_id;
    context["title"] = scene->title;
    nlohmann::json cast = nlohmann::json::array();
    bool player_here = false;
    for (const auto& character : world_->characters()) {
        if (!character.in_scene(scene_id)) continue;
        cast.push_back(character.name);
        if (character.is_player) player_here = true;
    }
    context["cast"] = std::move(cast);
    context["player_action"] = player_input.empty()
        ? nlohmann::json(nullptr) : nlohmann::json(player_input);

    std::string last;
    for (auto it = scene->history.messages().rbegin();
         it != scene->history.messages().rend(); ++it) {
        if (it->role == Role::Assistant) {
            last = it->content;
            break;
        }
    }
    context["narration"] = last;
    context["other_storylines"] =
        nlohmann::json::parse(tool_list_scenes(), nullptr, false);

    std::string raw;
    try {
        raw = lifecycle_cb_(
            kLifecycleInstructions,
            "Decide the lifecycle verdict for the beat just authored:\n" +
                context.dump(2));
    } catch (const std::exception& error) {
        log() << "  [lifecycle] verdict call failed: " << error.what() << "\n";
        return 0;
    }

    const auto left = raw.find('{');
    const auto right = raw.rfind('}');
    if (left == std::string::npos || right == std::string::npos || right < left) {
        log() << "  [lifecycle] verdict unparseable -- no-op\n";
        return 0;
    }
    const auto verdict = nlohmann::json::parse(
        raw.substr(left, right - left + 1), nullptr, false);
    if (!verdict.is_object()) return 0;

    auto without_player = [&](const nlohmann::json& values) {
        std::vector<std::string> names;
        if (!values.is_array()) return names;
        for (const auto& value : values) {
            if (!value.is_string()) continue;
            const std::string name = value.get<std::string>();
            const Character* character = world_->find_character(name);
            if (!character || !character->is_player) names.push_back(name);
        }
        return names;
    };

    if (const auto it = verdict.find("conclude");
        it != verdict.end() && it->is_string() && !it->get<std::string>().empty()) {
        if (player_here) {
            log() << "  [lifecycle] verdict: conclude '" << scene_id
                  << "' refused -- the player is in this storyline\n";
        } else {
            log() << "  [lifecycle] verdict: conclude '" << scene_id << "'\n";
            return conclude_scene(scene_id, it->get<std::string>()) ? 1 : 0;
        }
    }
    if (const auto it = verdict.find("merge_into");
        it != verdict.end() && it->is_string() && !it->get<std::string>().empty()) {
        if (player_here) {
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << it->get<std::string>()
                  << "' refused -- the player is in this storyline\n";
        } else {
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << it->get<std::string>() << "'\n";
            return merge_scene(scene_id, it->get<std::string>()) ? 1 : 0;
        }
    }

    int applied = 0;
    if (const auto it = verdict.find("fork"); it != verdict.end() && it->is_object()) {
        const auto fork_cast = without_player(it->value("cast", nlohmann::json::array()));
        const std::string intention = it->value("driving_intention", "");
        if (!fork_cast.empty()) {
            const std::string new_id = scene_id + "_f" + std::to_string(beat_clock_)
                                     + "_" + std::to_string(applied);
            if (fork_scene(scene_id, new_id, fork_cast, intention)) ++applied;
            log() << "  [lifecycle] verdict: fork from '" << scene_id
                  << "' intent=" << intention << "\n";
        }
    }
    if (const auto it = verdict.find("exited"); it != verdict.end()) {
        bool any = false;
        for (const auto& name : without_player(*it)) {
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

std::string Story::autonomous_cue(const std::string& scene_id) const {
    std::string intention;
    std::string last;
    if (const SceneData* scene = get_scene(scene_id)) {
        float charge = 0.0f;
        intention = derive_intention(*scene, &charge);
        for (auto it = scene->history.messages().rbegin();
             it != scene->history.messages().rend(); ++it) {
            if (it->role != Role::Assistant) continue;
            last = it->content.size() > 240 ? it->content.substr(0, 240) + "..."
                                            : it->content;
            break;
        }
    }
    std::string cue =
        "[Off-stage beat: the player is elsewhere. Advance THIS storyline by one "
        "meaningful step toward its goal, and record what changes as facts.]";
    if (!intention.empty()) cue += "\nDriving intention: " + intention;
    if (!last.empty()) cue += "\nLast we saw: " + last;
    cue += "\nIf this storyline's cast has physically reached another storyline's "
           "location (co-presence, not mere pursuit), call merge_scene into it.";
    return cue;
}

void Story::sync_beat(const TurnResult& result) {
    MemorySystem* memory = world_->memory();
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

std::vector<SceneMessage> Story::advance_scene(const std::string& player_input) {
    SceneData* active = active_scene();
    if (!active) throw std::runtime_error("Story::advance_scene: no active scene");
    const std::string player_scene_id = active->scene_id;

    TurnResult player_result = executor_->run_player_turn(*active, player_input);
    sync_beat(player_result);
    const int player_lifecycle = decide_lifecycle(player_scene_id, player_input);
    if (player_lifecycle)
        log() << "  [lifecycle] applied " << player_lifecycle
              << " op(s) from player beat\n";
    note_advanced(player_scene_id);
    std::vector<SceneMessage> outputs = std::move(player_result.outputs);

    if (scene_count() > 1) {
        const std::string pick = pick_off_stage_scene();
        if (!pick.empty()) {
            if (SceneData* scene = get_scene(pick)) {
                try {
                    TurnResult result =
                        executor_->run_autonomous_turn(*scene, autonomous_cue(pick));
                    const int completed_turn = result.completed_turn;
                    sync_beat(result);
                    const int lifecycle = decide_lifecycle(pick, "");
                    if (lifecycle)
                        log() << "  [lifecycle] applied " << lifecycle
                              << " op(s) from off-stage beat\n";
                    note_advanced(pick);
                    log() << "[scheduler] advanced off-stage scene '" << pick
                          << "' (turn " << completed_turn << ")\n";
                } catch (const std::exception& error) {
                    log() << "  [scheduler] off-stage beat failed for '" << pick
                          << "': " << error.what() << "\n";
                }
            }
        }
    }

    if (!saves_dir_.empty()) save(saves_dir_);
    return outputs;
}

}  // namespace rhapsode
