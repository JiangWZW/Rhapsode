#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "rhapsode/llm_callback.h"
#include "rhapsode/node.h"
#include "rhapsode/read_tools.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

class MemorySystem;

struct GraphSettlement {
    std::string scene_id;
    int turn = -1;
    std::uint64_t commit_version = 0;
    std::string prose;
    nlohmann::json plan;
    ReadToolLease read_tools;
};

struct TurnResult {
    std::string scene_id;
    std::vector<SceneMessage> outputs;
    std::vector<std::string> delivery_failures;
    struct Effects {
        std::vector<Node> created_nodes;
        std::vector<Node> expired_nodes;
    } effects;
    std::optional<GraphSettlement> graph_settlement;
};

using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

struct TurnServices {
    LLMCallback llm;
    NarratorLLMCallback narrator;
    TurnCompleteCallback turn_complete;
    LLMCallback downsampler;
    LLMCallback reflection;
    LLMCallback perception;
    MindReadyFn perception_ready;
    std::function<void(const std::vector<PromptJob>&)> perception_submit;
    MindReadyFn monologue_ready;
    std::function<void(const std::vector<PromptJob>&)> monologue_submit;
    size_t history_window = 8;
    std::string storyline_board;
    Weaver weaver;
    std::shared_ptr<MemorySystem> memory;
    SchedulerCallback scheduler;
    LifecycleCallback lifecycle;
    std::string saves_dir;
};

struct TurnInput {
    enum class Kind { Player, Autonomous };

    Kind kind = Kind::Player;
    std::string scene_id;
    std::string text;
};

TurnResult execute_turn(
    StoryData& data, TurnServices& services, const TurnInput& input);

TurnResult::Effects settle_graph_observations(
    StoryData& data, TurnServices& services,
    GraphSettlement settlement) noexcept;

std::vector<Node> process_post_turn(
    StoryData& data, TurnServices& services, const std::string& scene_id) noexcept;

}  // namespace rhapsode
