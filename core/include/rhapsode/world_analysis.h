#pragma once

#include <string>
#include <vector>

namespace rhapsode {

class World;

struct DeathCandidate {
    std::string character_name;
    std::vector<std::string> evidence;
};

std::string query_world_graph(const World& world, const std::string& query);
std::string query_character_mind(const World& world,
                                 const std::string& character);
std::vector<DeathCandidate> find_death_candidates(const World& world);

}  // namespace rhapsode
