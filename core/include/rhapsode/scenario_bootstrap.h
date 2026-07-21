#pragma once

#include <string>

#include <nlohmann/json_fwd.hpp>

#include "rhapsode/world.h"

namespace rhapsode {

World build_world_from_scenario(const nlohmann::json& scenario,
                                const std::string& root_scene_id = {});

}  // namespace rhapsode
