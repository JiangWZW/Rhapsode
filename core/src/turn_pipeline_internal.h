#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/graph_plan.h"
#include "rhapsode/turn_pipeline.h"

namespace rhapsode {

enum class OutputBucket { Narration, Dialogue };

struct NarratorPrompt {
    std::string instructions;
    std::string turn_state;
};

struct SpeechCue {
    std::string character;
    nlohmann::json direction;

    std::string field(const char* key) const {
        return direction.value(key, "");
    }
};

struct NarratorTurnResult {
    std::string prose;
    nlohmann::json plan;
    std::vector<SpeechCue> cues;
};

NarratorPrompt build_turn_prompt(
    World& world, TurnServices& services, SceneData& scene);

NarratorTurnResult run_narrator_with_retry(
    World& world, TurnServices& services, SceneData& scene, int turn,
    const NarratorPrompt& prompt,
    const ReadToolCallback& read_tool);

GraphPlanResult extract_graph_observations(
    World& world, WorldGraph& observations, TurnServices& services,
    const SceneData& scene, int turn, const NarratorPrompt& prompt,
    NarratorTurnResult& narrator, const ReadToolCallback& read_tool);

void apply_narrator_cast(
    World& world, SceneData& scene, const NarratorTurnResult& result);

LegacyTurnShadow adapt_legacy_shadow(
    const World& world, const SceneData& scene, int turn,
    const NarratorTurnResult& result,
    const std::vector<SceneMessage>& outputs,
    std::uint64_t base_state_version,
    std::uint64_t resulting_state_version);

}  // namespace rhapsode
