#include "rhapsode/scene_loop.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/str_util.h"
#include "rhapsode/world.h"

#include <exception>
#include <utility>

namespace rhapsode {
namespace {

SceneMessage make_scene_loop_message(const std::string& kind,
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

SceneLoop::SceneLoop(World& world)
    : world_(world), director_(world.graph()) {}

Weaver& SceneLoop::ensure_weaver() {
    if (!weaver_) weaver_ = std::make_unique<Weaver>(world_.graph());
    return *weaver_;
}

void SceneLoop::set_weaver_llm_callback(LLMCallback cb) {
    ensure_weaver().set_llm_callback(std::move(cb));
}

void SceneLoop::set_weaver_local_llm_callback(LLMCallback cb) {
    ensure_weaver().set_local_llm_callback(std::move(cb));
}

void SceneLoop::set_weaver_interval(int turns) {
    ensure_weaver().set_interval(turns);
}

void SceneLoop::set_history_window(size_t normal, size_t resume) {
    window_size_ = normal;
    resume_window_size_ = resume;
}

SceneTurnResult SceneLoop::run_player_turn(SceneData& scene,
                                           const std::string& text) {
    return run_turn(scene, text, false);
}

SceneTurnResult SceneLoop::run_autonomous_turn(SceneData& scene,
                                               const std::string& focus) {
    return run_turn(scene, focus, true);
}

SceneTurnResult SceneLoop::run_turn(SceneData& scene,
                                    const std::string& text,
                                    bool autonomous) {
    if (state_ != LoopState::Idle)
        throw std::runtime_error("Cannot run turn: loop is already active");

    const SceneData scene_snapshot = scene;
    const World world_snapshot = world_;
    const bool resuming_snapshot = resuming_;
    TurnWork work;
    std::future<BackgroundResult> background;

    try {
        submit_message(scene, text, autonomous, work);
        background = advance(scene, work);
        BackgroundResult completed = finish_background(background);

        SceneTurnResult result;
        result.scene_id = scene.scene_id;
        result.completed_turn = scene.turn_index;
        result.outputs = std::move(work.outputs);
        result.director = std::move(work.director);
        result.weave = std::move(completed.weave);
        result.expiry = std::move(completed.expiry);
        state_ = LoopState::Idle;
        return result;
    } catch (...) {
        const auto original = std::current_exception();
        if (weaver_) weaver_->stop_expiry_drain();
        if (background.valid()) (void)finish_background(background);
        scene = scene_snapshot;
        world_ = world_snapshot;
        resuming_ = resuming_snapshot;
        state_ = LoopState::Idle;
        std::rethrow_exception(original);
    }
}

void SceneLoop::submit_message(SceneData& scene,
                               const std::string& text,
                               bool autonomous,
                               TurnWork&) {
    state_ = LoopState::ProcessingInput;
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

std::future<SceneLoop::BackgroundResult> SceneLoop::advance(SceneData& scene,
                                                            TurnWork& work) {
    if (!llm_cb_) throw std::runtime_error("No LLM callback registered");

    const int turn = scene.turn_index;
    log() << "\n====== Turn " << turn << " [" << scene.scene_id << "] ======\n";

    const NarratorPrompt prompt = build_turn_prompt(scene, turn);
    NarratorTurnResult result =
        run_narrator_with_retry(scene, turn, prompt, work.director);
    apply_narrator_cast(scene, result);

    const std::string narration = result.prose;
    emit_output(scene,
                make_scene_loop_message("narrator", std::move(result.prose)),
                OutputBucket::Story, work);
    world_.route_perceptions(scene.scene_id, work.director.new_nodes, turn);
    emit_dialogue(scene, turn, result.cues, work);

    const auto death_candidates = world_.scan_death_candidates();
    if (!death_candidates.empty()) confirm_deaths(death_candidates, narration);

    if (weaver_) {
        std::vector<std::string> priority;
        for (const auto& node : work.director.new_nodes)
            priority.insert(priority.end(), node.entities.begin(), node.entities.end());
        weaver_->rebuild_expiry_queue(priority);
    }

    log() << "====== Turn " << turn << " done ======\n" << std::flush;
    return dispatch_background(scene, turn, work.director);
}

void SceneLoop::emit_output(SceneData& scene,
                            SceneMessage message,
                            OutputBucket bucket,
                            TurnWork& work) {
    History& target = bucket == OutputBucket::Story ? scene.history : scene.dialogue;
    target.append(std::move(message));
    work.outputs.push_back(target.messages().back());
    if (turn_complete_cb_) turn_complete_cb_(target.messages().back());
}

void SceneLoop::emit_dialogue(SceneData& scene,
                              int turn,
                              const std::vector<SpeechCue>& cues,
                              TurnWork& work) {
    log() << "[4/4] Emit authored dialogue...\n" << std::flush;
    for (const auto& cue : cues) {
        std::string spoken = str::trim(cue.field("line"));
        const std::string action = str::trim(cue.field("action"));
        if (!action.empty()) spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");
        if (spoken.empty()) spoken = "(" + cue.character + " is at a loss for words.)";

        auto message = make_scene_loop_message("character", std::move(spoken), cue.character);
        message.metadata["turn"] = turn;
        emit_output(scene, std::move(message), OutputBucket::Dialogue, work);
    }
}

void SceneLoop::confirm_deaths(const std::vector<DeathCandidate>& candidates,
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
