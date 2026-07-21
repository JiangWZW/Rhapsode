#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "rhapsode/scene_data.h"
#include "rhapsode/world.h"

namespace rhapsode {

struct ScenarioBootstrap {
    SceneData scene;
    World world;
};

ScenarioBootstrap load_scenario_file(const std::string& path);
ScenarioBootstrap bootstrap_scenario(const nlohmann::json& scenario,
                                     const std::string& scene_id = {});
nlohmann::json serialize_scenario(const SceneData& scene, const World& world);

}  // namespace rhapsode
