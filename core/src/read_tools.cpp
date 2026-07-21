#include "rhapsode/read_tools.h"

#include <nlohmann/json.hpp>

#include "rhapsode/scene_history.h"
#include "rhapsode/world.h"
#include "rhapsode/world_analysis.h"

namespace rhapsode {

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
        if (!context.world)
            return nlohmann::json{{"error", "world unavailable"}}.dump();
        return query_world_graph(*context.world, string_arg("query"));
    }
    if (name == "query_mind") {
        if (!context.world)
            return nlohmann::json{{"error", "world unavailable"}}.dump();
        return query_character_mind(
            *context.world, string_arg("character"));
    }
    if (name == "query_history") {
        if (!context.history) {
            return nlohmann::json{
                {"error", "unknown scene: " + context.scene_id}}.dump();
        }
        return query_scene_history(*context.history, string_arg("query"));
    }
    if (name == "list_scenes") return context.scene_summaries_json;
    return nlohmann::json{{"error", "unknown tool: " + name}}.dump();
}

}  // namespace rhapsode
