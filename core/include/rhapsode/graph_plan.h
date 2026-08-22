#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/world_graph.h"

namespace rhapsode {

struct Rejection {
    std::string fact;
    std::string reason;
};

struct GraphPlanResult {
    std::vector<std::string> context_blocks;
    std::vector<Node> newly_expired;
    std::vector<Node> new_nodes;
    std::vector<Rejection> rejections;
};

/// Applies observation-graph transitions and additions from parsed JSON.
/// The function retains no graph reference and performs no model call.
GraphPlanResult apply_graph_plan(
    WorldGraph& graph, int turn_index, const nlohmann::json& response);

}  // namespace rhapsode
