#include "rhapsode/story.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/turn_executor.h"
#include "rhapsode/str_util.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

Story::Story()
    : world_(std::make_unique<World>()),
      director_(std::make_unique<Director>(world_->graph())),
      weaver_(std::make_unique<Weaver>(world_->graph())),
      executor_(std::make_unique<TurnExecutor>(*world_, *director_, *weaver_)) {}

Story::~Story() = default;
Story::Story(Story&&) noexcept = default;
Story& Story::operator=(Story&&) noexcept = default;

Story Story::load_scenario(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) throw std::runtime_error("Cannot open scenario file: " + path);
    nlohmann::json scenario;
    input >> scenario;
    return from_scenario_json(
        scenario, std::filesystem::path(path).stem().string());
}

Story Story::from_scenario_json(const nlohmann::json& scenario,
                                const std::string& scene_id) {
    Story story;
    SceneData root;
    root.scene_id = scene_id;
    root.title = scenario.value("title", std::string{});
    root.system_prompt = scenario.value("system_prompt", std::string{});
    if (scenario.contains("history"))
        root.history = scenario.at("history").get<History>();
    for (const auto& message : scenario.value("seed_messages", nlohmann::json::array()))
        root.history.append(message.get<SceneMessage>());

    story.world_->seed_from_scenario(scenario, scene_id);
    story.active_scene_id_ = scene_id;
    story.adopt(std::move(root));
    return story;
}

Story Story::from_data(SceneData root, World world) {
    Story story;
    *story.world_ = std::move(world);
    story.active_scene_id_ = root.scene_id;
    story.adopt(std::move(root));
    return story;
}

nlohmann::json Story::to_scenario_json(const std::string& scene_id) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) throw std::runtime_error("Unknown scene: " + scene_id);
    nlohmann::json result;
    result["title"] = scene->title;
    result["system_prompt"] = scene->system_prompt;
    result["characters"] = world_->characters();
    result["history"] = scene->history;
    return result;
}

SceneData* Story::adopt(SceneData scene) {
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
    SceneData* adopted = adopt(std::move(child));

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

std::string Story::derive_intention(const SceneData& scene, float* charge_out) const {
    std::string best = scene.driving_intention;
    float best_weight = scene.charge;
    for (const auto& character : world_->characters()) {
        if (!character.in_scene(scene.scene_id)) continue;
        const auto it = world_->character_memories().find(character.name);
        if (it == world_->character_memories().end()) continue;
        it->second.beliefs().for_each([&](const Node& node) {
            if (node.type == "intention" && node.state == NodeState::Active &&
                node.weight > best_weight) {
                best_weight = node.weight;
                best = node.fact;
            }
        }, false);
    }
    if (charge_out) *charge_out = best_weight;
    return best;
}

std::string Story::tool_list_scenes() const {
    nlohmann::json rows = nlohmann::json::array();
    for (const auto& scene : scenes_) {
        nlohmann::json row;
        row["scene_id"] = scene->scene_id;
        row["title"] = scene->title;
        row["active"] = scene->scene_id == active_scene_id_;
        row["turn_index"] = scene->turn_index;
        row["staleness"] = beat_clock_ - scene->last_advanced;

        nlohmann::json cast = nlohmann::json::array();
        bool player_present = false;
        for (const auto& character : world_->characters()) {
            if (!character.in_scene(scene->scene_id)) continue;
            cast.push_back(character.name);
            if (character.is_player) player_present = true;
        }
        row["cast"] = std::move(cast);
        row["player_present"] = player_present;

        float charge = 0.0f;
        row["driving_intention"] = derive_intention(*scene, &charge);
        row["charge"] = charge;

        std::string last;
        const auto& messages = scene->history.messages();
        for (auto it = messages.rbegin(); it != messages.rend(); ++it) {
            if (it->role != Role::Assistant) continue;
            last = it->content.size() > 240 ? it->content.substr(0, 240) + "..."
                                            : it->content;
            break;
        }
        row["last_narration"] = last;
        rows.push_back(std::move(row));
    }
    return rows.dump();
}

std::string Story::query_history(const SceneData& scene,
                                 const std::string& query) const {
    std::vector<std::string> keywords;
    std::string current;
    for (const char character : str::to_lower(query)) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            if (!current.empty()) {
                keywords.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += character;
        }
    }
    if (!current.empty()) keywords.push_back(std::move(current));

    std::vector<const SceneMessage*> matches;
    for (const auto& message : scene.history.messages()) {
        const std::string text = str::to_lower(message.content);
        if (std::any_of(keywords.begin(), keywords.end(),
                        [&](const auto& keyword) {
                            return text.find(keyword) != std::string::npos;
                        }))
            matches.push_back(&message);
    }
    std::reverse(matches.begin(), matches.end());
    if (matches.size() > 10) matches.resize(10);

    auto role_name = [](Role role) {
        switch (role) {
            case Role::User: return "user";
            case Role::Assistant: return "assistant";
            case Role::System: return "system";
        }
        return "user";
    };
    nlohmann::json snippets = nlohmann::json::array();
    for (const auto* message : matches)
        snippets.push_back({{"role", role_name(message->role)},
                            {"text", truncate_utf8(message->content, 400)}});
    return nlohmann::json{{"snippets", std::move(snippets)}}.dump();
}

std::string Story::dispatch_tool(const std::string& scene_id,
                                 const std::string& name,
                                 const std::string& args_json) {
    nlohmann::json args = nlohmann::json::parse(args_json, nullptr, false);
    if (!args.is_object()) args = nlohmann::json::object();
    auto string_arg = [&](const char* key) {
        const auto it = args.find(key);
        return it != args.end() && it->is_string()
            ? it->get<std::string>() : std::string{};
    };
    if (name == "query_graph") return world_->tool_query_graph(string_arg("query"));
    if (name == "query_mind") return world_->tool_query_mind(string_arg("character"));
    if (name == "query_history") {
        const SceneData* scene = get_scene(scene_id);
        if (!scene) return nlohmann::json{{"error", "unknown scene: " + scene_id}}.dump();
        return query_history(*scene, string_arg("query"));
    }
    if (name == "list_scenes") return tool_list_scenes();
    return nlohmann::json{{"error", "unknown tool: " + name}}.dump();
}

std::vector<SceneMessage> Story::display_timeline(
    const std::string& scene_id,
    std::optional<size_t> cap) const {
    const SceneData* scene = get_scene(scene_id);
    if (!scene) return {};
    std::vector<SceneMessage> merged;
    merged.reserve(scene->history.size() + scene->dialogue.size());
    for (const auto& message : scene->history.messages()) merged.push_back(message);
    for (const auto& message : scene->dialogue.messages()) merged.push_back(message);
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

    const auto& messages = scene.history.messages();
    int user_count = 0;
    size_t cut = messages.size();
    for (size_t i = messages.size(); i > 0; --i) {
        if (messages[i - 1].role == Role::User && ++user_count >= actual) {
            cut = i - 1;
            break;
        }
    }
    scene.history.truncate(cut);
    scene.dialogue.drop_from_turn(target);
    world_->revert_to_turn(target);
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
void Story::set_history_window(size_t normal, size_t resume) {
    executor_->set_history_window(normal, resume);
}
void Story::set_resuming(bool value) { executor_->set_resuming(value); }
void Story::set_downsampler_callback(LLMCallback cb) {
    executor_->set_downsampler_callback(std::move(cb));
}

}  // namespace rhapsode
