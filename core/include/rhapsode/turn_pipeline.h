#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rhapsode/llm_callback.h"
#include "rhapsode/node.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_message.h"
#include "rhapsode/story_data.h"
#include "rhapsode/storyline_policy.h"
#include "rhapsode/turn_contracts.h"
#include "rhapsode/weaver.h"

namespace rhapsode {

class MemorySystem;

struct TurnResult {
    std::string scene_id;
    int completed_turn = 0;
    std::uint64_t base_state_version = 0;
    std::uint64_t resulting_state_version = 0;
    // Pre-increment turn index; argument for process_post_turn.
    int post_turn_index = -1;
    std::vector<SceneMessage> outputs;
    // Post-commit notification errors. The turn remains committed when these
    // are populated; callers may retry delivery but must not replay the turn.
    std::vector<std::string> delivery_failures;
    // Non-authoritative typed view of the accepted legacy narrator plan.
    std::optional<LegacyTurnShadow> legacy_shadow;
    struct Effects {
        std::vector<Node> created_nodes;
        std::vector<Node> expired_nodes;
    } effects;
};

using TurnCompleteCallback = std::function<void(const SceneMessage& assistant_msg)>;

// Stateful or configured dependencies used by turn operations. Durable story
// data lives in StoryData; services retain no World or SceneData pointers.
struct TurnServices {
    bool running = false;
    LLMCallback llm;
    NarratorLLMCallback narrator;
    TurnCompleteCallback turn_complete;
    LLMCallback downsampler;
    LLMCallback reflection;
    size_t history_window = 8;
    size_t resume_history_window = 12;
    bool resuming = false;
    std::string storyline_board;
    int timed_turns = 0;
    double turn_ms_sum = 0.0;
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

// The one narrative-turn operation. It stages input, narration, dialogue, and
// World changes on copies, commits them together, then records observations.
TurnResult execute_turn(
    StoryData& data, TurnServices& services, const TurnInput& input);

std::vector<Node> process_post_turn(
    StoryData& data, TurnServices& services, const std::string& scene_id,
    int turn) noexcept;
std::string synthesize_merge_context(
    World& world, TurnServices& services,
    const SceneData& source, const SceneData& target,
    ReadToolCallback read_tool = {});
std::string synthesize_fork_context(
    World& world, TurnServices& services,
    const SceneData& parent, const std::vector<std::string>& cast,
    const std::string& driving_intention,
    ReadToolCallback read_tool = {});

}  // namespace rhapsode
