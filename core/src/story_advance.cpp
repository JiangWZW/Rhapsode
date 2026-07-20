#include "rhapsode/story.h"

#include <nlohmann/json.hpp>

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/scene_loop.h"
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

    auto rows = nlohmann::json::parse(tool_list_scenes(), nullptr, false);
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
        pick = str::trim(scheduler_cb_(kSchedulerInstructions,
                                       "Pick the next off-stage scene to advance."));
    } catch (const std::exception& e) {
        log() << "  [scheduler] call failed: " << e.what() << "\n";
        return "";
    }

    if (pick.empty()) {
        log() << "  [scheduler] chose to advance nothing this turn\n";
        return "";
    }
    if (pick == active_scene_id_) {
        log() << "  [scheduler] declined '" << pick << "' (it is the player's scene)\n";
        return "";
    }
    if (!get_scene(pick)) {
        log() << "  [scheduler] picked unknown scene '" << pick << "'\n";
        return "";
    }
    log() << "  [scheduler] picked off-stage scene '" << pick << "'\n";
    return pick;
}

void Story::decide_lifecycle(const std::string& scene_id,
                             const std::string& player_input) {
    if (!lifecycle_cb_) return;
    Scene* s = get_scene(scene_id);
    if (!s) return;

    nlohmann::json ctx;
    ctx["scene_id"] = scene_id;
    ctx["title"]    = s->title;

    nlohmann::json cast = nlohmann::json::array();
    for (const auto& ch : world_->characters)
        if (ch.in_scene(scene_id)) cast.push_back(ch.name);
    ctx["cast"] = std::move(cast);

    if (player_input.empty()) ctx["player_action"] = nullptr;
    else                      ctx["player_action"] = player_input;

    std::string last;
    const auto& msgs = s->history.messages();
    for (auto it = msgs.rbegin(); it != msgs.rend(); ++it)
        if (it->role == Role::Assistant) { last = it->content; break; }
    ctx["narration"] = last;

    ctx["other_storylines"] =
        nlohmann::json::parse(tool_list_scenes(), nullptr, /*allow_exceptions=*/false);

    const std::string user =
        "Decide the lifecycle verdict for the beat just authored:\n" + ctx.dump(2);

    std::string raw;
    try {
        raw = lifecycle_cb_(kLifecycleInstructions, user);
    } catch (const std::exception& e) {
        log() << "  [lifecycle] verdict call failed: " << e.what() << "\n";
        return;
    }

    const auto lb = raw.find('{');
    const auto rb = raw.rfind('}');
    if (lb == std::string::npos || rb == std::string::npos || rb < lb) {
        log() << "  [lifecycle] verdict unparseable -- no-op\n";
        return;
    }
    nlohmann::json v =
        nlohmann::json::parse(raw.substr(lb, rb - lb + 1), nullptr, /*allow_exceptions=*/false);
    if (!v.is_object()) return;

    auto without_player = [&](const nlohmann::json& arr) {
        std::vector<std::string> out;
        if (!arr.is_array()) return out;
        for (const auto& c : arr) {
            if (!c.is_string()) continue;
            const Character* ch = world_->find_character(c.get<std::string>());
            if (ch && ch->is_player) continue;
            out.push_back(c.get<std::string>());
        }
        return out;
    };

    bool player_here = false;
    for (const auto& ch : world_->characters)
        if (ch.is_player && ch.in_scene(scene_id)) { player_here = true; break; }

    if (auto it = v.find("conclude");
        it != v.end() && it->is_string() && !it->get<std::string>().empty()) {
        if (player_here) {
            log() << "  [lifecycle] verdict: conclude '" << scene_id
                  << "' refused -- the player is in this storyline\n";
        } else {
            world_->stage_conclude(scene_id, it->get<std::string>());
            log() << "  [lifecycle] verdict: conclude '" << scene_id << "'\n";
            return;
        }
    }
    if (auto it = v.find("merge_into");
        it != v.end() && it->is_string() && !it->get<std::string>().empty()) {
        if (player_here) {
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << it->get<std::string>() << "' refused -- the player is in this storyline\n";
        } else {
            world_->stage_merge(scene_id, it->get<std::string>());
            log() << "  [lifecycle] verdict: merge '" << scene_id << "' -> '"
                  << it->get<std::string>() << "'\n";
            return;
        }
    }

    if (auto it = v.find("fork"); it != v.end() && it->is_object()) {
        std::vector<std::string> fork_cast =
            without_player(it->value("cast", nlohmann::json::array()));
        const std::string intent = it->value("driving_intention", "");
        if (!fork_cast.empty()) {
            world_->stage_fork(scene_id, intent, fork_cast);
            log() << "  [lifecycle] verdict: fork from '" << scene_id
                  << "' intent=" << intent << "\n";
        }
    }
    if (auto it = v.find("exited"); it != v.end()) {
        std::vector<std::string> exited = without_player(*it);
        if (!exited.empty()) {
            world_->stage_exit(scene_id, exited);
            log() << "  [lifecycle] verdict: " << exited.size()
                  << " exit(s) from '" << scene_id << "'\n";
        }
    }
}

std::string Story::autonomous_cue(const std::string& scene_id) const {
    std::string intent, last;
    if (const Scene* s = get_scene(scene_id)) {
        float charge = 0.0f;
        intent = derive_intention(*s, &charge);
        const auto& msgs = s->history.messages();
        for (auto it = msgs.rbegin(); it != msgs.rend(); ++it) {
            if (it->role == Role::Assistant) {
                last = it->content.size() > 240 ? it->content.substr(0, 240) + "..."
                                                : it->content;
                break;
            }
        }
    }

    std::string cue =
        "[Off-stage beat: the player is elsewhere. Advance THIS storyline by one "
        "meaningful step toward its goal, and record what changes as facts.]";
    if (!intent.empty()) cue += "\nDriving intention: " + intent;
    if (!last.empty())   cue += "\nLast we saw: " + last;
    cue += "\nIf this storyline's cast has physically reached another storyline's "
           "location (co-presence, not mere pursuit), call merge_scene into it.";
    return cue;
}

void Story::sync_beat(const SceneTurnResult& result) {
    MemorySystem* mem = world_->memory();
    Scene* s = get_scene(result.scene_id);
    if (!mem || !s) return;

    try {
        if (!result.expiry.empty()) {
            std::vector<Node> nodes;
            nodes.reserve(result.expiry.size());
            for (const auto& op : result.expiry)
                if (const Node* n = world_->world_graph.get_node(op.id))
                    nodes.push_back(*n);
            if (!nodes.empty()) {
                mem->sync_expired(nodes);
                log() << "  [expiry] synced " << nodes.size() << " superseded fact(s)\n";
            }
        }
    } catch (const std::exception& e) {
        log() << "  [expiry] sync failed: " << e.what() << "\n";
    }

    try {
        const DirectorOutput& out = result.director;
        if (!out.new_nodes.empty())     mem->process_new_nodes(out.new_nodes, s->turn_index);
        if (!out.newly_expired.empty()) mem->sync_expired(out.newly_expired);
    } catch (const std::exception& e) {
        log() << "  [sync] post-beat memory sync failed: " << e.what() << "\n";
    }
}

std::vector<SceneMessage> Story::advance_scene(const std::string& player_input) {
    if (!loop_)  throw std::runtime_error("Story::advance_scene: runtime not bound");
    Scene* active = active_scene();
    if (!active) throw std::runtime_error("Story::advance_scene: no active scene");

    auto apply_and_log = [&](const char* which) {
        int applied = apply_pending_ops();
        if (applied) {
            log() << "  [lifecycle] applied " << applied << " op(s) from " << which
                  << "; scenes now:";
            for (const auto& id : scene_ids()) log() << ' ' << id;
            log() << "\n";
        }
    };

    if (downsampler_cb_) active->downsampler.set_llm_callback(downsampler_cb_);
    SceneTurnResult player_result = loop_->run_player_turn(*active, player_input);
    sync_beat(player_result);
    decide_lifecycle(active_scene_id_, player_input);
    apply_and_log("player beat");
    note_advanced(active_scene_id_);

    std::vector<SceneMessage> outputs = std::move(player_result.outputs);

    if (scene_count() > 1) {
        const std::string pick = pick_off_stage_scene();
        if (!pick.empty()) {
            if (Scene* s = get_scene(pick)) {
                try {
                    if (downsampler_cb_) s->downsampler.set_llm_callback(downsampler_cb_);
                    SceneTurnResult off_stage_result =
                        loop_->run_autonomous_turn(*s, autonomous_cue(pick));
                    const int completed_turn = off_stage_result.completed_turn;
                    sync_beat(off_stage_result);
                    decide_lifecycle(pick, "");
                    apply_and_log("off-stage beat");
                    note_advanced(pick);
                    log() << "[scheduler] advanced off-stage scene '" << pick
                          << "' (turn " << completed_turn << ")\n";
                } catch (const std::exception& e) {
                    log() << "  [scheduler] off-stage beat failed for '" << pick
                          << "': " << e.what() << "\n";
                }
            }
        }
    }

    if (!saves_dir_.empty()) save(saves_dir_);

    if (scene_count() > 1) {
        log() << "[scenes] " << scene_count() << " live | active=" << active_scene_id_
              << " |";
        for (const auto& id : scene_ids()) log() << ' ' << id;
        log() << "\n";
    }
    return outputs;
}

}  // namespace rhapsode
