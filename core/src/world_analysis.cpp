#include "rhapsode/world_analysis.h"

#include <algorithm>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "rhapsode/str_util.h"
#include "rhapsode/world.h"
#include "rhapsode/world_graph.h"

namespace rhapsode {
namespace {

nlohmann::json node_to_json(const Node& node) {
    nlohmann::json value;
    value["id"] = node.id;
    value["fact"] = node.fact;
    value["state"] = to_string(node.state);
    value["type"] = node.type;
    value["valid_until"] = node.valid_until;
    value["weight"] = node.weight;
    value["created_at"] = node.created_at;
    value["entities"] = node.entities;
    value["chain_to"] = node.related_to;
    return value;
}

std::unordered_set<std::string> find_matching_entities(
    const std::vector<Node>& nodes, const std::string& query) {
    std::unordered_set<std::string> matched;
    for (const auto& node : nodes) {
        for (const auto& entity : node.entities) {
            if (str::iequals(entity, query))
                matched.insert(str::to_lower(entity));
        }
    }
    return matched;
}

std::vector<const Node*> collect_entity_timeline(
    const std::vector<Node>& nodes, const std::string& entity_lower,
    std::size_t max_count = 20) {
    std::vector<const Node*> timeline;
    for (const auto& node : nodes) {
        for (const auto& entity : node.entities) {
            if (str::to_lower(entity) == entity_lower) {
                timeline.push_back(&node);
                break;
            }
        }
    }
    std::sort(timeline.begin(), timeline.end(),
              [](const Node* left, const Node* right) {
                  return left->created_at < right->created_at;
              });
    if (timeline.size() > max_count) timeline.resize(max_count);
    return timeline;
}

std::vector<const Node*> find_fact_substring_matches(
    const std::vector<Node>& nodes, const std::string& query_lower,
    std::size_t max_count = 15) {
    std::vector<const Node*> matches;
    for (const auto& node : nodes) {
        if (str::to_lower(node.fact).find(query_lower) != std::string::npos)
            matches.push_back(&node);
    }
    std::sort(matches.begin(), matches.end(),
              [](const Node* left, const Node* right) {
                  return left->created_at > right->created_at;
              });
    if (matches.size() > max_count) matches.resize(max_count);
    return matches;
}

}  // namespace

std::string query_world_graph(
    const WorldGraph& graph, const std::string& query) {
    const std::string query_lower = str::to_lower(query);
    const auto nodes = graph.all_nodes(true);

    const auto matched_entities = find_matching_entities(nodes, query);
    if (!matched_entities.empty()) {
        nlohmann::json chains = nlohmann::json::array();
        for (const auto& entity_lower : matched_entities) {
            const auto timeline = collect_entity_timeline(nodes, entity_lower);

            nlohmann::json chain;
            std::string entity_name = query;
            for (const auto* node : timeline) {
                for (const auto& entity : node->entities) {
                    if (str::to_lower(entity) == entity_lower) {
                        entity_name = entity;
                        break;
                    }
                }
            }
            chain["entity"] = entity_name;
            nlohmann::json entries = nlohmann::json::array();
            for (const auto* node : timeline)
                entries.push_back(node_to_json(*node));
            chain["timeline"] = std::move(entries);
            chains.push_back(std::move(chain));
        }

        nlohmann::json result;
        result["chains"] = std::move(chains);
        result["matches"] = nlohmann::json::array();
        return result.dump();
    }

    const auto matches = find_fact_substring_matches(nodes, query_lower);
    nlohmann::json entries = nlohmann::json::array();
    for (const auto* node : matches)
        entries.push_back(node_to_json(*node));

    nlohmann::json result;
    result["chains"] = nlohmann::json::array();
    result["matches"] = std::move(entries);
    return result.dump();
}

std::string query_character_mind(const World& world,
                                 const std::string& character) {
    const Character* found_character = world.find_character(character);
    const auto& memories = world.character_memories();

    auto memory = memories.end();
    if (found_character) memory = memories.find(found_character->name);
    if (memory == memories.end()) {
        for (auto it = memories.begin(); it != memories.end(); ++it) {
            if (str::iequals(it->first, character)) {
                memory = it;
                break;
            }
        }
    }

    if (memory == memories.end()) {
        nlohmann::json error;
        error["error"] = "character not found";
        return error.dump();
    }

    nlohmann::json result;
    result["character"] = memory->first;
    if (found_character)
        result["voice"] = found_character->build_prompt__dialogue_voice();
    const auto mind = memory->second.render_mind_query();
    result["core"] = mind.value("core", "");
        result["monologue"] = mind.value("monologue", nlohmann::json::array());
    result["beliefs"] = mind.value("beliefs", "");
    // Backward-compatible alias for older tooling.
    result["thoughts"] = result["beliefs"];
    return result.dump();
}

}  // namespace rhapsode
