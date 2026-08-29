#include "rhapsode/story.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/scenario_bootstrap.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story_lifecycle.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/turn_pipeline.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

Story::Story() = default;

Story::~Story() = default;
Story::Story(Story&&) noexcept = default;
Story& Story::operator=(Story&&) noexcept = default;

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
    import_world(story.data_, std::move(world));
    story.data_.active_scene_id = root.scene_id;
    adopt_scene(story.data_, std::move(root));
    return story;
}

nlohmann::json Story::to_scenario_json(const std::string& scene_id) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) throw std::runtime_error("Unknown scene: " + scene_id);
    return serialize_scenario(*scene, data_.world);
}

SceneData* Story::get_scene(const std::string& id) {
    return find_scene(data_, id);
}

World Story::world_snapshot() const { return snapshot_world(data_); }

const SceneData* Story::get_scene(const std::string& id) const {
    return find_scene(data_, id);
}

std::vector<std::string> Story::scene_ids() const {
    return collect_scene_ids(data_);
}

void Story::set_active_scene(const std::string& id) {
    if (!get_scene(id)) throw std::invalid_argument("Unknown scene: " + id);
    data_.active_scene_id = id;
}

SceneData* Story::fork_scene(const std::string& parent_id,
                             const std::string& new_id,
                             const std::vector<std::string>& cast,
                             const std::string& driving_intention) {
    return fork_story_scene(
        data_, services_, parent_id, new_id, cast, driving_intention);
}

bool Story::conclude_scene(const std::string& id, const std::string& reason) {
    return conclude_story_scene(data_, id, reason);
}

bool Story::merge_scene(const std::string& from_id, const std::string& into_id) {
    return merge_story_scene(data_, services_, from_id, into_id);
}

void Story::note_advanced(const std::string& scene_id) {
    note_scene_advanced(data_, scene_id);
}

std::string Story::tool_list_scenes() const {
    return serialize_scene_summaries(summarize_story_scenes(data_));
}

std::string Story::player_situation() const {
    const SceneData* scene = active_scene();
    if (!scene) return {};

    std::vector<std::string> on_stage;
    std::vector<std::string> off_stage;
    for (const auto& character : data_.world.characters()) {
        if (character.is_player || character.dead) continue;
        if (character.in_scene(scene->scene_id))
            on_stage.push_back(character.name);
        else
            off_stage.push_back(character.name);
    }
    std::sort(off_stage.begin(), off_stage.end());

    std::ostringstream os;
    os << "### Situation\n";
    os << "Active scene: " << scene->scene_id << "\n";
    os << "On this stage: ";
    if (on_stage.empty()) {
        os << "(you are alone)\n";
    } else {
        for (size_t i = 0; i < on_stage.size(); ++i) {
            if (i) os << ", ";
            os << on_stage[i];
        }
        os << "\n";
    }
    if (!off_stage.empty()) {
        os << "Not on this stage: ";
        for (size_t i = 0; i < off_stage.size(); ++i) {
            if (i) os << ", ";
            os << off_stage[i];
        }
        os << "\n";
    }
    const std::string board = format_live_storylines_board(
        summarize_story_scenes(data_));
    if (!board.empty())
        os << "\n" << board;
    return os.str();
}

std::string Story::dispatch_tool(const std::string& scene_id,
                                 const std::string& name,
                                 const std::string& args_json) {
    const ReadToolContext context =
        make_story_read_tool_context(data_, scene_id);
    return dispatch_read_tool(context, name, args_json);
}

std::vector<SceneMessage> Story::display_timeline(
    const std::string& scene_id,
    std::optional<size_t> cap) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) return {};
    std::vector<SceneMessage> merged;
    const auto spans = attributed_transcript(*scene, cap);
    merged.reserve(spans.size());
    for (const auto& span : spans) merged.push_back(span.message);
    return merged;
}

namespace {

void append_closure_cast(std::ostringstream& out, const SceneClosure& closure) {
    if (closure.cast.empty()) return;
    out << "cast=";
    for (size_t i = 0; i < closure.cast.size(); ++i) {
        if (i) out << ", ";
        out << closure.cast[i];
    }
    out << "\n";
}

}  // namespace

std::string Story::render_transcript() const {
    std::ostringstream out;
    out << "# Story transcript\n\n";
    out << "Active scene: " << data_.active_scene_id << "\n";
    int concluded = 0;
    int merged = 0;
    for (const auto& closure : data_.scene_closures) {
        if (closure.merged_into.empty()) ++concluded;
        else ++merged;
    }
    out << "Live storylines: " << data_.scenes.size()
        << "  Concluded: " << concluded
        << "  Merged forks: " << merged << "\n\n";

    const std::string active = data_.active_scene_id;
    if (const SceneData* scene = active_scene()) {
        out << "============================================================\n";
        out << "## Main — " << scene->scene_id;
        if (!scene->title.empty()) out << " (" << scene->title << ")";
        out << "\n";
        out << "turn=" << scene->turn_index;
        if (!scene->driving_intention.empty())
            out << "  intention=" << scene->driving_intention;
        out << "\n\n";
        out << format_visible_transcript(*scene);
    }

    for (const auto& scene : data_.scenes) {
        if (!scene || scene->scene_id == active) continue;
        out << "============================================================\n";
        out << "## Off-stage — " << scene->scene_id;
        if (!scene->title.empty()) out << " (" << scene->title << ")";
        out << "\n";
        out << "turn=" << scene->turn_index;
        if (!scene->driving_intention.empty())
            out << "  intention=" << scene->driving_intention;
        out << "\n\n";
        out << format_visible_transcript(*scene);
    }

    for (const auto& closure : data_.scene_closures) {
        if (closure.merged_into.empty()) continue;
        out << "============================================================\n";
        out << "## Fork — " << closure.scene_id
            << " (merged into " << closure.merged_into << ")\n";
        out << "turn=" << closure.concluded_at << "\n";
        if (!closure.driving_intention.empty())
            out << "intention=" << closure.driving_intention << "\n";
        append_closure_cast(out, closure);
        if (!closure.reason.empty())
            out << "reason=" << closure.reason << "\n";
        out << "\n";
        if (!closure.story_so_far.empty())
            out << closure.story_so_far;
    }

    for (const auto& closure : data_.scene_closures) {
        if (!closure.merged_into.empty()) continue;
        out << "============================================================\n";
        out << "## Concluded — " << closure.scene_id
            << " (turn " << closure.concluded_at << ")\n";
        if (!closure.driving_intention.empty())
            out << "intention=" << closure.driving_intention << "\n";
        append_closure_cast(out, closure);
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
    if (count <= 0 || scene.turn_index < 0) return 0;
    const int target = std::max(-1, scene.turn_index - count);
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
    auto removed_ids = data_.observations.revert_to_turn(target);
    data_.world.revert_to_turn(target);
    if (services_.memory && !removed_ids.empty())
        services_.memory->delete_nodes(removed_ids);
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
    if (reverted > 0) ++data_.transaction_version;
    if (!services_.saves_dir.empty()) save(services_.saves_dir);
    return reverted;
}

void Story::set_llm_callback(LLMCallback cb) {
    services_.llm = std::move(cb);
}

void Story::set_narrator_llm_callback(NarratorLLMCallback cb) {
    services_.narrator = std::move(cb);
}

void Story::set_weaver_llm_callback(LLMCallback cb) {
    services_.weaver.set_llm_callback(std::move(cb));
}

void Story::set_weaver_interval(int turns) {
    services_.weaver.set_interval(turns);
}

WeaveResult Story::weave_scene(const std::string& scene_id) {
    if (pending_turn_)
        throw std::runtime_error(
            "Story::weave_scene: complete_turn pending from advance_player");
    const SceneData* scene = get_scene(scene_id);
    if (!scene) throw std::invalid_argument("Unknown scene: " + scene_id);
    WeaveResult result = services_.weaver.weave(
        data_.observations, scene->turn_index);
    ++data_.transaction_version;
    return result;
}

void Story::set_history_window(size_t normal, size_t /*resume*/) {
    services_.history_window = normal;
}
void Story::set_downsampler_callback(LLMCallback cb) {
    services_.downsampler = std::move(cb);
}

void Story::set_reflection_llm_callback(LLMCallback cb) {
    services_.reflection = std::move(cb);
}

void Story::set_perception_llm_callback(LLMCallback cb) {
    services_.perception = std::move(cb);
}

void Story::set_perception_ready_callback(MindReadyFn cb) {
    services_.perception_ready = std::move(cb);
}

void Story::set_perception_submit_callback(
    std::function<void(const std::vector<PromptJob>&)> cb) {
    services_.perception_submit = std::move(cb);
}

void Story::set_monologue_ready_callback(MindReadyFn cb) {
    services_.monologue_ready = std::move(cb);
}

void Story::set_monologue_submit_callback(
    std::function<void(const std::vector<PromptJob>&)> cb) {
    services_.monologue_submit = std::move(cb);
}

void Story::apply_ready_minds() {
    if (services_.perception_ready)
        data_.world.apply_ready_perceptions(
            0, services_.perception_ready);
    if (services_.monologue_ready)
        data_.world.apply_ready_monologues(
            0, services_.monologue_ready);
}

void Story::poll_minds() {
    if (pending_turn_) return;
    if (!services_.perception_ready || !services_.perception_submit) return;
    if (!services_.monologue_ready || !services_.monologue_submit) return;
    data_.world.apply_ready_perceptions(
        0, services_.perception_ready);
    data_.world.apply_ready_monologues(
        0, services_.monologue_ready);
    for (const auto& scene : data_.scenes) {
        if (!scene) continue;
        data_.world.submit_catchup_monologues(
            scene->scene_id, services_.monologue_submit);
    }
}

}  // namespace rhapsode
