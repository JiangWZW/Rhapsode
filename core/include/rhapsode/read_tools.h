#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "rhapsode/scene_message.h"

namespace rhapsode {

class World;

struct ReadToolContext {
    const World* world = nullptr;
    const std::vector<SceneMessage>* history = nullptr;
    std::string scene_id;
    std::string scene_summaries_json;
    std::unordered_map<std::string, const std::vector<SceneMessage>*>
        histories_by_scene;
};

std::string dispatch_read_tool(const ReadToolContext& context,
                               const std::string& name,
                               const std::string& args_json);

}  // namespace rhapsode
