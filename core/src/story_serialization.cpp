#include "rhapsode/story.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unordered_set>

#include "rhapsode/memory_system.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/text_downsampling.h"

namespace rhapsode {
namespace {

std::string manifest_path(const std::string& saves_dir) {
    return saves_dir + "/story.json";
}

std::string world_path(const std::string& saves_dir) {
    return saves_dir + "/world.json";
}

std::string scene_path(const std::string& saves_dir,
                       const std::string& scene_id) {
    return saves_dir + "/" + scene_id + ".json";
}

void migrate_character_lines_to_dialogue(std::vector<SceneMessage>& history,
                                         std::vector<SceneMessage>& dialogue) {
    std::vector<SceneMessage> kept;
    kept.reserve(history.size());
    for (const auto& message : history) {
        if (message.metadata.value("scene_kind", std::string{}) == "character")
            append_history_message(dialogue, message);
        else
            kept.push_back(message);
    }
    history.clear();
    for (auto& message : kept)
        append_history_message(history, std::move(message));
}

bool has_scene_save(const std::string& saves_dir, const std::string& scene_id) {
    return std::filesystem::exists(scene_path(saves_dir, scene_id));
}

std::vector<std::string> manifest_scene_ids(const std::string& saves_dir) {
    std::ifstream input(manifest_path(saves_dir));
    if (!input.is_open()) return {};
    try {
        nlohmann::json manifest;
        input >> manifest;
        std::vector<std::string> ids;
        for (const auto& value : manifest.value(
                 "scene_ids", nlohmann::json::array())) {
            if (value.is_string()) ids.push_back(value.get<std::string>());
        }
        return ids;
    } catch (const std::exception&) {
        return {};
    }
}

nlohmann::json scene_closure_to_json(const SceneClosure& closure) {
    return {
        {"scene_id", closure.scene_id},
        {"reason", closure.reason},
        {"cast", closure.cast},
        {"driving_intention", closure.driving_intention},
        {"story_so_far", closure.story_so_far},
        {"final_narration", closure.final_narration},
        {"concluded_at", closure.concluded_at},
    };
}

SceneClosure scene_closure_from_json(const nlohmann::json& value) {
    SceneClosure closure;
    closure.scene_id = value.value("scene_id", std::string{});
    closure.reason = value.value("reason", std::string{});
    closure.cast = value.value("cast", std::vector<std::string>{});
    closure.driving_intention =
        value.value("driving_intention", std::string{});
    closure.story_so_far = value.value("story_so_far", std::string{});
    closure.final_narration =
        value.value("final_narration", std::string{});
    closure.concluded_at = value.value("concluded_at", 0);
    return closure;
}

void load_scene_data(SceneData& scene, const std::string& saves_dir) {
    std::ifstream input(scene_path(saves_dir, scene.scene_id));
    if (!input.is_open()) throw std::runtime_error("No save for scene: " + scene.scene_id);
    nlohmann::json value;
    input >> value;

    scene.turn_index = value.value("turn_index", 0);
    scene.driving_intention = value.value("driving_intention", std::string{});
    scene.charge = value.value("charge", 0.0f);
    scene.last_advanced = value.value("last_advanced", 0);
    scene.intention_owner = value.value("intention_owner", std::string{});
    scene.intention_node_id = value.value("intention_node_id", std::uint64_t{0});
    if (scene.title.empty()) scene.title = value.value("title", std::string{});
    if (scene.system_prompt.empty())
        scene.system_prompt = value.value("system_prompt", std::string{});
    scene.history = history_from_json(value.at("history"));
    if (value.contains("dialogue"))
        scene.dialogue = history_from_json(value.at("dialogue"));
    else
        migrate_character_lines_to_dialogue(scene.history, scene.dialogue);
    if (value.contains("downsampler"))
        scene.downsampling = downsampling_from_json(value.at("downsampler"));
}

void save_scene_data(const SceneData& scene, const std::string& saves_dir) {
    nlohmann::json value;
    value["scene_id"] = scene.scene_id;
    value["title"] = scene.title;
    value["system_prompt"] = scene.system_prompt;
    value["turn_index"] = scene.turn_index;
    value["driving_intention"] = scene.driving_intention;
    value["charge"] = scene.charge;
    value["last_advanced"] = scene.last_advanced;
    value["intention_owner"] = scene.intention_owner;
    value["intention_node_id"] = scene.intention_node_id;
    value["history"] = scene.history;
    value["dialogue"] = scene.dialogue;
    value["downsampler"] = downsampling_to_json(scene.downsampling);

    std::ofstream output(scene_path(saves_dir, scene.scene_id));
    if (!output.is_open()) throw std::runtime_error("Cannot write save for: " + scene.scene_id);
    output << value.dump(2);
}

}  // namespace

bool Story::has_save(const std::string& saves_dir) const {
    if (!std::filesystem::exists(world_path(saves_dir))) return false;
    if (std::filesystem::exists(manifest_path(saves_dir))) return true;
    const SceneData* root = get_scene(data_.active_scene_id);
    return root && has_scene_save(saves_dir, root->scene_id);
}

void Story::load_save(const std::string& saves_dir) {
    pending_turn_.reset();
    std::ifstream world_input(world_path(saves_dir));
    if (!world_input.is_open())
        throw std::runtime_error("No world save in: " + saves_dir);
    nlohmann::json world_value;
    world_input >> world_value;
    if (!world_value.contains("world_graph") &&
        !world_value.contains("node_pool")) {
        throw std::runtime_error(
            "World save is missing world_graph/node_pool data");
    }
    import_world(data_, World::from_json(world_value));
    if (services_.memory)
        services_.memory->set_next_id(world_value.value("memory_next_id", 0));

    const std::string path = manifest_path(saves_dir);
    if (!std::filesystem::exists(path)) {
        if (SceneData* root = active_scene()) load_scene_data(*root, saves_dir);
        return;
    }

    std::ifstream input(path);
    nlohmann::json manifest;
    input >> manifest;
    data_.active_scene_id = manifest.value("active_scene_id", data_.active_scene_id);
    data_.turn_clock = manifest.value(
        "turn_clock", manifest.value("beat_clock", 0));
    data_.scene_closures.clear();
    for (const auto& value : manifest.value(
             "scene_closures", nlohmann::json::array())) {
        if (value.is_object())
            data_.scene_closures.push_back(scene_closure_from_json(value));
    }

    std::vector<std::unique_ptr<SceneData>> rebuilt;
    for (const auto& id_value : manifest.value("scene_ids", nlohmann::json::array())) {
        const std::string id = id_value.get<std::string>();
        std::unique_ptr<SceneData> scene;
        if (SceneData* existing = get_scene(id))
            scene = std::make_unique<SceneData>(std::move(*existing));
        else {
            scene = std::make_unique<SceneData>();
            scene->scene_id = id;
        }
        load_scene_data(*scene, saves_dir);
        rebuilt.push_back(std::move(scene));
    }
    if (!rebuilt.empty()) data_.scenes = std::move(rebuilt);
}

void Story::save(const std::string& saves_dir) const {
    const std::vector<std::string> previously_saved =
        manifest_scene_ids(saves_dir);
    std::filesystem::create_directories(saves_dir);
    std::ofstream world_output(world_path(saves_dir));
    if (!world_output.is_open())
        throw std::runtime_error("Cannot write world save in: " + saves_dir);
    auto world_value = snapshot_world(data_).to_json();
    world_value["memory_next_id"] = services_.memory
        ? services_.memory->get_next_id() : 0;
    world_output << world_value.dump(2);

    for (const auto& scene : data_.scenes) save_scene_data(*scene, saves_dir);

    nlohmann::json manifest;
    manifest["active_scene_id"] = data_.active_scene_id;
    manifest["beat_clock"] = data_.turn_clock;  // Legacy save compatibility.
    manifest["turn_clock"] = data_.turn_clock;
    manifest["scene_ids"] = scene_ids();
    manifest["scene_closures"] = nlohmann::json::array();
    for (const auto& closure : data_.scene_closures)
        manifest["scene_closures"].push_back(scene_closure_to_json(closure));
    std::ofstream output(manifest_path(saves_dir));
    if (!output.is_open())
        throw std::runtime_error("Cannot write story manifest in: " + saves_dir);
    output << manifest.dump(2);
    output.close();

    std::unordered_set<std::string> live;
    for (const auto& id : scene_ids()) live.insert(id);
    for (const auto& id : previously_saved) {
        if (live.count(id) == 0)
            std::filesystem::remove(scene_path(saves_dir, id));
    }
}

void Story::delete_save(const std::string& saves_dir) const {
    std::unordered_set<std::string> scene_files;
    for (const auto& id : manifest_scene_ids(saves_dir))
        scene_files.insert(id);
    for (const auto& id : scene_ids()) scene_files.insert(id);

    std::filesystem::remove(world_path(saves_dir));
    std::filesystem::remove(manifest_path(saves_dir));
    for (const auto& id : scene_files)
        std::filesystem::remove(scene_path(saves_dir, id));
}

}  // namespace rhapsode
