#include "rhapsode/scene_loop.h"
#include "rhapsode/character_memory.h"
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
    for (const auto& ch : scene.characters) {
        if (ch.is_player && resolve_cast_name(name, scene.characters) == &ch) return true;
    }
    return false;
}

bool is_npc_speech_cue(const std::string& name, const Scene& scene) {
    if (is_player_speech_name(name, scene)) return false;
    const Character* ch = resolve_cast_name(name, scene.characters);
    return ch && !ch->dead;
}

int count_speakable_npcs_in_cast(const nlohmann::json& plan, const Scene& scene) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) return 0;
    int count = 0;
    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        const Character* ch = resolve_cast_name(elem.get<std::string>(), scene.characters);
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

void SceneLoop::load_scene(Scene& scene) {
    scene_ = &scene;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::submit_input(const std::string& text) {
    if (state_ != LoopState::WaitingForInput)
        throw std::runtime_error("Cannot submit input: loop is not waiting for input");
    state_ = LoopState::ProcessingInput;

    SceneMessage user_msg;
    user_msg.role    = Role::User;
    user_msg.content = text;
    scene_->history.append(std::move(user_msg));
    advance();
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

    if (bg_future_.valid()) {
        try { bg_future_.get(); }
        catch (const std::exception& e) {
            log() << "  [bg] background work failed: " << e.what() << "\n";
        }
    }

    last_weave_result_ = std::move(bg_weave_result_);
    bg_weave_result_ = {};
    completed_expiry_ops_ = std::move(bg_expiry_ops_);
    bg_expiry_ops_.clear();

    if (!last_weave_result_.connected.empty()
        || !last_weave_result_.disconnected.empty()
        || !last_weave_result_.reweighted.empty()) {
        log() << "  [weave] +"
              << last_weave_result_.connected.size()
              << " -" << last_weave_result_.disconnected.size()
              << " ~" << last_weave_result_.reweighted.size() << "\n";
    }
}

std::vector<ExpiryOp> SceneLoop::take_completed_expiry_ops() {
    return std::exchange(completed_expiry_ops_, {});
}

void SceneLoop::dispatch_background() {
    const auto history = scene_->history.snapshot(window_size_);
    const std::string ctx =
        format_graph_seed(history, scene_->title, kGraphSeedMaxMessageChars);
    int turn = scene_->turn_index;

    bool has_weaver = weaver_ != nullptr;
    bool full_weave = has_weaver && weaver_->should_weave(turn);

    bg_stop_ = has_weaver
        ? std::function<void()>([this]() { weaver_->stop_expiry_drain(); })
        : std::function<void()>{};

    bg_future_ = std::async(std::launch::async,
        [this, turn, ctx = std::move(ctx), has_weaver, full_weave]()
    {
        if (has_weaver) {
            if (full_weave) {
                log() << "  [bg] full graph weave (cloud)...\n" << std::flush;
                bg_weave_result_ = weaver_->weave(turn, ctx);
            } else {
                log() << "  [bg] quick graph weave (local)...\n" << std::flush;
                bg_weave_result_ = weaver_->weave_local(turn, ctx);
            }

            if (!weaver_->expiry_queue_empty())
                bg_expiry_ops_ = weaver_->drain_expiry_queue(turn);
        }

        // Consolidate this turn's routed perceptions into beliefs (no-op for
        // minds that perceived nothing).
        for (auto& [name, mem] : scene_->character_memories)
            mem.reflect_perceptions(turn);

        // Downsample history off the foreground (thinking-on, multi-call). Safe
        // here: the next turn's join_background() completes this before the
        // prompt callback reads downsampler.render(), and history is not mutated
        // while the main thread waits for player input.
        if (scene_->downsampler.has_llm_callback()) {
            try {
                int before = scene_->downsampler.summarized_up_to();
                scene_->downsampler.process_turn(scene_->history.messages());
                int after = scene_->downsampler.summarized_up_to();
                log() << "  [downsampler] summarized_up_to " << before
                      << " -> " << after << "\n";
                auto rendered = scene_->downsampler.render();
                if (!rendered.empty())
                    log() << "  [downsampler] story_so_far (" << rendered.size()
                          << " chars): " << rendered.substr(0, 200)
                          << (rendered.size() > 200 ? "..." : "") << "\n";
            } catch (const std::exception& e) {
                log() << "  [downsampler] process_turn failed: " << e.what() << "\n";
            }
        }
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
    const std::string graph_seeds = format_graph_seed(history, scene_->title);
    resuming_ = false;

    NarratorPrompt prompt;
    prompt.instructions = build_narrator_instructions();
    prompt.turn_state = build_narrator_turn_state(
        history, *scene_, last_director_out_, scene_->memory(),
        director_->build_prompt__world_graph_context(turn, graph_seeds),
        scene_->build_prompt__inner_lives(turn));

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
        return sanitize_utf8(narrator_llm_cb_(safe_instructions, safe_turn_state));
    }
    return sanitize_utf8(llm_cb_(safe_instructions + "\n\n" + safe_turn_state));
}

void SceneLoop::rollback_turn_attempt(int turn, const nlohmann::json& graph_snapshot) {
    scene_->world_graph = WorldGraph::from_json(graph_snapshot);

    auto& chars = scene_->characters;
    chars.erase(std::remove_if(chars.begin(), chars.end(),
                               [turn](const Character& c) {
                                   return !c.is_player && c.created_at >= turn;
                               }),
                chars.end());
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
        ch.on_stage = true;
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
    const nlohmann::json graph_snapshot = scene_->world_graph.to_json();

    for (int attempt = 0; attempt < kMaxNarratorAttempts; ++attempt) {
        if (attempt > 0) {
            rollback_turn_attempt(turn, graph_snapshot);

            std::string rewrite_turn_state = prompt.turn_state;
            rewrite_turn_state += "\n\n### REVISION REQUIRED\n"
                                  "The following issues were found in your plan:\n";
            for (const auto& r : all_rejections) {
                rewrite_turn_state += "- " + r.fact + " -- " + r.reason + "\n";
            }
            rewrite_turn_state += "\nRewrite your narrative and plan to fix these issues.\n";

            state_ = LoopState::RunningLLM;
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

    if (!saves_dir_.empty()) {
        scene_->save(saves_dir_);
    }

    dispatch_background();
}

void SceneLoop::advance() {
    if (!llm_cb_)    throw std::runtime_error("No LLM callback registered");
    if (!director_)  throw std::runtime_error("Null director in scene");

    join_background();

    const int turn = scene_->turn_index;
    log() << "\n====== Turn " << turn << " ======\n";
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
                for (auto& ch : scene_->characters) {
                    if (ch.name == dc.character_name) {
                        ch.dead = true;
                        ch.on_stage = false;
                        log() << "  [dead] " << ch.name
                              << " (confirmed by LLM)\n";
                        break;
                    }
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
