#include "rhapsode/turn_pipeline.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/story_data_ops.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/weaver.h"
#include "rhapsode/world.h"
#include "turn_pipeline_internal.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
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

class TurnRunGuard {
public:
    explicit TurnRunGuard(bool& running) : running_(running) {
        if (running_)
            throw std::runtime_error("Cannot run turn: loop is already active");
        running_ = true;
    }

    ~TurnRunGuard() { running_ = false; }

    TurnRunGuard(const TurnRunGuard&) = delete;
    TurnRunGuard& operator=(const TurnRunGuard&) = delete;

private:
    bool& running_;
};

nlohmann::json scene_continuity_context(const SceneData& scene,
                                        const World& world) {
    nlohmann::json context;
    context["scene_id"] = scene.scene_id;
    context["title"] = scene.title;
    context["driving_intention"] = scene.driving_intention;
    context["story_so_far"] = render_text_downsampling(scene.downsampling);

    nlohmann::json cast = nlohmann::json::array();
    for (const auto& character : world.characters()) {
        if (!character.dead && character.in_scene(scene.scene_id))
            cast.push_back(character.name);
    }
    context["cast"] = std::move(cast);

    std::vector<const SceneMessage*> timeline;
    timeline.reserve(scene.history.size() + scene.dialogue.size());
    for (const auto& message : scene.history) timeline.push_back(&message);
    for (const auto& message : scene.dialogue) timeline.push_back(&message);
    std::stable_sort(timeline.begin(), timeline.end(),
        [](const SceneMessage* left, const SceneMessage* right) {
            return left->timestamp < right->timestamp;
        });
    constexpr std::size_t kRecentMessages = 8;
    const std::size_t start = timeline.size() > kRecentMessages
        ? timeline.size() - kRecentMessages : 0;
    nlohmann::json recent = nlohmann::json::array();
    for (std::size_t index = start; index < timeline.size(); ++index) {
        const SceneMessage& message = *timeline[index];
        nlohmann::json row{
            {"role", message.role},
            {"content", truncate_utf8(message.content, 600)},
        };
        if (message.metadata.contains("speaker"))
            row["speaker"] = message.metadata["speaker"];
        recent.push_back(std::move(row));
    }
    context["recent_timeline"] = std::move(recent);
    return context;
}

std::string parse_synthesized_story_so_far(const std::string& response,
                                           const char* field,
                                           const char* operation) {
    const std::string safe = sanitize_utf8(response);
    const auto left = safe.find('{');
    const auto right = safe.rfind('}');
    if (left == std::string::npos || right == std::string::npos || right < left)
        throw std::runtime_error(
            std::string(operation) + " narrator returned no JSON object");

    const nlohmann::json value = nlohmann::json::parse(
        safe.substr(left, right - left + 1), nullptr, false);
    if (!value.is_object())
        throw std::runtime_error(
            std::string(operation) + " narrator returned invalid JSON");
    const auto it = value.find(field);
    if (it == value.end() || !it->is_string() || str::trim(it->get<std::string>()).empty())
        throw std::runtime_error(
            std::string(operation) + " narrator omitted " + field);
    return sanitize_utf8(str::trim(it->get<std::string>()));
}

}  // namespace

std::string synthesize_merge_context(
    World& world, TurnServices& services,
    const SceneData& source, const SceneData& target,
    ReadToolCallback read_tool) {
    TurnRunGuard run_guard(services.running);
    if (!services.narrator)
        throw std::runtime_error("No narrator LLM callback registered");

    const std::string instructions =
        "You reconcile two converging storyline contexts into one factual, "
        "compact story-so-far for the destination scene. Preserve established "
        "causality, character state, unresolved goals, and the immediate "
        "situation. Incorporate source details only when they remain relevant "
        "after convergence. Do not advance time, invent events, write dialogue, "
        "or narrate a new turn. You may use query_history(scene_id, query), "
        "query_graph(query), and query_mind(character) to resolve ambiguity. "
        "Return only JSON: {\"merged_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["source"] = scene_continuity_context(source, world);
    payload["destination"] = scene_continuity_context(target, world);
    payload["instruction"] =
        "Fold source continuity into destination continuity. The destination's "
        "recent transcript remains available separately on its next turn.";

    const std::string response = services.narrator(
        target.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "merged_story_so_far", "Merge");
}

std::string synthesize_fork_context(
    World& world, TurnServices& services,
    const SceneData& parent, const std::vector<std::string>& cast,
    const std::string& driving_intention,
    ReadToolCallback read_tool) {
    TurnRunGuard run_guard(services.running);
    if (!services.narrator)
        throw std::runtime_error("No narrator LLM callback registered");

    const std::string instructions =
        "You prepare the starting context for a new parallel storyline that "
        "has just split from its parent scene. Isolate the departing cast's "
        "relevant established situation, relationships, unresolved facts, and "
        "the immediate reason for their stated intention. Do not advance time, "
        "invent events, write dialogue, or narrate a new turn. You may use "
        "query_history(scene_id, query), query_graph(query), and "
        "query_mind(character) to resolve ambiguity. Return only JSON: "
        "{\"fork_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["parent"] = scene_continuity_context(parent, world);
    payload["fork"] = {
        {"cast", cast},
        {"driving_intention", driving_intention},
    };
    payload["instruction"] =
        "Write only the continuity needed for this departing cast's first "
        "autonomous turn. The parent keeps its own transcript.";

    const std::string response = services.narrator(
        parent.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "fork_story_so_far", "Fork");
}

namespace {

void publish_outputs(TurnServices& services, TurnResult& result) noexcept {
    if (!services.turn_complete) return;
    for (const auto& message : result.outputs) {
        try {
            services.turn_complete(message);
        } catch (const std::exception& error) {
            result.delivery_failures.push_back(error.what());
            log_warn("turn") << "committed output delivery failed: "
                             << error.what() << "\n" << std::flush;
        } catch (...) {
            result.delivery_failures.push_back("unknown callback failure");
            log_warn("turn")
                << "committed output delivery failed: unknown error\n"
                << std::flush;
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

void emit_output(SceneData& scene, SceneMessage message,
                 OutputBucket bucket, std::vector<SceneMessage>& outputs) {
    auto& target = bucket == OutputBucket::Narration
        ? scene.history : scene.dialogue;
    append_history_message(target, std::move(message));
    outputs.push_back(target.back());
}

void emit_dialogue(SceneData& scene, int turn,
                   std::uint64_t commit_version,
                   const std::vector<SpeechCue>& cues,
                   std::vector<SceneMessage>& outputs) {
    log_debug("narrator") << "emit dialogue cues=" << cues.size() << "\n";
    int ordinal = 2;
    for (const auto& cue : cues) {
        std::string spoken = str::trim(cue.field("line"));
        const std::string action = str::trim(cue.field("action"));
        if (!action.empty())
            spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");
        if (spoken.empty())
            spoken = "(" + cue.character + " is at a loss for words.)";

        auto message = make_turn_message(
            "character", std::move(spoken), cue.character);
        set_turn_metadata(message, scene.scene_id,
                          commit_version, turn, ordinal++);
        emit_output(
            scene, std::move(message), OutputBucket::Dialogue, outputs);
    }
}

void record_graph_observations(
    World& world, WorldGraph& observations, TurnServices& services,
    SceneData& scene,
    TurnResult& result, int turn, const NarratorPrompt& prompt,
    NarratorTurnResult& narrator,
    const ReadToolCallback& read_tool) noexcept {
    const World committed_world = world;
    const WorldGraph committed_observations = observations;
    const SceneData committed_scene = scene;
    try {
        World observation_world = committed_world;
        WorldGraph next_observations = committed_observations;
        GraphPlanResult output = extract_graph_observations(
            observation_world, next_observations, services, committed_scene,
            turn, prompt, narrator, read_tool);

        // Observation code never owns authoritative state. Replacing the live
        // value from the committed copy also discards any callback side effect.
        world = std::move(observation_world);
        observations = std::move(next_observations);
        scene = committed_scene;
        result.effects.created_nodes = std::move(output.new_nodes);
        result.effects.expired_nodes = std::move(output.newly_expired);

        if (services.weaver.active()) {
            std::vector<std::string> priority;
            for (const auto& node : result.effects.created_nodes) {
                priority.insert(
                    priority.end(), node.entities.begin(), node.entities.end());
            }
            services.weaver.rebuild_expiry_queue(observations, priority);
        }
    } catch (const std::exception& error) {
        world = committed_world;
        observations = committed_observations;
        scene = committed_scene;
        result.effects = {};
        narrator.plan["transitions"] = nlohmann::json::array();
        narrator.plan["new_nodes"] = nlohmann::json::array();
        log_warn("turn") << "graph observation extraction failed: "
                         << error.what() << "\n" << std::flush;
    } catch (...) {
        world = committed_world;
        observations = committed_observations;
        scene = committed_scene;
        result.effects = {};
        narrator.plan["transitions"] = nlohmann::json::array();
        narrator.plan["new_nodes"] = nlohmann::json::array();
        log_warn("turn")
            << "graph observation extraction failed: unknown error\n"
            << std::flush;
    }

    try {
        result.legacy_shadow = adapt_legacy_shadow(
            world, scene, turn, narrator, result.outputs,
            result.base_state_version, result.resulting_state_version);
    } catch (const std::exception& error) {
        log_warn("turn") << "legacy shadow adapter failed closed: "
                         << error.what() << "\n" << std::flush;
        result.legacy_shadow.reset();
    } catch (...) {
        log_warn("turn")
            << "legacy shadow adapter failed closed: unknown error\n"
            << std::flush;
        result.legacy_shadow.reset();
    }
}

}  // namespace

TurnResult execute_turn(
    StoryData& data, TurnServices& services, const TurnInput& input) {
    TurnRunGuard run_guard(services.running);
    if (!services.llm)
        throw std::runtime_error("No LLM callback registered");

    SceneData* live_scene = find_scene(data, input.scene_id);
    if (!live_scene)
        throw std::invalid_argument("Unknown scene: " + input.scene_id);

    const bool autonomous = input.kind == TurnInput::Kind::Autonomous;
    const World original_world = data.world;
    const WorldGraph original_observations = data.observations;
    const SceneData original_scene = *live_scene;
    const bool original_resuming = services.resuming;
    const std::string original_board = services.storyline_board;
    const int original_timed_turns = services.timed_turns;
    const double original_turn_ms_sum = services.turn_ms_sum;
    const std::uint64_t base_version = data.transaction_version;
    const std::uint64_t commit_version = base_version + 1;

    World candidate_world = original_world;
    SceneData candidate_scene = original_scene;
    ReadToolLease read_tools = make_frozen_story_read_tools(data, input.scene_id);
    services.storyline_board =
        format_live_storylines_board(summarize_story_scenes(data));

    TurnResult result;
    NarratorPrompt prompt;
    NarratorTurnResult narrator;
    const int turn = candidate_scene.turn_index;
    set_log_context(input.scene_id, turn, autonomous ? "offstage" : "player");
    const auto started_at = std::chrono::steady_clock::now();

    try {
        append_input_message(
            candidate_scene, input.text, autonomous, commit_version);
        prompt = build_turn_prompt(
            candidate_world, services, candidate_scene);
        narrator = run_narrator_with_retry(
            candidate_world, services, candidate_scene, turn, prompt,
            read_tools.callback());
        apply_narrator_cast(candidate_world, candidate_scene, narrator);

        auto narrator_message =
            make_turn_message("narrator", narrator.prose);
        set_turn_metadata(
            narrator_message, input.scene_id, commit_version, turn, 1);
        emit_output(candidate_scene, std::move(narrator_message),
                    OutputBucket::Narration, result.outputs);
        emit_dialogue(candidate_scene, turn, commit_version, narrator.cues,
                      result.outputs);

        if (data.transaction_version != base_version)
            throw std::runtime_error(
                "Turn state changed while generation was in progress");

        data.world = std::move(candidate_world);
        *live_scene = std::move(candidate_scene);
        data.transaction_version = commit_version;
        result.scene_id = input.scene_id;
        result.completed_turn = live_scene->turn_index;
        result.base_state_version = base_version;
        result.resulting_state_version = commit_version;
        result.post_turn_index = turn;
    } catch (...) {
        data.world = original_world;
        data.observations = original_observations;
        *live_scene = original_scene;
        data.transaction_version = base_version;
        services.resuming = original_resuming;
        services.storyline_board = original_board;
        services.timed_turns = original_timed_turns;
        services.turn_ms_sum = original_turn_ms_sum;
        clear_log_context();
        throw;
    }

    const double elapsed_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - started_at).count();
    ++services.timed_turns;
    services.turn_ms_sum += elapsed_ms;
    const double average_ms = services.turn_ms_sum /
        static_cast<double>(services.timed_turns);
    log_info("turn") << "end scene=" << input.scene_id << " id=" << turn
                     << " ms=" << static_cast<long long>(
                            std::lround(elapsed_ms))
                     << " avg=" << static_cast<long long>(
                            std::lround(average_ms)) << "\n";
    clear_log_context();

    publish_outputs(services, result);
    record_graph_observations(
        data.world, data.observations, services, *live_scene, result, turn,
        prompt, narrator, read_tools.callback());
    data.transaction_version = result.resulting_state_version;
    return result;
}

}  // namespace rhapsode
