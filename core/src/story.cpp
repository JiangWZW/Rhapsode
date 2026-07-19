#include "rhapsode/story.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/str_util.h"

namespace rhapsode {

namespace {

// The scheduler's policy lives here, in the engine. The Python side only runs
// the tool-use loop and returns the chosen scene_id.
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

// The lifecycle policy lives here too. This is the sole authority on cross-scene
// membership: after every beat it is asked, directly, whether that beat changed
// the set of live storylines -- never left to an optional tool the narrator might
// skip. The Python side only runs the completion and returns the JSON verdict.
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

Story Story::from_scene(Scene root) {
    Story story;
    story.world_ = root.world_ptr();      // the root's World becomes the Story's World
    story.active_scene_id_ = root.scene_id;
    story.adopt(std::move(root));
    return story;
}

Scene* Story::adopt(Scene s) {
    scenes_.push_back(std::make_unique<Scene>(std::move(s)));
    return scenes_.back().get();
}

Scene* Story::get_scene(const std::string& id) {
    for (auto& s : scenes_)
        if (s->scene_id == id) return s.get();
    return nullptr;
}

const Scene* Story::get_scene(const std::string& id) const {
    for (const auto& s : scenes_)
        if (s->scene_id == id) return s.get();
    return nullptr;
}

std::vector<std::string> Story::scene_ids() const {
    std::vector<std::string> ids;
    ids.reserve(scenes_.size());
    for (const auto& s : scenes_) ids.push_back(s->scene_id);
    return ids;
}

// -- Lifecycle ---------------------------------------------------------------

Scene* Story::fork_scene(const std::string& parent_id,
                         const std::string& new_id,
                         const std::vector<std::string>& cast,
                         const std::string& driving_intention) {
    Scene* parent = get_scene(parent_id);
    if (!parent) {
        log() << "  [story] fork_scene: unknown parent '" << parent_id << "'\n";
        return nullptr;
    }
    if (get_scene(new_id)) {
        log() << "  [story] fork_scene: scene '" << new_id << "' already exists\n";
        return nullptr;
    }

    Scene* child = adopt(parent->fork(new_id, cast));
    child->driving_intention = driving_intention;
    child->last_advanced = beat_clock_;

    // Seed the drive as an intention node in the first cast member's mind, so it
    // has graph presence and a charge the scheduler can read.
    if (!driving_intention.empty() && !cast.empty()) {
        auto it = world_->character_memories.find(cast.front());
        if (it != world_->character_memories.end()) {
            std::uint64_t id = it->second.seed_belief(
                driving_intention, cast, /*created_at=*/0,
                CharacterMemory::kAuthoredSeedWeight, /*type=*/"intention");
            (void)id;
            child->charge = CharacterMemory::kAuthoredSeedWeight;
        }
    }
    return child;
}

bool Story::conclude_scene(const std::string& id, const std::string& reason) {
    auto it = std::find_if(scenes_.begin(), scenes_.end(),
                           [&](const auto& s) { return s->scene_id == id; });
    if (it == scenes_.end()) {
        log() << "  [story] conclude_scene: unknown scene '" << id << "'\n";
        return false;
    }

    for (auto& ch : world_->characters) ch.leave_scene(id);
    scenes_.erase(it);
    log() << "  [story] conclude scene '" << id << "': " << reason << "\n";

    if (active_scene_id_ == id)
        active_scene_id_ = scenes_.empty() ? std::string{} : scenes_.front()->scene_id;
    return true;
}

bool Story::merge_scene(const std::string& from_id, const std::string& into_id) {
    Scene* from = get_scene(from_id);
    Scene* into = get_scene(into_id);
    if (!from || !into || from_id == into_id) {
        log() << "  [story] merge_scene: bad ids '" << from_id << "' -> '"
              << into_id << "'\n";
        return false;
    }

    for (auto& ch : world_->characters) {
        if (ch.in_scene(from_id)) {
            ch.join_scene(into_id);
            ch.leave_scene(from_id);
        }
    }
    log() << "  [story] merge scene '" << from_id << "' -> '" << into_id << "'\n";

    auto it = std::find_if(scenes_.begin(), scenes_.end(),
                           [&](const auto& s) { return s->scene_id == from_id; });
    scenes_.erase(it);
    if (active_scene_id_ == from_id) active_scene_id_ = into_id;
    return true;
}

int Story::apply_pending_ops() {
    auto ops = world_->take_pending_ops();
    int applied = 0;
    for (const auto& op : ops) {
        switch (op.kind) {
            case LifecycleKind::Fork: {
                // Derive a stable, unique child id from the source and clock.
                std::string new_id = op.source_scene_id + "_f" +
                                     std::to_string(beat_clock_) + "_" +
                                     std::to_string(applied);
                if (fork_scene(op.source_scene_id, new_id, op.cast,
                               op.driving_intention))
                    ++applied;
                break;
            }
            case LifecycleKind::Conclude:
                if (conclude_scene(op.source_scene_id, op.reason)) ++applied;
                break;
            case LifecycleKind::Merge:
                if (merge_scene(op.source_scene_id, op.target_scene_id)) ++applied;
                break;
            case LifecycleKind::Exit: {
                bool any = false;
                for (const auto& name : op.cast) {
                    Character* ch = world_->find_character(name);
                    if (ch && !ch->is_player && ch->in_scene(op.source_scene_id)) {
                        ch->leave_scene(op.source_scene_id);
                        log() << "  [lifecycle] " << ch->name << " exits '"
                              << op.source_scene_id << "' (no new storyline)\n";
                        any = true;
                    }
                }
                if (any) ++applied;
                break;
            }
        }
    }
    return applied;
}

// -- Scheduler bookkeeping ---------------------------------------------------

void Story::note_advanced(const std::string& scene_id) {
    ++beat_clock_;
    if (Scene* s = get_scene(scene_id)) s->last_advanced = beat_clock_;
}

// Highest-charge intention among a scene's cast minds; falls back to the scene's
// own authored drive. Writes the winning charge to *charge_out.
std::string Story::derive_intention(const Scene& s, float* charge_out) const {
    std::string best = s.driving_intention;
    float best_w = s.charge;

    for (const auto& ch : world_->characters) {
        if (!ch.in_scene(s.scene_id)) continue;
        auto it = world_->character_memories.find(ch.name);
        if (it == world_->character_memories.end()) continue;
        it->second.beliefs().for_each([&](const Node& n) {
            if (n.type != "intention" || n.state != NodeState::Active) return;
            if (n.weight > best_w) { best_w = n.weight; best = n.fact; }
        }, /*all=*/false);
    }

    if (charge_out) *charge_out = best_w;
    return best;
}

std::string Story::tool_list_scenes() const {
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& s : scenes_) {
        nlohmann::json row;
        row["scene_id"]   = s->scene_id;
        row["title"]      = s->title;
        row["active"]     = (s->scene_id == active_scene_id_);
        row["turn_index"] = s->turn_index;
        row["staleness"]  = beat_clock_ - s->last_advanced;

        nlohmann::json cast = nlohmann::json::array();
        bool player_present = false;
        for (const auto& ch : world_->characters) {
            if (!ch.in_scene(s->scene_id)) continue;
            cast.push_back(ch.name);
            if (ch.is_player) player_present = true;
        }
        row["cast"] = std::move(cast);
        row["player_present"] = player_present;

        float charge = 0.0f;
        row["driving_intention"] = derive_intention(*s, &charge);
        row["charge"] = charge;

        std::string last;
        const auto& msgs = s->history.messages();
        for (auto m = msgs.rbegin(); m != msgs.rend(); ++m) {
            if (m->role == Role::Assistant) {
                last = m->content.size() > 240 ? m->content.substr(0, 240) + "..."
                                               : m->content;
                break;
            }
        }
        row["last_narration"] = last;

        rows.push_back(std::move(row));
    }
    return rows.dump();
}

// -- Tool dispatch -----------------------------------------------------------

std::string Story::dispatch_tool(const std::string& scene_id,
                                 const std::string& name,
                                 const std::string& args_json) {
    nlohmann::json args = nlohmann::json::parse(args_json, nullptr, /*allow_exceptions=*/false);
    if (!args.is_object()) args = nlohmann::json::object();

    auto str_arg = [&](const char* key) -> std::string {
        auto it = args.find(key);
        return (it != args.end() && it->is_string()) ? it->get<std::string>() : std::string{};
    };

    if (name == "query_graph") return world_->tool_query_graph(str_arg("query"));
    if (name == "query_mind")  return world_->tool_query_mind(str_arg("character"));
    if (name == "query_history") {
        Scene* s = get_scene(scene_id);
        if (!s) return nlohmann::json{{"error", "unknown scene: " + scene_id}}.dump();
        return s->tool_query_history(str_arg("query"));
    }
    if (name == "list_scenes") return tool_list_scenes();

    // Lifecycle is no longer an optional narrator/scheduler tool: it is decided by
    // the dedicated verdict in Story::decide_lifecycle. Only read tools route here.
    return nlohmann::json{{"error", "unknown tool: " + name}}.dump();
}

// -- The turn ----------------------------------------------------------------

void Story::point_loop_at(Scene* s) {
    loop_->load_scene(*s);
    if (downsampler_cb_) s->downsampler.set_llm_callback(downsampler_cb_);
}

std::string Story::pick_off_stage_scene() {
    if (!scheduler_cb_) return "";

    // Observability: log the rows the scheduler will weigh.
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

    // Describe the beat just authored: the scene, its cast now, the player's
    // action (if any), the fresh narration, and the other live storylines (so a
    // merge can name a target).
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
            if (ch && ch->is_player) continue;  // the player never leaves their thread
            out.push_back(c.get<std::string>());
        }
        return out;
    };

    // The player's own storyline is never concluded or merged away out from under
    // them -- the player is always in it, so it must always survive (the same
    // invariant that forbids forking the player out). Only fork/exit may touch a
    // scene the player occupies. This also guarantees the scene set is never left
    // empty, which would strand advance_scene with no active scene.
    bool player_here = false;
    for (const auto& ch : world_->characters)
        if (ch.is_player && ch.in_scene(scene_id)) { player_here = true; break; }

    // Conclude and merge retire THIS scene, so they win outright and skip the rest.
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
        std::vector<std::string> cast = without_player(it->value("cast", nlohmann::json::array()));
        const std::string intent = it->value("driving_intention", "");
        if (!cast.empty()) {
            world_->stage_fork(scene_id, intent, cast);
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

void Story::sync_beat(Scene* s) {
    MemorySystem* mem = world_->memory();
    if (!mem || !loop_ || !s) return;

    try {
        const auto expiry = loop_->take_completed_expiry_ops();
        if (!expiry.empty()) {
            std::vector<Node> nodes;
            nodes.reserve(expiry.size());
            for (const auto& op : expiry)
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
        const DirectorOutput& out = loop_->last_director_output();
        if (!out.new_nodes.empty())    mem->process_new_nodes(out.new_nodes, s->turn_index);
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

    // --- Player beat -------------------------------------------------------
    point_loop_at(active);
    loop_->submit_input(player_input);
    decide_lifecycle(active_scene_id_, player_input);
    apply_and_log("player beat");
    note_advanced(active_scene_id_);
    sync_beat(active);

    // Capture player-facing outputs before an off-stage beat repoints the loop
    // (take_last_turn_outputs clears on read).
    std::vector<SceneMessage> outputs = loop_->take_last_turn_outputs();

    // --- One off-stage beat ------------------------------------------------
    if (scene_count() > 1) {
        const std::string pick = pick_off_stage_scene();
        if (!pick.empty()) {
            if (Scene* s = get_scene(pick)) {
                point_loop_at(s);
                try {
                    loop_->submit_autonomous(autonomous_cue(pick));
                    decide_lifecycle(pick, "");
                    apply_and_log("off-stage beat");
                    note_advanced(pick);
                    sync_beat(s);
                    log() << "[scheduler] advanced off-stage scene '" << pick
                          << "' (turn " << s->turn_index << ")\n";
                } catch (const std::exception& e) {
                    log() << "  [scheduler] off-stage beat failed for '" << pick
                          << "': " << e.what() << "\n";
                    loop_->join_background();
                }
            }
        }
    }

    // --- Persist -----------------------------------------------------------
    if (!saves_dir_.empty()) save(saves_dir_);

    if (scene_count() > 1) {
        log() << "[scenes] " << scene_count() << " live | active=" << active_scene_id_
              << " |";
        for (const auto& id : scene_ids()) log() << ' ' << id;
        log() << "\n";
    }
    return outputs;
}

// -- Persistence -------------------------------------------------------------

namespace {
std::string manifest_path(const std::string& saves_dir) {
    return saves_dir + "/story.json";
}
}  // namespace

bool Story::has_save(const std::string& saves_dir) const {
    // A story save needs the shared world blob plus the manifest. Fall back to a
    // legacy single-scene save (world.json + the active scene's blob, no
    // manifest) so pre-Story saves still resume.
    if (!world_->has_save(saves_dir)) return false;
    if (std::filesystem::exists(manifest_path(saves_dir))) return true;
    const Scene* root = get_scene(active_scene_id_);
    return root && root->has_ephemeral_save(saves_dir);
}

void Story::load_save(const std::string& saves_dir) {
    world_->load_save(saves_dir);

    const std::string mpath = manifest_path(saves_dir);
    if (!std::filesystem::exists(mpath)) {
        // Legacy single-scene resume: keep the root scene, load its blob.
        if (Scene* root = get_scene(active_scene_id_))
            root->load_ephemeral(saves_dir);
        return;
    }

    std::ifstream in(mpath);
    nlohmann::json j;
    in >> j;

    active_scene_id_ = j.value("active_scene_id", active_scene_id_);
    beat_clock_ = j.value("beat_clock", 0);

    // Rebuild the scene set from the manifest, all sharing this World. The root
    // scene (already adopted) is reused; others are created empty and loaded.
    std::vector<std::unique_ptr<Scene>> rebuilt;
    for (const auto& sid_j : j.value("scene_ids", nlohmann::json::array())) {
        std::string sid = sid_j.get<std::string>();
        std::unique_ptr<Scene> sc;
        if (Scene* existing = get_scene(sid)) {
            sc = std::make_unique<Scene>(std::move(*existing));
        } else {
            sc = std::make_unique<Scene>();
            sc->set_world(world_);
            sc->scene_id = sid;
        }
        sc->load_ephemeral(saves_dir);
        rebuilt.push_back(std::move(sc));
    }
    if (!rebuilt.empty()) scenes_ = std::move(rebuilt);
}

void Story::save(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);
    world_->save(saves_dir);
    for (const auto& s : scenes_) s->save_ephemeral(saves_dir);

    nlohmann::json j;
    j["active_scene_id"] = active_scene_id_;
    j["beat_clock"]      = beat_clock_;
    j["scene_ids"]       = scene_ids();

    std::ofstream out(manifest_path(saves_dir));
    if (!out.is_open())
        throw std::runtime_error("Cannot write story manifest in: " + saves_dir);
    out << j.dump(2);
}

void Story::delete_save(const std::string& saves_dir) const {
    world_->delete_save(saves_dir);
    std::filesystem::remove(manifest_path(saves_dir));
    for (const auto& s : scenes_)
        std::filesystem::remove(saves_dir + "/" + s->scene_id + ".json");
}

} // namespace rhapsode
