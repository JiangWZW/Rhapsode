#include "rhapsode/read_tools.h"

#include <atomic>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#include "rhapsode/scene_data.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/world.h"
#include "rhapsode/world_analysis.h"

namespace rhapsode {
namespace {

const SceneData* resolve_scene(const ReadToolContext& context,
                               const std::string& requested_scene) {
    if (requested_scene.empty() || requested_scene == context.scene_id)
        return context.scene;
    const auto found = context.scenes_by_id.find(requested_scene);
    return found == context.scenes_by_id.end() ? nullptr : found->second;
}

std::string unknown_scene_error(const ReadToolContext& context,
                                const std::string& requested_scene) {
    const std::string& scene_id = requested_scene.empty()
        ? context.scene_id : requested_scene;
    return nlohmann::json{{"error", "unknown scene: " + scene_id}}.dump();
}

}  // namespace

ReadToolContext make_read_tool_context(
    const World& world,
    const WorldGraph& observations,
    std::string scene_id,
    std::string scene_summaries_json,
    std::unordered_map<std::string, const SceneData*> scenes_by_id) {
    ReadToolContext context;
    context.world = &world;
    context.observations = &observations;
    context.scene_id = std::move(scene_id);
    context.scene_summaries_json = std::move(scene_summaries_json);
    context.scenes_by_id = std::move(scenes_by_id);
    const auto selected = context.scenes_by_id.find(context.scene_id);
    if (selected != context.scenes_by_id.end()) context.scene = selected->second;
    return context;
}

struct ReadToolLease::State {
    std::atomic_bool active{true};
};

ReadToolLease::ReadToolLease(ReadToolContext context,
                             std::shared_ptr<const void> keep_alive)
    : state_(std::make_shared<State>()) {
    callback_ = [context = std::move(context), state = state_,
                 keep_alive = std::move(keep_alive)](
                    const std::string& name,
                    const std::string& args_json) {
        (void)keep_alive;
        if (!state->active.load())
            throw std::runtime_error("Read tool callback is no longer active");
        return dispatch_read_tool(context, name, args_json);
    };
}

ReadToolLease::~ReadToolLease() { close(); }

ReadToolLease::ReadToolLease(ReadToolLease&& other) noexcept
    : state_(std::move(other.state_)), callback_(std::move(other.callback_)) {}

ReadToolLease& ReadToolLease::operator=(ReadToolLease&& other) noexcept {
    if (this == &other) return *this;
    close();
    state_ = std::move(other.state_);
    callback_ = std::move(other.callback_);
    return *this;
}

void ReadToolLease::close() noexcept {
    if (state_) state_->active.store(false);
}

std::string dispatch_read_tool(const ReadToolContext& context,
                               const std::string& name,
                               const std::string& args_json) {
    nlohmann::json args = nlohmann::json::parse(args_json, nullptr, false);
    if (!args.is_object()) args = nlohmann::json::object();
    const auto string_arg = [&](const char* key) {
        const auto it = args.find(key);
        return it != args.end() && it->is_string()
            ? it->get<std::string>() : std::string{};
    };

    if (name == "query_graph") {
        if (!context.observations)
            return nlohmann::json{{"error", "graph unavailable"}}.dump();
        return query_world_graph(*context.observations, string_arg("query"));
    }
    if (name == "query_mind") {
        if (!context.world)
            return nlohmann::json{{"error", "world unavailable"}}.dump();
        return query_character_mind(
            *context.world, string_arg("character"));
    }
    if (name == "query_character_core") {
        if (!context.world)
            return nlohmann::json{{"error", "world unavailable"}}.dump();
        return query_character_core(
            *context.world, string_arg("character"));
    }
    if (name == "query_history") {
        const std::string requested_scene = string_arg("scene_id");
        const SceneData* scene = resolve_scene(context, requested_scene);
        if (!scene) return unknown_scene_error(context, requested_scene);
        return query_scene_history(scene->history, string_arg("query"));
    }
    if (name == "query_transcript") {
        const std::string requested_scene = string_arg("scene_id");
        const SceneData* scene = resolve_scene(context, requested_scene);
        if (!scene) return unknown_scene_error(context, requested_scene);
        return query_attributed_transcript(*scene, string_arg("query"));
    }
    if (name == "list_scenes") return context.scene_summaries_json;
    return nlohmann::json{{"error", "unknown tool: " + name}}.dump();
}

}  // namespace rhapsode
