#include "rhapsode/story.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "rhapsode/character_memory.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene_loop.h"
#include "rhapsode/str_util.h"

namespace rhapsode {

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

// -- Undo -------------------------------------------------------------------

int Story::revert_active_turns(int count) {
    if (!loop_) throw std::runtime_error("Story::revert_active_turns: runtime not bound");
    Scene* active = active_scene();
    if (!active) throw std::runtime_error("Story::revert_active_turns: no active scene");

    loop_->join_background();
    const int reverted = active->revert_turns(count);
    loop_->set_resuming(true);
    if (!saves_dir_.empty()) save(saves_dir_);
    return reverted;
}

} // namespace rhapsode
