#pragma once

#include <string>
#include <vector>

#include "rhapsode/scene_message.h"

namespace rhapsode {

class Scene;

std::string build_narrator_instructions();

std::string build_narrator_turn_state(
    const std::vector<SceneMessage>& history,
    const Scene& scene);

}  // namespace rhapsode
