#include "rhapsode/world_analysis.h"

#include <algorithm>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "rhapsode/str_util.h"
#include "rhapsode/world.h"

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

std::string query_world_graph(const World& world, const std::string& query) {
    const std::string query_lower = str::to_lower(query);
    const auto nodes = world.graph().all_nodes(true);

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
    result["thoughts"] = memory->second.render_thoughts({});
    return result.dump();
}

std::vector<DeathCandidate> find_death_candidates(const World& world) {
    std::vector<DeathCandidate> candidates;
    const auto nodes = world.graph().all_nodes(true);

    static const std::vector<std::string> death_keywords = {
        "dead", "dies", "died", "kills", "killed", "killing",
        "corpse", "slain", "perished", "executed", "murdered",
        "succumbed", "fatal"};

    const auto has_death_keyword = [&](const std::string& text_lower) {
        for (const auto& keyword : death_keywords) {
            if (text_lower.find(keyword) != std::string::npos) return true;
        }
        return false;
    };

    const auto mentions_character = [&](const Node& node,
                                        const std::string& name,
                                        const std::string& name_lower) {
        for (const auto& entity : node.entities) {
            if (str::iequals(entity, name)) return true;
        }
        if (str::to_lower(node.fact).find(name_lower) != std::string::npos)
            return true;
        return str::to_lower(node.active_ctx).find(name_lower) !=
               std::string::npos;
    };

    for (const auto& character : world.characters()) {
        if (character.is_player || character.dead) continue;

        const std::string name_lower = str::to_lower(character.name);
        bool has_death_hit = false;
        for (const auto& node : nodes) {
            if (node.state != NodeState::Active) continue;
            if (!mentions_character(node, character.name, name_lower)) continue;
            if (has_death_keyword(str::to_lower(node.fact)) ||
                has_death_keyword(str::to_lower(node.active_ctx))) {
                has_death_hit = true;
            }
        }
        if (!has_death_hit) continue;

        DeathCandidate candidate;
        candidate.character_name = character.name;
        for (const auto& node : nodes) {
            if (node.state != NodeState::Active) continue;
            if (mentions_character(node, character.name, name_lower))
                candidate.evidence.push_back(node.fact);
        }
        candidates.push_back(std::move(candidate));
    }

    return candidates;
}

}  // namespace rhapsode
