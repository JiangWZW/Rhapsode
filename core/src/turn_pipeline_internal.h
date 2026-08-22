#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "rhapsode/graph_plan.h"
#include "rhapsode/turn_pipeline.h"

namespace rhapsode {

struct NarratorTurnResult {
    std::string prose;
    nlohmann::json plan;
};

NarratorTurnResult run_narrator_with_retry(
    World& world, TurnServices& services, SceneData& scene, int turn,
    const std::string& instructions, const std::string& turn_state,
    const ReadToolCallback& read_tool);

GraphPlanResult extract_graph_observations(
    World& world, WorldGraph& observations, TurnServices& services,
    const SceneData& scene, int turn,
    NarratorTurnResult& narrator, const ReadToolCallback& read_tool);

void apply_narrator_cast(
    World& world, SceneData& scene, const NarratorTurnResult& result);

}  // namespace rhapsode
