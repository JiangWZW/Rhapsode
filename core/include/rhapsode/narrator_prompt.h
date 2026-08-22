#pragma once

#include <string>

namespace rhapsode {

struct SceneData;
class World;

std::string build_narrator_instructions();

/// Phase B: graph ops only. Contains stable marker "GRAPH_UPDATE".
std::string build_narrator_graph_instructions();

std::string build_narrator_turn_state(
    const SceneData& scene,
    const World& world,
    const std::string& live_storylines_board = {});

}  // namespace rhapsode
