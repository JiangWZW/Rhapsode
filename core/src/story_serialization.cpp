#include "rhapsode/story.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace rhapsode {
namespace {

std::string manifest_path(const std::string& saves_dir) {
    return saves_dir + "/story.json";
}

std::string scene_path(const std::string& saves_dir,
                       const std::string& scene_id) {
    return saves_dir + "/" + scene_id + ".json";
}

void migrate_character_lines_to_dialogue(History& history, History& dialogue) {
    std::vector<SceneMessage> kept;
    kept.reserve(history.size());
    for (const auto& message : history.messages()) {
        if (message.metadata.value("scene_kind", std::string{}) == "character")
            dialogue.append(message);
        else
            kept.push_back(message);
    }
    history.clear();
    for (auto& message : kept) history.append(std::move(message));
}

bool has_scene_save(const std::string& saves_dir, const std::string& scene_id) {
    return std::filesystem::exists(scene_path(saves_dir, scene_id));
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
    if (scene.title.empty()) scene.title = value.value("title", std::string{});
    if (scene.system_prompt.empty())
        scene.system_prompt = value.value("system_prompt", std::string{});
    scene.history = value.at("history").get<History>();
    if (value.contains("dialogue"))
        scene.dialogue = value.at("dialogue").get<History>();
    else
        migrate_character_lines_to_dialogue(scene.history, scene.dialogue);
    if (value.contains("downsampler"))
        scene.downsampler = TextDownsampler::from_json(value.at("downsampler"));
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
    value["history"] = scene.history;
    value["dialogue"] = scene.dialogue;
    value["downsampler"] = scene.downsampler.to_json();

    std::ofstream output(scene_path(saves_dir, scene.scene_id));
    if (!output.is_open()) throw std::runtime_error("Cannot write save for: " + scene.scene_id);
    output << value.dump(2);
}

}  // namespace

bool Story::has_save(const std::string& saves_dir) const {
    if (!world_->has_save(saves_dir)) return false;
    if (std::filesystem::exists(manifest_path(saves_dir))) return true;
    const SceneData* root = get_scene(active_scene_id_);
    return root && has_scene_save(saves_dir, root->scene_id);
}

void Story::load_save(const std::string& saves_dir) {
    world_->load_save(saves_dir);
    const std::string path = manifest_path(saves_dir);
    if (!std::filesystem::exists(path)) {
        if (SceneData* root = active_scene()) load_scene_data(*root, saves_dir);
        return;
    }

    std::ifstream input(path);
    nlohmann::json manifest;
    input >> manifest;
    active_scene_id_ = manifest.value("active_scene_id", active_scene_id_);
    beat_clock_ = manifest.value("beat_clock", 0);

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
    if (!rebuilt.empty()) scenes_ = std::move(rebuilt);
}

void Story::save(const std::string& saves_dir) const {
    std::filesystem::create_directories(saves_dir);
    world_->save(saves_dir);
    for (const auto& scene : scenes_) save_scene_data(*scene, saves_dir);

    nlohmann::json manifest;
    manifest["active_scene_id"] = active_scene_id_;
    manifest["beat_clock"] = beat_clock_;
    manifest["scene_ids"] = scene_ids();
    std::ofstream output(manifest_path(saves_dir));
    if (!output.is_open())
        throw std::runtime_error("Cannot write story manifest in: " + saves_dir);
    output << manifest.dump(2);
}

void Story::delete_save(const std::string& saves_dir) const {
    world_->delete_save(saves_dir);
    std::filesystem::remove(manifest_path(saves_dir));
    for (const auto& scene : scenes_)
        std::filesystem::remove(scene_path(saves_dir, scene->scene_id));
}

}  // namespace rhapsode
