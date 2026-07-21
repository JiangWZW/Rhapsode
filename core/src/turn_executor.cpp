#include "rhapsode/turn_executor.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"
#include "rhapsode/world.h"
#include "rhapsode/world_analysis.h"

#include <algorithm>
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

TurnResult TurnExecutor::run_turn(SceneData& scene,
                                  const std::string& text,
                                  bool autonomous,
                                  ReadToolCallback read_tool) {
    if (running_)
        throw std::runtime_error("Cannot run turn: loop is already active");

    running_ = true;

    const SceneData scene_snapshot = scene;
    const World world_snapshot = world_;
    const bool resuming_snapshot = resuming_;
    TurnWork work;
    work.read_tool = std::move(read_tool);

    try {
        submit_message(scene, text, autonomous, work);
        PostTurnResult completed = advance(scene, work);

        TurnResult result;
        result.scene_id = scene.scene_id;
        result.completed_turn = scene.turn_index;
        result.outputs = std::move(work.outputs);
        result.effects.created_nodes = std::move(work.director.new_nodes);
        result.effects.expired_nodes = std::move(work.director.newly_expired);
        for (auto& node : completed.expired_nodes) {
            const auto duplicate = std::find_if(
                result.effects.expired_nodes.begin(),
                result.effects.expired_nodes.end(),
                [&](const Node& existing) { return existing.id == node.id; });
            if (duplicate == result.effects.expired_nodes.end())
                result.effects.expired_nodes.push_back(std::move(node));
        }
        running_ = false;
        return result;
    } catch (...) {
        const auto original = std::current_exception();
        scene = scene_snapshot;
        world_ = world_snapshot;
        resuming_ = resuming_snapshot;
        running_ = false;
        std::rethrow_exception(original);
    }
}

void TurnExecutor::submit_message(SceneData& scene,
                                  const std::string& text,
                                  bool autonomous,
                                  TurnWork&) {
    if (autonomous) {
        log() << "\n[off-stage beat] advancing scene '" << scene.scene_id
              << "' player-lessly\n" << std::flush;
    }

    SceneMessage message;
    message.role = Role::User;
    message.content = text;
    if (autonomous) message.metadata["scene_kind"] = "director_cue";
    scene.history.append(std::move(message));
}

TurnExecutor::PostTurnResult TurnExecutor::advance(SceneData& scene,
                                                   TurnWork& work) {
    if (!llm_cb_) throw std::runtime_error("No LLM callback registered");

    const int turn = scene.turn_index;
    log() << "\n====== Turn " << turn << " [" << scene.scene_id << "] ======\n";

    const NarratorPrompt prompt = build_turn_prompt(scene, turn);
    NarratorTurnResult result =
        run_narrator_with_retry(
            scene, turn, prompt, work.director, work.read_tool);
    apply_narrator_cast(scene, result);

    const std::string narration = result.prose;
    emit_output(scene,
                make_turn_message("narrator", std::move(result.prose)),
                OutputBucket::Story, work);
    world_.route_perceptions(scene.scene_id, work.director.new_nodes, turn);
    emit_dialogue(scene, turn, result.cues, work);

    const auto death_candidates = find_death_candidates(world_);
    if (!death_candidates.empty()) confirm_deaths(death_candidates, narration);

    if (weaver_.active()) {
        std::vector<std::string> priority;
        for (const auto& node : work.director.new_nodes)
            priority.insert(priority.end(), node.entities.begin(), node.entities.end());
        weaver_.rebuild_expiry_queue(priority);
    }

    log() << "====== Turn " << turn << " done ======\n" << std::flush;
    return run_post_turn(scene, turn, work.director);
}

void TurnExecutor::emit_output(SceneData& scene,
                               SceneMessage message,
                               OutputBucket bucket,
                               TurnWork& work) {
    History& target = bucket == OutputBucket::Story ? scene.history : scene.dialogue;
    target.append(std::move(message));
    work.outputs.push_back(target.messages().back());
    if (turn_complete_cb_) turn_complete_cb_(target.messages().back());
}

void TurnExecutor::emit_dialogue(SceneData& scene,
                                 int turn,
                                 const std::vector<SpeechCue>& cues,
                                 TurnWork& work) {
    log() << "[4/4] Emit authored dialogue...\n" << std::flush;
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

        try {
            const auto response = llm_cb_(sanitize_utf8(prompt));
            if (is_affirmative_yes_response(response)) {
                if (world_.mark_character_dead(candidate.character_name))
                    log() << "  [dead] " << candidate.character_name
                          << " (confirmed by LLM)\n";
            } else {
                log() << "  [death-scan] " << candidate.character_name
                      << " -- keyword match but LLM says alive\n";
            }
        } catch (const std::exception& error) {
            log() << "  [death-scan] LLM confirmation failed for "
                  << candidate.character_name << ": " << error.what()
                  << " -- skipping (fail-safe)\n";
        }
    }
}

}  // namespace rhapsode
