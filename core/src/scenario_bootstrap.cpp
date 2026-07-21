#include "rhapsode/scenario_bootstrap.h"

#include <cstdint>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/character.h"
#include "rhapsode/character_memory.h"

namespace rhapsode {

World build_world_from_scenario(const nlohmann::json& scenario,
                                const std::string& root_scene_id) {
    std::vector<Character> characters;
    for (const auto& character_value :
         scenario.value("characters", nlohmann::json::array())) {
        Character character = character_value.get<Character>();
        const bool authored_on = character_value.value(
            "on_stage", character_value.value("is_player", false));
        if (authored_on && !root_scene_id.empty())
            character.join_scene(root_scene_id);
        characters.push_back(std::move(character));
    }

    nlohmann::json graph_value;
    graph_value["next_id"] = 1;
    graph_value["nodes"] =
        scenario.value("nodes", nlohmann::json::array());
    graph_value["edges"] =
        scenario.value("edges", nlohmann::json::array());

    nlohmann::json world_value;
    world_value["world_graph"] = std::move(graph_value);
    world_value["characters"] = std::move(characters);
    World world = World::from_json(world_value);

    for (const auto& character_value :
         scenario.value("characters", nlohmann::json::array())) {
        const std::string name = character_value.value("name", "");
        if (name.empty() || character_value.value("is_player", false))
            continue;
        if (!character_value.contains("initial_memory")) continue;

        CharacterMemory memory(name);
        const auto& initial_memory = character_value["initial_memory"];
        const auto& beliefs =
            initial_memory.value("beliefs", nlohmann::json::array());
        std::vector<std::uint64_t> seeded_ids(beliefs.size(), 0);
        for (std::size_t index = 0; index < beliefs.size(); ++index) {
            const auto& belief = beliefs[index];
            std::vector<std::string> subjects;
            if (belief.contains("about")) {
                const auto& about = belief["about"];
                if (about.is_string()) {
                    subjects.push_back(about.get<std::string>());
                } else if (about.is_array()) {
                    for (const auto& subject : about)
                        subjects.push_back(subject.get<std::string>());
                }
            }
            float weight = CharacterMemory::kAuthoredSeedWeight;
            if (belief.contains("weight"))
                weight = belief["weight"].get<float>();
            const std::string type = belief.value("intention", false)
                ? std::string("intention")
                : belief.value("type", std::string("belief"));
            seeded_ids[index] = memory.seed_belief(
                belief.value("content", ""), subjects, 0, weight, type);
        }

        for (std::size_t index = 0; index < beliefs.size(); ++index) {
            if (!beliefs[index].contains("tension_with")) continue;
            const auto& tension_with = beliefs[index]["tension_with"];
            const auto link = [&](long long other) {
                if (other >= 0 &&
                    static_cast<std::size_t>(other) < seeded_ids.size()) {
                    memory.link_tension(
                        seeded_ids[index],
                        seeded_ids[static_cast<std::size_t>(other)], 0);
                }
            };
            if (tension_with.is_number()) {
                link(tension_with.get<long long>());
            } else if (tension_with.is_array()) {
                for (const auto& other : tension_with) {
                    if (other.is_number()) link(other.get<long long>());
                }
            }
        }

        for (const auto& context :
             initial_memory.value("context", nlohmann::json::array())) {
            memory.seed_belief(context.get<std::string>(), {name}, 0);
        }
        world.set_character_memory(std::move(memory));
    }

    return world;
}

}  // namespace rhapsode
