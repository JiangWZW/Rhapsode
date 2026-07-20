#include "rhapsode/story.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "rhapsode/scene_loop.h"

namespace rhapsode {
namespace {

std::string manifest_path(const std::string& saves_dir) {
    return saves_dir + "/story.json";
}

}  // namespace

bool Story::has_save(const std::string& saves_dir) const {
    if (!world_->has_save(saves_dir)) return false;
    if (std::filesystem::exists(manifest_path(saves_dir))) return true;
    const Scene* root = get_scene(active_scene_id_);
    return root && root->has_ephemeral_save(saves_dir);
}

void Story::load_save(const std::string& saves_dir) {
    if (loop_) loop_->join_background();
    world_->load_save(saves_dir);

    const std::string mpath = manifest_path(saves_dir);
    if (!std::filesystem::exists(mpath)) {
        if (Scene* root = get_scene(active_scene_id_))
            root->load_ephemeral(saves_dir);
        return;
    }

    std::ifstream in(mpath);
    nlohmann::json j;
    in >> j;

    active_scene_id_ = j.value("active_scene_id", active_scene_id_);
    beat_clock_ = j.value("beat_clock", 0);

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

}  // namespace rhapsode
