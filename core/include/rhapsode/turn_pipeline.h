#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
#include "rhapsode/node.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

class MemorySystem;

struct TurnResult {
    std::string scene_id;
    int post_turn_index = -1;
    std::vector<SceneMessage> outputs;
    std::vector<std::string> delivery_failures;
    struct Effects {
        std::vector<Node> created_nodes;
        std::vector<Node> expired_nodes;
    } effects;
};

using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

struct TurnServices {
    LLMCallback llm;
    NarratorLLMCallback narrator;
    TurnCompleteCallback turn_complete;
    LLMCallback downsampler;
    LLMCallback reflection;
    LLMCallback observation;
    std::function<bool(std::size_t, int, std::string&, bool&)> observation_ready;
    std::function<void(const std::vector<PromptJob>&)> observation_submit;
    std::function<bool(std::size_t, int, std::string&, bool&)> monologue_ready;
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

std::vector<Node> process_post_turn(
    StoryData& data, TurnServices& services, const std::string& scene_id,
    int turn) noexcept;

}  // namespace rhapsode
