#include "rhapsode/turn_executor.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/world.h"
#include "rhapsode/world_analysis.h"

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

bool is_affirmative_yes_response(const std::string& response) {
    const std::string lower = str::to_lower(str::trim(response));
    if (lower.size() < 3 || lower.compare(0, 3, "yes") != 0) return false;
    return lower.size() == 3 || str::is_word_boundary(lower, 3);
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

std::string build_death_confirmation_prompt(
    const DeathCandidate& candidate, const std::string& narration) {
    std::string prompt;
    prompt.reserve(1024);
    prompt += "A character death detector flagged the following character "
              "as potentially dead.\nReview the evidence and determine if "
              "they are ACTUALLY dead -- not feared dead, not hypothetically "
              "dead, not metaphorically dead.\n\n";
    prompt += "Character: " + candidate.character_name + "\n\n";
    prompt += "Current narration:\n" + narration + "\n\n";
    prompt += "World state facts mentioning this character:\n";
    for (const auto& fact : candidate.evidence) prompt += "- " + fact + "\n";
    prompt += "\nIs " + candidate.character_name
           + " dead? Answer ONLY \"yes\" or \"no\".\n";
    return prompt;
}

}  // namespace

TurnExecutor::TurnExecutor(World& world, Director& director, Weaver& weaver)
    : world_(world), director_(director), weaver_(weaver) {
    if (!director_.uses_graph(world_.graph()) ||
        !weaver_.uses_graph(world_.graph())) {
        throw std::invalid_argument(
            "TurnExecutor services must use the injected World's graph");
    }
}

void TurnExecutor::set_history_window(size_t normal, size_t resume) {
    window_size_ = normal;
    resume_window_size_ = resume;
}

TurnResult TurnExecutor::run_player_turn(SceneData& scene,
                                         const std::string& text,
                                         ReadToolCallback read_tool) {
    return run_turn(scene, text, false, std::move(read_tool));
}

TurnResult TurnExecutor::run_autonomous_turn(SceneData& scene,
                                             const std::string& focus,
                                             ReadToolCallback read_tool) {
    return run_turn(scene, focus, true, std::move(read_tool));
}

std::string TurnExecutor::synthesize_merge_context(
    const SceneData& source, const SceneData& target,
    ReadToolCallback read_tool) {
    TurnRunGuard run_guard(running_);
    if (!narrator_llm_cb_)
        throw std::runtime_error("No narrator LLM callback registered");

    const std::string instructions =
        "You reconcile two converging storyline contexts into one factual, "
        "compact story-so-far for the destination scene. Preserve established "
        "causality, character state, unresolved goals, and the immediate "
        "situation. Incorporate source details only when they remain relevant "
        "after convergence. Do not advance time, invent events, write dialogue, "
        "or narrate a new beat. You may use query_history(scene_id, query), "
        "query_graph(query), and query_mind(character) to resolve ambiguity. "
        "Return only JSON: {\"merged_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["source"] = scene_continuity_context(source, world_);
    payload["destination"] = scene_continuity_context(target, world_);
    payload["instruction"] =
        "Fold source continuity into destination continuity. The destination's "
        "recent transcript remains available separately on its next turn.";

    const std::string response = narrator_llm_cb_(
        target.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "merged_story_so_far", "Merge");
}

std::string TurnExecutor::synthesize_fork_context(
    const SceneData& parent, const std::vector<std::string>& cast,
    const std::string& driving_intention,
    ReadToolCallback read_tool) {
    TurnRunGuard run_guard(running_);
    if (!narrator_llm_cb_)
        throw std::runtime_error("No narrator LLM callback registered");

    const std::string instructions =
        "You prepare the starting context for a new parallel storyline that "
        "has just split from its parent scene. Isolate the departing cast's "
        "relevant established situation, relationships, unresolved facts, and "
        "the immediate reason for their stated intention. Do not advance time, "
        "invent events, write dialogue, or narrate a new beat. You may use "
        "query_history(scene_id, query), query_graph(query), and "
        "query_mind(character) to resolve ambiguity. Return only JSON: "
        "{\"fork_story_so_far\":\"...\"}.";

    nlohmann::json payload;
    payload["parent"] = scene_continuity_context(parent, world_);
    payload["fork"] = {
        {"cast", cast},
        {"driving_intention", driving_intention},
    };
    payload["instruction"] =
        "Write only the continuity needed for this departing cast's first "
        "autonomous beat. The parent keeps its own transcript.";

    const std::string response = narrator_llm_cb_(
        parent.scene_id, instructions, payload.dump(2), read_tool);
    return parse_synthesized_story_so_far(
        response, "fork_story_so_far", "Fork");
}

TurnResult TurnExecutor::run_turn(SceneData& scene,
                                  const std::string& text,
                                  bool autonomous,
                                  ReadToolCallback read_tool) {
    TurnRunGuard run_guard(running_);

    const SceneData scene_snapshot = scene;
    const World world_snapshot = world_;
    const bool resuming_snapshot = resuming_;
    TurnWork work;
    work.read_tool = std::move(read_tool);

    set_log_context(scene.scene_id, scene.turn_index,
                    autonomous ? "offstage" : "player");
    try {
        append_input_message(scene, text, autonomous);
        execute_turn(scene, work);

        TurnResult result;
        result.scene_id = scene.scene_id;
        result.completed_turn = scene.turn_index;
        result.post_turn_index = work.post_turn_index;
        result.outputs = std::move(work.outputs);
        result.effects.created_nodes = std::move(work.director_output.new_nodes);
        result.effects.expired_nodes = std::move(work.director_output.newly_expired);
        clear_log_context();
        return result;
    } catch (...) {
        const auto original = std::current_exception();
        scene = scene_snapshot;
        world_ = world_snapshot;
        resuming_ = resuming_snapshot;
        clear_log_context();
        std::rethrow_exception(original);
    }
}

void TurnExecutor::append_input_message(SceneData& scene,
                                        const std::string& text,
                                        bool autonomous) {
    if (autonomous) {
        log_debug("turn") << "off-stage cue scene=" << scene.scene_id << "\n";
    }

    SceneMessage message;
    message.role = Role::User;
    message.content = text;
    if (autonomous) message.metadata["scene_kind"] = "director_cue";
    append_history_message(scene.history, std::move(message));
}

void TurnExecutor::execute_turn(SceneData& scene, TurnWork& work) {
    if (!llm_cb_) throw std::runtime_error("No LLM callback registered");

    const int turn = scene.turn_index;
    const auto& ctx = log_context();
    log_info("turn") << "begin scene=" << scene.scene_id
                     << " id=" << turn
                     << " kind=" << (ctx.kind.empty() ? "?" : ctx.kind) << "\n";
    const auto t0 = std::chrono::steady_clock::now();

    const NarratorPrompt prompt = build_turn_prompt(scene);
    NarratorTurnResult result =
        run_narrator_with_retry(
            scene, turn, prompt, work.director_output, work.read_tool);
    apply_narrator_cast(scene, result);

    const std::string narration = result.prose;
    emit_output(scene,
                make_turn_message("narrator", std::move(result.prose)),
                OutputBucket::Narration, work);
    world_.route_perceptions(scene.scene_id, work.director_output.new_nodes, turn);
    emit_dialogue(scene, turn, result.cues, work);

    const auto death_candidates = find_death_candidates(world_);
    if (!death_candidates.empty()) confirm_deaths(death_candidates, narration);

    if (weaver_.active()) {
        std::vector<std::string> priority;
        for (const auto& node : work.director_output.new_nodes)
            priority.insert(priority.end(), node.entities.begin(), node.entities.end());
        weaver_.rebuild_expiry_queue(priority);
    }

    work.post_turn_index = turn;
    const double ms = std::chrono::duration<double, std::milli>(
                          std::chrono::steady_clock::now() - t0)
                          .count();
    ++timed_turns_;
    turn_ms_sum_ += ms;
    const double avg = turn_ms_sum_ / static_cast<double>(timed_turns_);
    log_info("turn") << "end scene=" << scene.scene_id << " id=" << turn
                     << " ms=" << static_cast<long long>(std::lround(ms))
                     << " avg=" << static_cast<long long>(std::lround(avg))
                     << "\n";
}

void TurnExecutor::emit_output(SceneData& scene,
                               SceneMessage message,
                               OutputBucket bucket,
                               TurnWork& work) {
    auto& target = bucket == OutputBucket::Narration ? scene.history : scene.dialogue;
    append_history_message(target, std::move(message));
    work.outputs.push_back(target.back());
    if (turn_complete_cb_) turn_complete_cb_(target.back());
}

void TurnExecutor::emit_dialogue(SceneData& scene,
                                 int turn,
                                 const std::vector<SpeechCue>& cues,
                                 TurnWork& work) {
    log_debug("narrator") << "emit dialogue cues=" << cues.size() << "\n";
    for (const auto& cue : cues) {
        std::string spoken = str::trim(cue.field("line"));
        const std::string action = str::trim(cue.field("action"));
        if (!action.empty()) spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");
        if (spoken.empty()) spoken = "(" + cue.character + " is at a loss for words.)";

        auto message = make_turn_message("character", std::move(spoken), cue.character);
        message.metadata["turn"] = turn;
        emit_output(scene, std::move(message), OutputBucket::Dialogue, work);
    }
}

void TurnExecutor::confirm_deaths(const std::vector<DeathCandidate>& candidates,
                                  const std::string& narration) {
    if (!llm_cb_) return;
    for (const auto& candidate : candidates) {
        const std::string prompt =
            build_death_confirmation_prompt(candidate, narration);
        try {
            const auto response = llm_cb_(sanitize_utf8(prompt));
            if (is_affirmative_yes_response(response)) {
                if (world_.mark_character_dead(candidate.character_name))
                    log_info("turn") << "dead " << candidate.character_name
                                     << " (confirmed)\n";
            } else {
                log_debug("turn") << "death-scan " << candidate.character_name
                                  << " -- keyword match but alive\n";
            }
        } catch (const std::exception& error) {
            log_warn("turn") << "death-scan failed for "
                             << candidate.character_name << ": " << error.what()
                             << " -- skipping\n" << std::flush;
        }
    }
}

}  // namespace rhapsode
