#include "rhapsode/scene_loop.h"
#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/scene.h"
#include "rhapsode/str_util.h"

#include <exception>
#include <utility>

namespace rhapsode {

namespace {

SceneMessage make_scene_loop_message(const std::string& kind,
                                     std::string content,
                                     const std::string& speaker = {}) {
    SceneMessage msg;
    msg.role = Role::Assistant;
    msg.content = std::move(content);
    msg.metadata = {{"scene_kind", kind}};
    if (!speaker.empty()) {
        msg.metadata["speaker"] = speaker;
    }
    return msg;
}

bool is_affirmative_yes_response(const std::string& response) {
    const std::string lower = str::to_lower(str::trim(response));
    if (lower.size() < 3 || lower.compare(0, 3, "yes") != 0) {
        return false;
    }
    return lower.size() == 3 || str::is_word_boundary(lower, 3);
}

}  // anonymous namespace

// -- SceneLoop -- public interface ------------------------------------

SceneLoop::~SceneLoop() noexcept {
    try {
        join_background();
    } catch (...) {
        log() << "  [bg] failed to finish background work during shutdown\n";
    }
}

void SceneLoop::load_scene(Scene& scene) {
    // Any in-flight background work belongs to the previously loaded scene and
    // touches the shared World; finish it before we re-point at another scene so
    // its async lambda never reads the wrong scene_.
    join_background();
    scene_ = &scene;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::submit_input(const std::string& text) {
    submit_message(text, false);
}

void SceneLoop::submit_autonomous(const std::string& focus) {
    submit_message(focus, true);
}

SceneTurnResult SceneLoop::run_player_turn(Scene& scene, const std::string& text) {
    return run_turn(scene, text, false);
}

SceneTurnResult SceneLoop::run_autonomous_turn(Scene& scene, const std::string& focus) {
    return run_turn(scene, focus, true);
}

SceneTurnResult SceneLoop::run_turn(Scene& scene,
                                    const std::string& text,
                                    bool autonomous) {
    load_scene(scene);

    struct DetachScene {
        SceneLoop& loop;
        ~DetachScene() {
            loop.scene_ = nullptr;
            loop.state_ = LoopState::Idle;
        }
    } detach{*this};

    submit_message(text, autonomous);
    join_background();
    return take_scene_turn_result();
}

SceneTurnResult SceneLoop::take_scene_turn_result() {
    SceneTurnResult result;
    result.scene_id = scene_->scene_id;
    result.completed_turn = scene_->turn_index;
    result.outputs = std::exchange(last_turn_outputs_, {});
    result.director = std::exchange(last_director_out_, {});
    result.weave = std::exchange(last_weave_result_, {});
    result.expiry = std::exchange(completed_expiry_ops_, {});
    return result;
}

void SceneLoop::validate_runtime_graph() const {
    if (!director_)
        throw std::runtime_error("Null director in scene");
    const WorldGraph& graph = scene_->world().world_graph;
    if (!director_->uses_graph(graph))
        throw std::runtime_error("Director is bound to a different WorldGraph");
    if (weaver_ && !weaver_->uses_graph(graph))
        throw std::runtime_error("Weaver is bound to a different WorldGraph");
}

void SceneLoop::submit_message(const std::string& text, bool autonomous) {
    if (state_ != LoopState::WaitingForInput)
        throw std::runtime_error("Cannot submit message: loop is not waiting for input");

    validate_runtime_graph();

    // The previous beat is part of the state being snapshotted. Finish it first
    // so rollback never races a background writer.
    join_background();

    const History history_snapshot = scene_->history;
    const History dialogue_snapshot = scene_->dialogue;
    const TextDownsampler downsampler_snapshot = scene_->downsampler;
    const int turn_snapshot = scene_->turn_index;
    const World world_snapshot = scene_->world();
    const bool resuming_snapshot = resuming_;
    const auto outputs_snapshot = last_turn_outputs_;
    const auto director_snapshot = last_director_out_;
    const auto weave_snapshot = last_weave_result_;
    const auto expiry_snapshot = completed_expiry_ops_;

    state_ = LoopState::ProcessingInput;

    try {
        if (autonomous) {
            log() << "\n[off-stage beat] advancing scene '" << scene_->scene_id
                  << "' player-lessly\n" << std::flush;
        }

        SceneMessage message;
        message.role = Role::User;
        message.content = text;
        if (autonomous) message.metadata["scene_kind"] = "director_cue";
        scene_->history.append(std::move(message));
        advance();
    } catch (...) {
        const auto original = std::current_exception();
        try { join_background(); } catch (...) {}

        scene_->history = history_snapshot;
        scene_->dialogue = dialogue_snapshot;
        scene_->downsampler = downsampler_snapshot;
        scene_->turn_index = turn_snapshot;
        scene_->world() = world_snapshot;
        resuming_ = resuming_snapshot;
        last_turn_outputs_ = outputs_snapshot;
        last_director_out_ = director_snapshot;
        last_weave_result_ = weave_snapshot;
        completed_expiry_ops_ = expiry_snapshot;
        bg_stop_ = {};
        background_save_pending_ = false;
        state_ = LoopState::WaitingForInput;

        if (!saves_dir_.empty()) {
            try { scene_->save(saves_dir_); }
            catch (const std::exception& e) {
                log() << "  [rollback] failed to restore save: " << e.what() << "\n";
            } catch (...) {
                log() << "  [rollback] failed to restore save\n";
            }
        }
        std::rethrow_exception(original);
    }
}

LoopState SceneLoop::state() const { return state_; }

void SceneLoop::set_llm_callback(LLMCallback cb)               { llm_cb_          = std::move(cb); }
void SceneLoop::set_narrator_llm_callback(NarratorLLMCallback cb) { narrator_llm_cb_ = std::move(cb); }
void SceneLoop::set_turn_complete_callback(TurnCompleteCallback cb) { turn_complete_cb_ = std::move(cb); }
void SceneLoop::set_director(Director* director)                { director_        = director; }
void SceneLoop::set_weaver(Weaver* weaver)                      { weaver_          = weaver; }
const WeaveResult& SceneLoop::last_weave_result() const          { return last_weave_result_; }

const DirectorOutput& SceneLoop::last_director_output() const   { return last_director_out_; }

std::vector<SceneMessage> SceneLoop::take_last_turn_outputs() {
    return std::exchange(last_turn_outputs_, {});
}

void SceneLoop::set_history_window(size_t normal, size_t resume) {
    window_size_        = normal;
    resume_window_size_ = resume;
}

void SceneLoop::set_saves_dir(const std::string& dir) { saves_dir_ = dir; }

// -- SceneLoop -- private helpers -------------------------------------

void SceneLoop::emit_output(SceneMessage msg, OutputBucket bucket) {
    History& target = bucket == OutputBucket::Story ? scene_->history : scene_->dialogue;
    target.append(std::move(msg));
    last_turn_outputs_.push_back(target.messages().back());
    if (turn_complete_cb_)
        turn_complete_cb_(target.messages().back());
}

void SceneLoop::emit_dialogue(int turn, const std::vector<SpeechCue>& cues) {
    log() << "[4/4] Emit authored dialogue...\n" << std::flush;

    for (const auto& cue : cues) {
        std::string spoken = str::trim(cue.field("line"));
        std::string action = str::trim(cue.field("action"));
        if (!action.empty()) {
            spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");
        }

        if (spoken.empty()) {
            spoken = "(" + cue.character + " is at a loss for words.)";
        }

        auto msg = make_scene_loop_message("character", std::move(spoken), cue.character);
        msg.metadata["turn"] = turn;
        emit_output(std::move(msg), OutputBucket::Dialogue);
    }
}

void SceneLoop::post_turn_cleanup(const std::string& narration) {
    const auto death_candidates = scene_->scan_death_candidates();
    if (!death_candidates.empty()) {
        confirm_deaths(death_candidates, narration);
    }

    if (weaver_) {
        std::vector<std::string> prio;
        for (const auto& n : last_director_out_.new_nodes) {
            for (const auto& e : n.entities) {
                prio.push_back(e);
            }
        }
        weaver_->rebuild_expiry_queue(prio);
    }

    dispatch_background();
    background_save_pending_ = !saves_dir_.empty();
}

void SceneLoop::advance() {
    if (!llm_cb_)    throw std::runtime_error("No LLM callback registered");

    join_background();

    const int turn = scene_->turn_index;
    log() << "\n====== Turn " << turn << " [" << scene_->scene_id << "] ======\n";
    last_turn_outputs_.clear();

    const NarratorPrompt prompt = build_turn_prompt(turn);
    NarratorTurnResult result = run_narrator_with_retry(turn, prompt);

    apply_narrator_cast(result);

    const std::string narration = result.prose;
    emit_output(make_scene_loop_message("narrator", std::move(result.prose)), OutputBucket::Story);
    scene_->world().route_perceptions(
        scene_->scene_id, last_director_out_.new_nodes, turn);

    emit_dialogue(turn, result.cues);
    post_turn_cleanup(narration);

    log() << "====== Turn " << turn << " done ======\n" << std::flush;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::confirm_deaths(const std::vector<DeathCandidate>& candidates,
                               const std::string& narration) {
    const auto& llm = llm_cb_;
    if (!llm) return;

    for (const auto& dc : candidates) {
        std::string prompt;
        prompt.reserve(1024);
        prompt += "A character death detector flagged the following character "
                  "as potentially dead.\nReview the evidence and determine if "
                  "they are ACTUALLY dead -- not feared dead, not hypothetically "
                  "dead, not metaphorically dead.\n\n";
        prompt += "Character: " + dc.character_name + "\n\n";
        prompt += "Current narration:\n" + narration + "\n\n";
        prompt += "World state facts mentioning this character:\n";
        for (const auto& fact : dc.evidence)
            prompt += "- " + fact + "\n";
        prompt += "\nIs " + dc.character_name
               +  " dead? Answer ONLY \"yes\" or \"no\".\n";

        try {
            auto response = llm(sanitize_utf8(prompt));
            if (is_affirmative_yes_response(response)) {
                if (scene_->world().mark_character_dead(dc.character_name)) {
                    log() << "  [dead] " << dc.character_name
                          << " (confirmed by LLM)\n";
                }
            } else {
                log() << "  [death-scan] " << dc.character_name
                      << " -- keyword match but LLM says alive\n";
            }
        } catch (const std::exception& e) {
            log() << "  [death-scan] LLM confirmation failed for "
                  << dc.character_name << ": " << e.what()
                  << " -- skipping (fail-safe)\n";
        }
    }
}

}  // namespace rhapsode
