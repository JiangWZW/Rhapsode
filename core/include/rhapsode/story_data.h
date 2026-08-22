#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rhapsode/scene_data.h"
#include "rhapsode/world.h"

namespace rhapsode {

struct SceneClosure {
    std::string scene_id;
    std::string reason;
    std::vector<std::string> cast;
    std::string driving_intention;
    std::string story_so_far;
    std::string final_narration;
    int concluded_at = 0;
    /// Empty for a concluded storyline. Set when a fork was merged away.
    std::string merged_into;
};

// Durable data for one playthrough. Scene allocations stay stable because the
// public C++ API historically returns SceneData pointers.
struct StoryData {
    World world;
    WorldGraph observations;
    std::vector<std::unique_ptr<SceneData>> scenes;
    std::vector<SceneClosure> scene_closures;
    std::string active_scene_id;
    int turn_clock = 0;
    std::uint64_t transaction_version = 0;
};

}  // namespace rhapsode
