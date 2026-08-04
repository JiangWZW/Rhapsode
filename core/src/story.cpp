#include "rhapsode/story.h"

#include <algorithm>
#include <atomic>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/scenario_bootstrap.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/turn_executor.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

Story::Story()
    : world_(std::make_unique<World>()),
      director_(std::make_unique<Director>(world_->graph())),
      weaver_(std::make_unique<Weaver>(world_->graph())),
      executor_(std::make_unique<TurnExecutor>(*world_, *director_, *weaver_)) {}

Story::~Story() = default;
Story::Story(Story&&) noexcept = default;
Story& Story::operator=(Story&& other) noexcept {
    if (this == &other) return *this;

    // Tear down borrowers before replacing the state they reference. The
    // transferred service allocations already refer to other's World, whose
    // allocation is transferred unchanged below.
    executor_.reset();
    weaver_.reset();
    director_.reset();

    world_ = std::move(other.world_);
    scenes_ = std::move(other.scenes_);
    scene_closures_ = std::move(other.scene_closures_);
    active_scene_id_ = std::move(other.active_scene_id_);
    beat_clock_ = other.beat_clock_;
    scheduler_cb_ = std::move(other.scheduler_cb_);
    lifecycle_cb_ = std::move(other.lifecycle_cb_);
    saves_dir_ = std::move(other.saves_dir_);
    memory_ = std::move(other.memory_);

    director_ = std::move(other.director_);
    weaver_ = std::move(other.weaver_);
    executor_ = std::move(other.executor_);
    return *this;
}

Story Story::load_scenario(const std::string& path) {
    auto scenario = load_scenario_file(path);
    return from_data(std::move(scenario.scene), std::move(scenario.world));
}

Story Story::from_scenario_json(const nlohmann::json& scenario,
                                const std::string& scene_id) {
    auto result = bootstrap_scenario(scenario, scene_id);
    return from_data(std::move(result.scene), std::move(result.world));
}

Story Story::from_data(SceneData root, World world) {
    Story story;
    *story.world_ = std::move(world);
    story.active_scene_id_ = root.scene_id;
    story.adopt_scene(std::move(root));
    return story;
}

nlohmann::json Story::to_scenario_json(const std::string& scene_id) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) throw std::runtime_error("Unknown scene: " + scene_id);
    return serialize_scenario(*scene, *world_);
}

SceneData* Story::adopt_scene(SceneData scene) {
    scenes_.push_back(std::make_unique<SceneData>(std::move(scene)));
    return scenes_.back().get();
}

SceneData* Story::get_scene(const std::string& id) {
    for (auto& scene : scenes_)
        if (scene->scene_id == id) return scene.get();
    return nullptr;
}

const SceneData* Story::get_scene(const std::string& id) const {
    for (const auto& scene : scenes_)
        if (scene->scene_id == id) return scene.get();
    return nullptr;
}

std::vector<std::string> Story::scene_ids() const {
    std::vector<std::string> ids;
    ids.reserve(scenes_.size());
    for (const auto& scene : scenes_) ids.push_back(scene->scene_id);
    return ids;
}

void Story::set_active_scene(const std::string& id) {
    if (!get_scene(id)) throw std::invalid_argument("Unknown scene: " + id);
    active_scene_id_ = id;
}

std::optional<std::vector<std::string>> Story::resolve_non_player_members(
    const std::string& scene_id,
    const std::vector<std::string>& names) const {
    std::vector<std::string> resolved;
    resolved.reserve(names.size());
    std::unordered_set<std::string> seen;
    for (const auto& requested : names) {
        if (str::trim(requested).empty()) return std::nullopt;
        const Character* character = world_->find_in_scene(scene_id, requested);
        if (!character || character->is_player) return std::nullopt;
        const std::string key = str::to_lower(character->name);
        if (!seen.insert(key).second) return std::nullopt;
        resolved.push_back(character->name);
    }
    return resolved;
}

std::optional<std::vector<std::string>> Story::resolve_fork_cast(
    const std::string& parent_id,
    const std::vector<std::string>& names,
    const std::string& driving_intention) const {
    if (!get_scene(parent_id) || str::trim(driving_intention).empty())
        return std::nullopt;
    auto resolved = resolve_non_player_members(parent_id, names);
    if (!resolved || resolved->empty()) return std::nullopt;

    std::unordered_set<std::string> departing;
    for (const auto& name : *resolved)
        departing.insert(str::to_lower(name));
    bool parent_keeps_cast = false;
    for (const auto& character : world_->characters()) {
        if (character.dead || !character.in_scene(parent_id)) continue;
        if (departing.count(str::to_lower(character.name)) == 0) {
            parent_keeps_cast = true;
            break;
        }
    }
    return parent_keeps_cast ? resolved : std::nullopt;
}

SceneData* Story::fork_scene(const std::string& parent_id,
                             const std::string& new_id,
                             const std::vector<std::string>& cast,
                             const std::string& driving_intention) {
    SceneData* parent = get_scene(parent_id);
    if (!parent) {
        log() << "  [story] fork_scene: unknown parent '" << parent_id << "'\n";
        return nullptr;
    }
    if (new_id.empty() || get_scene(new_id)) {
        log() << "  [story] fork_scene: scene '" << new_id << "' already exists\n";
        return nullptr;
    }

    const auto resolved_cast = resolve_fork_cast(
        parent_id, cast, driving_intention);
    if (!resolved_cast) {
        log() << "  [story] fork_scene: invalid cast or intention for '"
              << parent_id << "'\n";
        return nullptr;
    }

    std::unordered_map<std::string, const std::vector<SceneMessage>*> histories;
    for (const auto& scene : scenes_)
        histories.emplace(scene->scene_id, &scene->history);
    const ReadToolContext context{
        world_.get(),
        &parent->history,
        parent_id,
        tool_list_scenes(),
        std::move(histories),
    };
    const auto read_tools_active = std::make_shared<std::atomic_bool>(true);
    const ReadToolCallback read_tool =
        [context, active = read_tools_active](
            const std::string& name, const std::string& args_json) {
            if (!active->load())
                throw std::runtime_error("Read tool callback is no longer active");
            return dispatch_read_tool(context, name, args_json);
        };

    std::string fork_story_so_far;
    try {
        fork_story_so_far = executor_->synthesize_fork_context(
            *parent, *resolved_cast, str::trim(driving_intention), read_tool);
        read_tools_active->store(false);
    } catch (const std::exception& error) {
        read_tools_active->store(false);
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
    child.last_advanced = beat_clock_;

    World next_world = *world_;
    child.intention_owner = resolved_cast->front();
    child.intention_node_id = next_world.seed_character_intention(
        child.intention_owner, child.driving_intention,
        *resolved_cast, beat_clock_);
    if (child.intention_node_id == 0) {
        log() << "  [story] fork_scene: could not seed intention for '"
              << child.intention_owner << "'\n";
        return nullptr;
    }
    child.charge = CharacterMemory::kAuthoredSeedWeight;
    next_world.move_scene_members(parent_id, new_id, *resolved_cast);

    SceneData* adopted = adopt_scene(std::move(child));
    *world_ = std::move(next_world);
    std::ostringstream cast_line;
    for (size_t i = 0; i < resolved_cast->size(); ++i) {
        if (i) cast_line << ", ";
        cast_line << (*resolved_cast)[i];
    }
    log_info("story") << "fork from=" << parent_id << " to=" << new_id
                      << " cast=[" << cast_line.str() << "]\n";
    return adopted;
}

bool Story::conclude_scene(const std::string& id, const std::string& reason) {
    auto it = std::find_if(scenes_.begin(), scenes_.end(),
        [&](const auto& scene) { return scene->scene_id == id; });
    if (it == scenes_.end()) {
        log() << "  [story] conclude_scene: unknown scene '" << id << "'\n";
        return false;
    }

    const std::string clean_reason = str::trim(reason);
    if (clean_reason.empty() || scenes_.size() <= 1) {
        log() << "  [story] conclude_scene: refused '" << id
              << "' (empty reason or final live scene)\n";
        return false;
    }

    SceneClosure closure;
    closure.scene_id = id;
    closure.reason = clean_reason;
    closure.driving_intention = (*it)->driving_intention;
    closure.story_so_far = render_text_downsampling((*it)->downsampling);
    closure.concluded_at = beat_clock_;
    for (const auto& character : world_->characters()) {
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

    World next_world = *world_;
    if ((*it)->intention_node_id != 0 &&
        !next_world.expire_character_intention(
            (*it)->intention_owner, (*it)->intention_node_id, beat_clock_)) {
        log() << "  [story] conclude_scene: owned intention missing for '"
              << id << "'\n";
        return false;
    }
    next_world.clear_scene_membership(id);

    scene_closures_.push_back(std::move(closure));
    *world_ = std::move(next_world);
    scenes_.erase(it);
    log_info("story") << "conclude scene=" << id << ": " << clean_reason << "\n";
    if (active_scene_id_ == id)
        active_scene_id_ = scenes_.front()->scene_id;
    return true;
}

bool Story::merge_scene(const std::string& from_id, const std::string& into_id) {
    SceneData* source = get_scene(from_id);
    SceneData* target = get_scene(into_id);
    if (!source || !target || from_id == into_id) {
        log() << "  [story] merge_scene: bad ids '" << from_id << "' -> '"
              << into_id << "'\n";
        return false;
    }
    for (const auto& character : world_->characters()) {
        if (character.is_player && character.in_scene(from_id)) {
            log() << "  [story] merge_scene: source '" << from_id
                  << "' contains the Player\n";
            return false;
        }
    }

    std::unordered_map<std::string, const std::vector<SceneMessage>*> histories;
    for (const auto& scene : scenes_)
        histories.emplace(scene->scene_id, &scene->history);
    const ReadToolContext context{
        world_.get(),
        &target->history,
        into_id,
        tool_list_scenes(),
        std::move(histories),
    };
    const auto read_tools_active = std::make_shared<std::atomic_bool>(true);
    const ReadToolCallback read_tool =
        [context, active = read_tools_active](
            const std::string& name, const std::string& args_json) {
            if (!active->load())
                throw std::runtime_error("Read tool callback is no longer active");
            return dispatch_read_tool(context, name, args_json);
        };

    std::string merged_story_so_far;
    try {
        merged_story_so_far = executor_->synthesize_merge_context(
            *source, *target, read_tool);
        read_tools_active->store(false);
    } catch (const std::exception& error) {
        read_tools_active->store(false);
        log() << "  [story] merge synthesis failed for '" << from_id
              << "' -> '" << into_id << "': " << error.what() << "\n";
        return false;
    }

    constexpr int kVerbatimTail = 6;
    const int summarized_up_to = std::max(
        0, static_cast<int>(target->history.size()) - kVerbatimTail);
    DownsamplingState merged_downsampling = text_downsampling_from_summary(
        std::move(merged_story_so_far), summarized_up_to);

    target->downsampling = std::move(merged_downsampling);
    world_->move_scene_members(from_id, into_id);
    auto it = std::find_if(scenes_.begin(), scenes_.end(),
        [&](const auto& scene) { return scene->scene_id == from_id; });
    scenes_.erase(it);
    if (active_scene_id_ == from_id) active_scene_id_ = into_id;
    log_info("story") << "merge from=" << from_id << " into=" << into_id << "\n";
    return true;
}

void Story::note_advanced(const std::string& scene_id) {
    ++beat_clock_;
    if (SceneData* scene = get_scene(scene_id)) scene->last_advanced = beat_clock_;
}

Story::SceneDrive Story::derive_intention(const SceneData& scene) const {
    SceneDrive drive{scene.driving_intention, scene.charge};
    for (const auto& character : world_->characters()) {
        if (!character.in_scene(scene.scene_id)) continue;
        const auto it = world_->character_memories().find(character.name);
        if (it == world_->character_memories().end()) continue;
        it->second.beliefs().for_each([&](const Node& node) {
            if (node.type == "intention" && node.state == NodeState::Active &&
                node.weight > drive.charge) {
                drive.charge = node.weight;
                drive.intention = node.fact;
            }
        }, false);
    }
    return drive;
}

SceneSummary Story::summarize_scene(const SceneData& scene) const {
    SceneSummary summary;
    summary.scene_id = scene.scene_id;
    summary.title = scene.title;
    summary.active = scene.scene_id == active_scene_id_;
    summary.turn_index = scene.turn_index;
    summary.staleness = beat_clock_ - scene.last_advanced;

    for (const auto& character : world_->characters()) {
        if (!character.in_scene(scene.scene_id)) continue;
        summary.cast.push_back(character.name);
        if (character.is_player) summary.player_present = true;
    }

    SceneDrive drive = derive_intention(scene);
    summary.driving_intention = std::move(drive.intention);
    summary.charge = drive.charge;

    for (auto it = scene.history.rbegin(); it != scene.history.rend(); ++it) {
        if (it->role != Role::Assistant) continue;
        // Byte-substr mid codepoint breaks nlohmann dump() (UTF-8 type_error.316).
        summary.last_narration = truncate_utf8_ellipsis(it->content, 240);
        // Board lifecycle payload only — keeps list_scenes payloads small.
        summary.recent_narration = truncate_utf8_ellipsis(it->content, 1200);
        break;
    }
    return summary;
}

std::vector<SceneSummary> Story::summarize_scenes() const {
    std::vector<SceneSummary> summaries;
    summaries.reserve(scenes_.size());
    for (const auto& scene : scenes_)
        summaries.push_back(summarize_scene(*scene));
    return summaries;
}

std::string Story::tool_list_scenes() const {
    return serialize_scene_summaries(summarize_scenes());
}

std::string Story::dispatch_tool(const std::string& scene_id,
                                 const std::string& name,
                                 const std::string& args_json) {
    const SceneData* scene = get_scene(scene_id);
    std::unordered_map<std::string, const std::vector<SceneMessage>*> histories;
    for (const auto& item : scenes_)
        histories.emplace(item->scene_id, &item->history);
    const ReadToolContext context{
        world_.get(),
        scene ? &scene->history : nullptr,
        scene_id,
        tool_list_scenes(),
        std::move(histories),
    };
    return dispatch_read_tool(context, name, args_json);
}

std::vector<SceneMessage> Story::display_timeline(
    const std::string& scene_id,
    std::optional<size_t> cap) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) return {};
    std::vector<SceneMessage> merged;
    merged.reserve(scene->history.size() + scene->dialogue.size());
    for (const auto& message : scene->history) merged.push_back(message);
    for (const auto& message : scene->dialogue) merged.push_back(message);
    std::sort(merged.begin(), merged.end(),
        [](const SceneMessage& left, const SceneMessage& right) {
            return left.timestamp < right.timestamp;
        });
    if (cap && merged.size() > *cap)
        return {merged.end() - static_cast<ptrdiff_t>(*cap), merged.end()};
    return merged;
}

namespace {

std::string message_speaker_label(const SceneMessage& message) {
    std::string kind;
    if (message.metadata.is_object() &&
        message.metadata.contains("scene_kind") &&
        message.metadata["scene_kind"].is_string())
        kind = message.metadata["scene_kind"].get<std::string>();

    // Autonomous off-stage inputs are director cues, not player speech.
    if (kind == "director_cue") return "Off-stage cue";
    if (message.role == Role::User) return "Player";
    if (message.role == Role::System) return "System";

    std::string speaker;
    if (message.metadata.is_object() &&
        message.metadata.contains("speaker") &&
        message.metadata["speaker"].is_string())
        speaker = message.metadata["speaker"].get<std::string>();
    if (!speaker.empty()) return speaker;
    if (kind == "character") return "Character";
    return "Narrator";
}

void append_timeline(std::ostringstream& out, const Story& story,
                     const std::string& scene_id) {
    for (const auto& message : story.display_timeline(scene_id)) {
        if (message.role == Role::System) continue;
        // Omit the internal autonomous prompt; section headers already carry
        // intention, and the following Narrator beat is the readable content.
        if (message.metadata.is_object() &&
            message.metadata.value("scene_kind", std::string{}) == "director_cue")
            continue;
        const std::string content = str::trim(message.content);
        if (content.empty()) continue;
        out << "[" << message_speaker_label(message) << "]\n"
            << content << "\n\n";
    }
}

}  // namespace

std::string Story::render_transcript() const {
    std::ostringstream out;
    out << "# Story transcript\n\n";
    out << "Active scene: " << active_scene_id_ << "\n";
    out << "Live storylines: " << scenes_.size()
        << "  Concluded: " << scene_closures_.size() << "\n\n";

    const std::string active = active_scene_id_;
    if (const SceneData* scene = active_scene()) {
        out << "============================================================\n";
        out << "## Main — " << scene->scene_id;
        if (!scene->title.empty()) out << " (" << scene->title << ")";
        out << "\n";
        out << "turn=" << scene->turn_index;
        if (!scene->driving_intention.empty())
            out << "  intention=" << scene->driving_intention;
        out << "\n\n";
        append_timeline(out, *this, scene->scene_id);
    }

    for (const auto& scene : scenes_) {
        if (!scene || scene->scene_id == active) continue;
        out << "============================================================\n";
        out << "## Off-stage — " << scene->scene_id;
        if (!scene->title.empty()) out << " (" << scene->title << ")";
        out << "\n";
        out << "turn=" << scene->turn_index;
        if (!scene->driving_intention.empty())
            out << "  intention=" << scene->driving_intention;
        out << "\n\n";
        append_timeline(out, *this, scene->scene_id);
    }

    for (const auto& closure : scene_closures_) {
        out << "============================================================\n";
        out << "## Concluded — " << closure.scene_id
            << " (beat " << closure.concluded_at << ")\n";
        if (!closure.driving_intention.empty())
            out << "intention=" << closure.driving_intention << "\n";
        if (!closure.cast.empty()) {
            out << "cast=";
            for (size_t i = 0; i < closure.cast.size(); ++i) {
                if (i) out << ", ";
                out << closure.cast[i];
            }
            out << "\n";
        }
        if (!closure.reason.empty())
            out << "reason=" << closure.reason << "\n";
        out << "\n";
        if (!closure.story_so_far.empty()) {
            out << "[Story so far]\n" << closure.story_so_far << "\n\n";
        }
        if (!closure.final_narration.empty()) {
            out << "[Final narration]\n" << closure.final_narration << "\n\n";
        }
    }
    return out.str();
}

int Story::revert_scene_turns(SceneData& scene, int count) {
    if (count <= 0 || scene.turn_index <= 0) return 0;
    const int target = std::max(0, scene.turn_index - count);
    const int actual = scene.turn_index - target;

    const auto& messages = scene.history;
    int user_count = 0;
    size_t cut = messages.size();
    for (size_t i = messages.size(); i > 0; --i) {
        if (messages[i - 1].role == Role::User && ++user_count >= actual) {
            cut = i - 1;
            break;
        }
    }
    truncate_history(scene.history, cut);
    drop_history_from_turn(scene.dialogue, target);
    const auto removed_ids = world_->revert_to_turn(target);
    if (memory_ && !removed_ids.empty()) memory_->delete_nodes(removed_ids);
    scene.turn_index = target;
    log() << "  [undo] Reverted " << actual << " turn(s), now at turn "
          << target << "\n" << std::flush;
    return actual;
}

int Story::revert_active_turns(int count) {
    pending_turn_.reset();
    SceneData* scene = active_scene();
    if (!scene) throw std::runtime_error("Story::revert_active_turns: no active scene");
    const int reverted = revert_scene_turns(*scene, count);
    executor_->set_resuming(true);
    if (!saves_dir_.empty()) save(saves_dir_);
    return reverted;
}

void Story::set_llm_callback(LLMCallback cb) {
    executor_->set_llm_callback(std::move(cb));
}

void Story::set_narrator_llm_callback(NarratorLLMCallback cb) {
    executor_->set_narrator_llm_callback(std::move(cb));
}

void Story::set_weaver_llm_callback(LLMCallback cb) {
    weaver_->set_llm_callback(std::move(cb));
}

void Story::set_weaver_interval(int turns) { weaver_->set_interval(turns); }

WeaveResult Story::weave_scene(const std::string& scene_id) {
    if (pending_turn_)
        throw std::runtime_error(
            "Story::weave_scene: complete_turn pending from advance_player");
    const SceneData* scene = get_scene(scene_id);
    if (!scene) throw std::invalid_argument("Unknown scene: " + scene_id);
    return weaver_->weave(scene->turn_index);
}

void Story::set_history_window(size_t normal, size_t resume) {
    executor_->set_history_window(normal, resume);
}
void Story::set_resuming(bool value) { executor_->set_resuming(value); }
void Story::set_downsampler_callback(LLMCallback cb) {
    executor_->set_downsampler_callback(std::move(cb));
}

void Story::set_reflection_llm_callback(LLMCallback cb) {
    executor_->set_reflection_llm_callback(std::move(cb));
}

}  // namespace rhapsode
