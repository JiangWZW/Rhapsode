#include "rhapsode/turn_pipeline.h"

#include "rhapsode/log_util.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/str_util.h"
#include "rhapsode/weaver.h"
#include "rhapsode/world.h"
#include "turn_pipeline_internal.h"

#include <chrono>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <utility>

namespace rhapsode {
namespace {

SceneMessage make_turn_message(const std::string& kind,
                               std::string content,
                               const std::string& speaker = {}) {
    SceneMessage message;
    message.role = Role::Assistant;
    message.content = std::move(content);
    message.metadata = {{"scene_kind", kind}};
    if (!speaker.empty()) message.metadata["speaker"] = speaker;
    return message;
}

void set_turn_metadata(SceneMessage& message, const std::string& scene_id,
                       std::uint64_t commit_version, int turn, int ordinal) {
    message.metadata["turn"] = turn;
    message.metadata["turn_ordinal"] = ordinal;
    message.metadata["message_ref"] = scene_id + ":v" +
        std::to_string(commit_version) + ":" +
        message.metadata.value("scene_kind", std::string{"message"}) + ":" +
        std::to_string(ordinal);
}

void publish_outputs(TurnServices& services, TurnResult& result) noexcept {
    if (!services.turn_complete) return;
    for (const auto& message : result.outputs) {
        try {
            services.turn_complete(message);
        } catch (const std::exception& error) {
            result.delivery_failures.push_back(error.what());
            log_warn("turn") << "committed output delivery failed: "
                             << error.what() << "\n" << std::flush;
        }
    }
}

void append_input_message(SceneData& scene, const std::string& text,
                          bool autonomous,
                          std::uint64_t commit_version) {
    if (autonomous) {
        log_debug("turn") << "off-stage cue scene=" << scene.scene_id << "\n";
    }

    SceneMessage message;
    message.role = Role::User;
    message.content = text;
    message.metadata["scene_kind"] = autonomous ? "director_cue" : "player";
    set_turn_metadata(
        message, scene.scene_id, commit_version, scene.turn_index, 0);
    append_history_message(scene.history, std::move(message));
}

void emit_dialogue(SceneData& scene, int turn,
                   std::uint64_t commit_version,
                   const nlohmann::json& plan,
                   std::vector<SceneMessage>& outputs) {
    const auto speech = plan.find("speech_turns");
    if (speech == plan.end() || !speech->is_array()) return;
    log_debug("narrator") << "emit dialogue cues=" << speech->size() << "\n";
    int ordinal = 2;
    for (const auto& cue : *speech) {
        if (!cue.is_object()) continue;
        const std::string character = cue.value("character", "");
        if (character.empty()) continue;
        std::string spoken = str::trim(cue.value("line", ""));
        const std::string action = str::trim(cue.value("action", ""));
        if (!action.empty())
            spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");
        if (spoken.empty())
            spoken = "(" + character + " is at a loss for words.)";

        auto message = make_turn_message(
            "character", std::move(spoken), character);
        set_turn_metadata(message, scene.scene_id,
                          commit_version, turn, ordinal++);
        append_history_message(scene.dialogue, std::move(message));
        outputs.push_back(scene.dialogue.back());
    }
}

struct TurnRollback {
    StoryData& data;
    SceneData* live_scene;
    TurnServices& services;
    World world;
    WorldGraph observations;
    SceneData scene;
    std::string board;
    std::uint64_t version;
    bool armed = true;

    ~TurnRollback() {
        if (!armed) return;
        data.world = std::move(world);
        data.observations = std::move(observations);
        *live_scene = std::move(scene);
        data.transaction_version = version;
        services.storyline_board = std::move(board);
    }

    void disarm() { armed = false; }
};

}  // namespace

TurnResult execute_turn(
    StoryData& data, TurnServices& services, const TurnInput& input) {
    SceneData* live_scene = find_scene(data, input.scene_id);
    if (!live_scene)
        throw std::invalid_argument("Unknown scene: " + input.scene_id);

    const bool autonomous = input.kind == TurnInput::Kind::Autonomous;
    const std::uint64_t base_version = data.transaction_version;
    const std::uint64_t commit_version = base_version + 1;

    TurnRollback rollback{
        data, live_scene, services,
        data.world, data.observations, *live_scene,
        services.storyline_board, base_version};

    World candidate_world = rollback.world;
    SceneData candidate_scene = rollback.scene;
    ReadToolLease read_tools = make_frozen_story_read_tools(data, input.scene_id);
    services.storyline_board =
        format_live_storylines_board(summarize_story_scenes(data));

    TurnResult result;
    NarratorTurnResult narrator;
    const int turn = candidate_scene.turn_index;
    const auto started_at = std::chrono::steady_clock::now();

    append_input_message(
        candidate_scene, input.text, autonomous, commit_version);

    const std::string instructions = build_narrator_instructions();
    const std::string turn_state = build_narrator_turn_state(
        candidate_scene, candidate_world, services.storyline_board);
    services.storyline_board.clear();
    ++candidate_scene.turn_index;
    log_debug("narrator") << "prompt instructions=" << instructions.size()
                          << " turn_state=" << turn_state.size()
                          << " chars\n" << std::flush;

    narrator = run_narrator_with_retry(
        candidate_world, services, candidate_scene, turn,
        instructions, turn_state, read_tools.callback());
    apply_narrator_cast(candidate_world, candidate_scene, narrator);

    auto narrator_message = make_turn_message("narrator", narrator.prose);
    set_turn_metadata(
        narrator_message, input.scene_id, commit_version, turn, 1);
    append_history_message(candidate_scene.history, std::move(narrator_message));
    result.outputs.push_back(candidate_scene.history.back());
    emit_dialogue(candidate_scene, turn, commit_version, narrator.plan,
                  result.outputs);

    data.world = std::move(candidate_world);
    *live_scene = std::move(candidate_scene);
    data.transaction_version = commit_version;
    rollback.disarm();
    result.scene_id = input.scene_id;
    result.post_turn_index = turn;

    const double elapsed_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started_at).count();
    log_info("turn") << "end scene=" << input.scene_id << " id=" << turn
                     << " ms=" << static_cast<long long>(
                            std::lround(elapsed_ms)) << "\n";

    publish_outputs(services, result);

    try {
        World observation_world = data.world;
        WorldGraph next_observations = data.observations;
        GraphPlanResult output = extract_graph_observations(
            observation_world, next_observations, services, *live_scene,
            turn, narrator, read_tools.callback());
        data.world = std::move(observation_world);
        data.observations = std::move(next_observations);
        result.effects.created_nodes = std::move(output.new_nodes);
        result.effects.expired_nodes = std::move(output.newly_expired);
        if (services.weaver.active()) {
            std::vector<std::string> priority;
            for (const auto& node : result.effects.created_nodes) {
                priority.insert(
                    priority.end(), node.entities.begin(), node.entities.end());
            }
            services.weaver.rebuild_expiry_queue(data.observations, priority);
        }
    } catch (...) {
        result.effects = {};
        log_warn("turn") << "graph observation extraction failed\n" << std::flush;
    }
    return result;
}

}  // namespace rhapsode
