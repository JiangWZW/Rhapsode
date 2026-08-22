#pragma once

#include <string>

namespace rhapsode {

class World;
class WorldGraph;

std::string query_world_graph(
    const WorldGraph& graph, const std::string& query);
std::string query_character_mind(const World& world,
                                 const std::string& character);

}  // namespace rhapsode
