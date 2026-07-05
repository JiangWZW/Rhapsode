#pragma once

#include <string>
#include <vector>

#include "rhapsode/director.h"
#include "rhapsode/scene_message.h"

namespace rhapsode {

class MemorySystem;
class Scene;

std::string build_narrator_instructions();

std::string build_narrator_turn_state(
    const std::vector<SceneMessage>& history,
    const Scene& scene,
    const DirectorOutput& director_out,
    MemorySystem* memory,
    const std::string& world_graph_context,
    const std::string& inner_lives);

}  // namespace rhapsode
