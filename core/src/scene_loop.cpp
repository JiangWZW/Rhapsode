#include "rhapsode/scene_loop.h"
#include "rhapsode/director.h"
#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/narrator_prompt.h"
#include "rhapsode/scene.h"
#include "rhapsode/scene_loop_support.h"
#include "rhapsode/str_util.h"

#include <algorithm>
#include <exception>
#include <functional>
#include <optional>
#include <tuple>
#include <utility>

#include <nlohmann/json.hpp>

namespace rhapsode {

namespace {

constexpr size_t kGraphSeedMessages = 4;
constexpr size_t kGraphSeedMaxMessageChars = 300;
constexpr int kMaxNarratorAttempts = 3;

std::string format_graph_seed(const std::vector<SceneMessage>& history,
                              const std::string& title,
                              std::optional<size_t> cap_per_msg = std::nullopt) {
    std::string out = title;
    const size_t start = history.size() > kGraphSeedMessages
                             ? history.size() - kGraphSeedMessages
                             : 0;
    for (size_t i = start; i < history.size(); ++i) {
        const auto& msg = history[i];
        if (msg.role != Role::User && msg.role != Role::Assistant)
            continue;
        out += '\n';
        out += (msg.role == Role::User ? "user: " : "assistant: ");
        out += cap_per_msg ? truncate_utf8(msg.content, *cap_per_msg) : msg.content;
    }
    return out;
}

bool is_player_speech_name(const std::string& name, const Scene& scene) {
    if (str::to_lower(name) == "player") return true;
    for (const auto& ch : scene.world().characters) {
        if (ch.is_player && resolve_cast_name(name, scene.world().characters) == &ch) return true;
    }
    return false;
}

bool is_npc_speech_cue(const std::string& name, const Scene& scene) {
    if (is_player_speech_name(name, scene)) return false;
    const Character* ch = resolve_cast_name(name, scene.world().characters);
    return ch && !ch->dead;
}

int count_speakable_npcs_in_cast(const nlohmann::json& plan, const Scene& scene) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) return 0;
    int count = 0;
    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        const Character* ch = resolve_cast_name(elem.get<std::string>(), scene.world().characters);
        if (ch && !ch->dead) ++count;
    }
    return count;
}

std::vector<Rejection> validate_speech_turns(const nlohmann::json& plan,
                                             const Scene& scene) {
    std::vector<Rejection> rejections;
    if (!plan.contains("speech_turns") || !plan["speech_turns"].is_array())
        return rejections;

    const auto& turns = plan["speech_turns"];
    int npc_cues = 0;
    for (const auto& el : turns) {
        if (!el.is_object()) continue;
        const auto name = el.value("character", "");
        if (name.empty()) continue;
        if (is_player_speech_name(name, scene)) {
            rejections.push_back({
                "speech_turns includes \"" + name + "\"",
                "Player must not appear in speech_turns; the user message is the "
                "player's speech -- give responding NPCs their own speech_turn entries"
            });
            continue;
        }
        if (is_npc_speech_cue(name, scene)) ++npc_cues;
    }

    if (turns.empty()) return rejections;

    if (count_speakable_npcs_in_cast(plan, scene) > 0 && npc_cues == 0) {
        rejections.push_back({
            "speech_turns",
            "NPCs are present in active_cast but no NPC speech_turns were authored "
            "(speech_turns must contain each speaking NPC's line, not the Player's)"
        });
    }
    return rejections;
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

void SceneLoop::submit_message(const std::string& text, bool autonomous) {
    if (state_ != LoopState::WaitingForInput)
        throw std::runtime_error("Cannot submit message: loop is not waiting for input");

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

void SceneLoop::join_background() {
    if (bg_stop_) {
        try { bg_stop_(); } catch (...) {}
        bg_stop_ = {};
    }

    BackgroundResult completed;
    if (bg_future_.valid()) {
        try { completed = bg_future_.get(); }
        catch (const std::exception& e) {
            log() << "  [bg] background work failed: " << e.what() << "\n";
        }
        catch (...) {
            log() << "  [bg] background work failed with an unknown exception\n";
        }
    }

    last_weave_result_ = std::move(completed.weave);
    completed_expiry_ops_ = std::move(completed.expiry);

    if (!last_weave_result_.connected.empty()
        || !last_weave_result_.disconnected.empty()
        || !last_weave_result_.reweighted.empty()) {
        log() << "  [weave] +"
              << last_weave_result_.connected.size()
              << " -" << last_weave_result_.disconnected.size()
              << " ~" << last_weave_result_.reweighted.size() << "\n";
    }

    if (background_save_pending_) {
        background_save_pending_ = false;
        if (!saves_dir_.empty() && scene_) scene_->save(saves_dir_);
    }
}

std::vector<ExpiryOp> SceneLoop::take_completed_expiry_ops() {
    return std::exchange(completed_expiry_ops_, {});
}

void SceneLoop::dispatch_background() {
    Scene* const scene = scene_;
    Weaver* const weaver = weaver_;
    const auto history = scene->history.snapshot(window_size_);
    const std::string ctx =
        format_graph_seed(history, scene->title, kGraphSeedMaxMessageChars);
    int turn = scene->turn_index;

    bool has_weaver = weaver != nullptr;
    bool full_weave = has_weaver && weaver->should_weave(turn);

    bg_stop_ = has_weaver
        ? std::function<void()>([weaver]() { weaver->stop_expiry_drain(); })
        : std::function<void()>{};

    bg_future_ = std::async(std::launch::async,
        [scene, weaver, turn, ctx = std::move(ctx), has_weaver, full_weave]()
            -> BackgroundResult
    {
        BackgroundResult result;
        if (has_weaver) {
            if (full_weave) {
                log() << "  [bg] full graph weave (cloud)...\n" << std::flush;
                result.weave = weaver->weave(turn, ctx);
            } else {
                log() << "  [bg] quick graph weave (local)...\n" << std::flush;
                result.weave = weaver->weave_local(turn, ctx);
            }

            if (!weaver->expiry_queue_empty())
                result.expiry = weaver->drain_expiry_queue(turn);
        }

        // Consolidate this turn's routed perceptions into beliefs (no-op for
        // minds that perceived nothing).
        scene->world().reflect_perceptions(turn);

        // Downsample history off the foreground (thinking-on, multi-call). Safe
        // here: the next turn's join_background() completes this before the
        // prompt callback reads downsampler.render(), and history is not mutated
        // while the main thread waits for player input.
        if (scene->downsampler.has_llm_callback()) {
            try {
                int before = scene->downsampler.summarized_up_to();
                scene->downsampler.process_turn(scene->history.messages());
                int after = scene->downsampler.summarized_up_to();
                log() << "  [downsampler] summarized_up_to " << before
                      << " -> " << after << "\n";
                auto rendered = scene->downsampler.render();
                if (!rendered.empty())
                    log() << "  [downsampler] story_so_far (" << rendered.size()
                          << " chars): " << rendered.substr(0, 200)
                          << (rendered.size() > 200 ? "..." : "") << "\n";
            } catch (const std::exception& e) {
                log() << "  [downsampler] process_turn failed: " << e.what() << "\n";
            }
        }
        return result;
    });
}

// -- SceneLoop -- private helpers -------------------------------------

void SceneLoop::emit_output(SceneMessage msg, OutputBucket bucket) {
    History& target = bucket == OutputBucket::Story ? scene_->history : scene_->dialogue;
    target.append(std::move(msg));
    last_turn_outputs_.push_back(target.messages().back());
    if (turn_complete_cb_)
        turn_complete_cb_(target.messages().back());
}

// -- SceneLoop::advance -- the four-phase turn pipeline --------------

NarratorPrompt SceneLoop::build_turn_prompt(int turn) {
    state_ = LoopState::BuildingPrompt;
    log() << "[1/4] Building merged prompt...\n" << std::flush;

    const size_t win = resuming_ ? resume_window_size_ : window_size_;
    const std::vector<SceneMessage> history = scene_->history.snapshot(win);
    resuming_ = false;

    NarratorPrompt prompt;
    prompt.instructions = build_narrator_instructions();
    prompt.turn_state = build_narrator_turn_state(
        history, *scene_);

    ++scene_->turn_index;

    log() << "  [prompt] instructions=" << prompt.instructions.size()
          << " turn_state=" << prompt.turn_state.size() << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log() << "--- NARRATOR INSTRUCTIONS ---\n" << prompt.instructions << "\n"
              << "--- NARRATOR TURN STATE ---\n" << prompt.turn_state << "\n"
              << "--- END NARRATOR PROMPT ---\n" << std::flush;
    }
    return prompt;
}

std::string SceneLoop::call_narrator(const std::string& instructions,
                                     const std::string& turn_state) const {
    const std::string safe_instructions = sanitize_utf8(instructions);
    const std::string safe_turn_state   = sanitize_utf8(turn_state);
    if (narrator_llm_cb_) {
        return sanitize_utf8(narrator_llm_cb_(scene_->scene_id, safe_instructions, safe_turn_state));
    }
    return sanitize_utf8(llm_cb_(safe_instructions + "\n\n" + safe_turn_state));
}

void SceneLoop::rollback_turn_attempt(const World& world_snapshot) {
    scene_->world() = world_snapshot;
}

void SceneLoop::register_new_characters(int turn, const nlohmann::json& plan) {
    if (!plan.contains("new_characters") || !plan["new_characters"].is_array()) {
        return;
    }

    for (const auto& ch_j : plan["new_characters"]) {
        Character ch;
        ch.name = ch_j.value("name", "");
        ch.description = ch_j.value("description", "");
        ch.dialogue_instructions = ch_j.value("dialogue_instructions", "");
        ch.role = ch_j.value("role", "minor_npc");
        ch.created_at = turn;

        if (!ch.name.empty() && !scene_->find_on_stage(ch.name)) {
            log() << "  [enter] " << ch.name << " enters the stage\n";
            scene_->enter_character(std::move(ch));
        }
    }
}

NarratorTurnResult SceneLoop::run_narrator_with_retry(int turn,
                                                      const NarratorPrompt& prompt) {
    state_ = LoopState::RunningLLM;
    log() << "[2/4] Calling narrative LLM...\n" << std::flush;

    // Discard any lifecycle ops a prior attempt staged; only the accepted
    // attempt's decision-tool calls survive to be applied after the beat.
    scene_->world().clear_pending_ops();
    auto raw_response = call_narrator(prompt.instructions, prompt.turn_state);
    log() << "  [narrator] response=" << raw_response.size() << " chars\n" << std::flush;
    if (verbose_logging_enabled()) {
        log() << "--- NARRATOR RESPONSE ---\n" << raw_response
              << "\n--- END NARRATOR RESPONSE ---\n" << std::flush;
    }

    NarratorTurnResult result;
    std::tie(result.prose, result.plan) = split_merged_response(std::move(raw_response));

    state_ = LoopState::AppendingResult;
    log() << "[3/4] Applying graph...\n" << std::flush;

    std::vector<Rejection> all_rejections;
    const World world_snapshot = scene_->world();

    for (int attempt = 0; attempt < kMaxNarratorAttempts; ++attempt) {
        if (attempt > 0) {
            rollback_turn_attempt(world_snapshot);

            std::string rewrite_turn_state = prompt.turn_state;
            rewrite_turn_state += "\n\n### REVISION REQUIRED\n"
                                  "The following issues were found in your plan:\n";
            for (const auto& r : all_rejections) {
                rewrite_turn_state += "- " + r.fact + " -- " + r.reason + "\n";
            }
            rewrite_turn_state += "\nRewrite your narrative and plan to fix these issues.\n";

            state_ = LoopState::RunningLLM;
            scene_->world().clear_pending_ops();
            auto [new_prose, new_plan] =
                split_merged_response(call_narrator(prompt.instructions, rewrite_turn_state));
            result.prose = std::move(new_prose);
            result.plan = std::move(new_plan);
            state_ = LoopState::AppendingResult;
        }

        last_director_out_ = director_->apply_planned_turn(turn, result.plan);
        register_new_characters(turn, result.plan);

        const auto cast_rejections = validate_active_cast(result.plan, *scene_);
        const auto speech_rejections = validate_speech_turns(result.plan, *scene_);
        all_rejections = last_director_out_.rejections;
        all_rejections.insert(all_rejections.end(), cast_rejections.begin(), cast_rejections.end());
        all_rejections.insert(all_rejections.end(), speech_rejections.begin(), speech_rejections.end());

        if (all_rejections.empty()) {
            break;
        }

        log() << "  [retry] attempt " << (attempt + 1) << "/" << kMaxNarratorAttempts
              << ": " << all_rejections.size() << " issue(s)\n";
        for (const auto& r : all_rejections) {
            log() << "    - " << r.fact << " -- " << r.reason << "\n";
        }
        log() << std::flush;
    }

    result.cues = extract_speech_cues(result.plan);
    return result;
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
    if (!director_)  throw std::runtime_error("Null director in scene");

    join_background();

    const int turn = scene_->turn_index;
    log() << "\n====== Turn " << turn << " [" << scene_->scene_id << "] ======\n";
    last_turn_outputs_.clear();

    const NarratorPrompt prompt = build_turn_prompt(turn);
    NarratorTurnResult result = run_narrator_with_retry(turn, prompt);

    apply_active_cast(result.plan, result.cues, *scene_);

    const std::string narration = result.prose;
    emit_output(make_scene_loop_message("narrator", std::move(result.prose)), OutputBucket::Story);
    route_perception(*scene_, last_director_out_.new_nodes, turn);

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
