#include "rhapsode/story.h"

#include <algorithm>
#include <stdexcept>

#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/scenario_bootstrap.h"
#include "rhapsode/scene_history.h"
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

SceneData* Story::fork_scene(const std::string& parent_id,
                             const std::string& new_id,
                             const std::vector<std::string>& cast,
                             const std::string& driving_intention) {
    SceneData* parent = get_scene(parent_id);
    if (!parent) {
        log() << "  [story] fork_scene: unknown parent '" << parent_id << "'\n";
        return nullptr;
    }
    if (get_scene(new_id)) {
        log() << "  [story] fork_scene: scene '" << new_id << "' already exists\n";
        return nullptr;
    }

    SceneData child;
    child.scene_id = new_id;
    child.title = parent->title;
    child.system_prompt = parent->system_prompt;
    child.driving_intention = driving_intention;
    child.last_advanced = beat_clock_;
    SceneData* adopted = adopt_scene(std::move(child));

    world_->move_scene_members(parent_id, new_id, cast);
    log() << "  [fork] scene '" << parent_id << "' -> '" << new_id << "' with cast [";
    for (size_t i = 0; i < cast.size(); ++i) {
        if (i) log() << ", ";
        log() << cast[i];
    }
    log() << "]\n";

    if (!driving_intention.empty() && !cast.empty() &&
        world_->seed_character_intention(cast.front(), driving_intention, cast, 0)) {
        adopted->charge = CharacterMemory::kAuthoredSeedWeight;
    }
    return adopted;
}

bool Story::conclude_scene(const std::string& id, const std::string& reason) {
    auto it = std::find_if(scenes_.begin(), scenes_.end(),
        [&](const auto& scene) { return scene->scene_id == id; });
    if (it == scenes_.end()) {
        log() << "  [story] conclude_scene: unknown scene '" << id << "'\n";
        return false;
    }
    world_->clear_scene_membership(id);
    scenes_.erase(it);
    log() << "  [story] conclude scene '" << id << "': " << reason << "\n";
    if (active_scene_id_ == id)
        active_scene_id_ = scenes_.empty() ? std::string{} : scenes_.front()->scene_id;
    return true;
}

bool Story::merge_scene(const std::string& from_id, const std::string& into_id) {
    if (!get_scene(from_id) || !get_scene(into_id) || from_id == into_id) {
        log() << "  [story] merge_scene: bad ids '" << from_id << "' -> '"
              << into_id << "'\n";
        return false;
    }
    world_->move_scene_members(from_id, into_id);
    auto it = std::find_if(scenes_.begin(), scenes_.end(),
        [&](const auto& scene) { return scene->scene_id == from_id; });
    scenes_.erase(it);
    if (active_scene_id_ == from_id) active_scene_id_ = into_id;
    log() << "  [story] merge scene '" << from_id << "' -> '" << into_id << "'\n";
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
        summary.last_narration = it->content.size() > 240
            ? it->content.substr(0, 240) + "..." : it->content;
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
    const ReadToolContext context{
        world_.get(),
        scene ? &scene->history : nullptr,
        scene_id,
        tool_list_scenes(),
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

void Story::set_weaver_local_llm_callback(LLMCallback cb) {
    weaver_->set_local_llm_callback(std::move(cb));
}

void Story::set_weaver_interval(int turns) { weaver_->set_interval(turns); }

WeaveResult Story::weave_scene(const std::string& scene_id) {
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
