#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace rhapsode {

namespace {

// -- Utilities -------------------------------------------------------

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.pop_back();
    return s;
}

std::string truncate(const std::string& s, size_t max_len) {
    if (s.size() <= max_len) return s;
    size_t pos = max_len;
    // Back up past any UTF-8 continuation bytes (10xxxxxx) to avoid
    // splitting a multi-byte character.
    while (pos > 0 && (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80)
        --pos;
    return s.substr(0, pos);
}

std::string to_lower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool name_matches(const std::string& cast_name, const std::string& char_name) {
    auto lc = to_lower(cast_name);
    auto ln = to_lower(char_name);
    return lc == ln
        || ln.find(lc) != std::string::npos
        || lc.find(ln) != std::string::npos;
}

const Character* resolve_cast_name(const std::string& cast_name,
                                   const std::vector<Character>& characters) {
    const Character* best = nullptr;
    for (const auto& ch : characters) {
        if (ch.is_player) continue;
        if (!name_matches(cast_name, ch.name)) continue;
        if (!best || to_lower(cast_name) == to_lower(ch.name)) {
            best = &ch;
            if (to_lower(cast_name) == to_lower(ch.name)) break;
        }
    }
    return best;
}

std::vector<Rejection> validate_active_cast(const nlohmann::json& plan,
                                            const Scene& scene) {
    std::vector<Rejection> rejections;
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array())
        return rejections;

    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        auto name = elem.get<std::string>();
        const Character* ch = resolve_cast_name(name, scene.characters);
        if (!ch) {
            // Unknown name (the player, a group like "the soldiers", or an
            // un-introduced minor) is harmless: apply_active_cast ignores names
            // it can't resolve.  Warn, but do NOT force an expensive rewrite.
            std::cerr << "  [cast] active_cast lists unresolved \"" << name
                      << "\" -- ignoring\n";
        } else if (ch->dead) {
            rejections.push_back({
                "active_cast includes \"" + name + "\"",
                "character is dead"
            });
        }
    }
    return rejections;
}

// -- LLM response parsing --------------------------------------------

constexpr char kJsonToken[] = "<<<RHAPSODE_JSON>>>";

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw) {
    // Normalize smart quotes etc. up front so both the marker split and the
    // brace-fallback below operate on parseable text (models frequently emit
    // curly quotes that otherwise corrupt the whole structured plan).
    raw = normalize_json_punct(std::move(raw));

    // Tolerate the marker with or without its surrounding newlines.
    auto marker = raw.find(kJsonToken);
    if (marker != std::string::npos) {
        auto prose = trim(raw.substr(0, marker));
        auto json  = trim(raw.substr(marker + std::strlen(kJsonToken)));
        return {std::move(prose), json.empty() ? nlohmann::json::object()
                                               : try_parse_json(json)};
    }

    // Fallback (model omitted the sentinel): scan braces left-to-right and take
    // the FIRST balanced object that parses AND looks like a plan.  Scanning from
    // the first '{' captures the OUTERMOST object; the old rfind('{') grabbed the
    // last nested object (e.g. the final speech_turns entry), which left the rest
    // of the JSON sitting in the prose -- the leak.  Requiring a known key lets us
    // skip a stray '{' that might appear in narration.
    static constexpr std::array<const char*, 5> kPlanKeys = {
        "transitions", "new_nodes", "speech_turns", "active_cast", "new_characters"};
    for (auto brace = raw.find('{'); brace != std::string::npos;
         brace = raw.find('{', brace + 1)) {
        std::string fragment;
        if (!extract_balanced_json(std::string_view(raw).substr(brace), fragment))
            break;  // no balanced object from here on
        try {
            auto plan = nlohmann::json::parse(fragment);
            if (plan.is_object() &&
                std::any_of(kPlanKeys.begin(), kPlanKeys.end(),
                            [&](const char* k) { return plan.contains(k); })) {
                return {trim(raw.substr(0, brace)), std::move(plan)};
            }
        } catch (...) {}
    }

    return {trim(std::move(raw)), nlohmann::json::object()};
}

// -- Speech cues -----------------------------------------------------

struct SpeechCue {
    std::string    character;
    nlohmann::json direction;

    std::string field(const char* key) const { return direction.value(key, ""); }
};

std::vector<SpeechCue> extract_speech_cues(const nlohmann::json& plan) {
    std::vector<SpeechCue> cues;
    auto it = plan.find("speech_turns");
    if (it == plan.end() || !it->is_array()) return cues;

    for (const auto& el : *it) {
        if (!el.is_object()) continue;
        auto name = el.value("character", "");
        if (!name.empty())
            cues.push_back({std::move(name), el});
    }
    return cues;
}

// -- Active cast application ------------------------------------------

void apply_active_cast(const nlohmann::json& plan,
                       const std::vector<SpeechCue>& cues,
                       Scene& scene) {
    if (!plan.contains("active_cast") || !plan["active_cast"].is_array()) {
        std::cerr << "  [cast] active_cast missing -- keeping current cast\n";
        return;
    }

    std::unordered_set<std::string> resolved_names;

    for (const auto& elem : plan["active_cast"]) {
        if (!elem.is_string()) continue;
        auto name = elem.get<std::string>();
        const Character* ch = resolve_cast_name(name, scene.characters);
        if (ch && !ch->dead)
            resolved_names.insert(to_lower(ch->name));
    }

    for (const auto& cue : cues) {
        const Character* ch = resolve_cast_name(cue.character, scene.characters);
        if (ch && !ch->dead)
            resolved_names.insert(to_lower(ch->name));
    }

    if (resolved_names.empty() && !cues.empty()) {
        std::cerr << "  [cast] active_cast resolved empty but "
                  << cues.size() << " speech cue(s) -- keeping current cast\n";
        return;
    }

    for (auto& ch : scene.characters) {
        if (ch.is_player || ch.dead) continue;
        bool in_cast = resolved_names.count(to_lower(ch.name)) > 0;
        if (ch.on_stage && !in_cast) {
            ch.on_stage = false;
            std::cerr << "  [cast] " << ch.name << " exits (not in active_cast)\n";
        } else if (!ch.on_stage && in_cast) {
            ch.on_stage = true;
            std::cerr << "  [cast] " << ch.name << " re-enters (in active_cast)\n";
        }
    }
}

// Render each on-stage character's interior INTO the narrator's context: their
// voice (dialogue_instructions + a couple example lines) and their live Thoughts
// as weighted, tension-marked chains.  This is the state the single narrator
// writes FROM; the engine never commands behavior, it only renders.  Read-only
// over the minds now (no per-turn self-state rewrite).  Returns "" if nothing to
// show (section omitted entirely).
std::string build_inner_states(Scene& scene, int turn) {
    // Gated experiments: rendered as context, never commands (off by default).
    const bool exp_surfacing = std::getenv("RHAPSODE_EXP_SURFACING") != nullptr;
    const bool exp_crisis    = std::getenv("RHAPSODE_EXP_CRISIS") != nullptr;

    std::string body;
    for (const auto& ch : scene.characters) {
        if (ch.is_player || ch.dead || !ch.on_stage) continue;
        auto it = scene.character_memories.find(ch.name);
        if (it == scene.character_memories.end()) continue;
        const CharacterMemory& mem = it->second;

        // Subjects this character holds a view of: the others present (and the
        // player by description) plus itself (its dispositional self-view).
        std::vector<std::string> subjects;
        for (const auto& c : scene.characters) {
            if (c.name == ch.name || c.dead || !c.on_stage) continue;
            subjects.push_back(c.name);
            if (c.is_player && !c.description.empty())
                subjects.push_back(c.description);
        }
        subjects.push_back(ch.name);

        std::string block;
        if (!ch.dialogue_instructions.empty())
            block += "  Voice: " + ch.dialogue_instructions + "\n";
        if (!ch.example_dialogue.empty()) {
            block += "  Example lines:\n";
            int shown = 0;
            for (const auto& ex : ch.example_dialogue) {
                block += "    - " + ex + "\n";
                if (++shown >= 2) break;
            }
        }
        std::string thoughts = mem.render_thoughts(subjects);
        if (!thoughts.empty()) {
            block += "  Interior (live thoughts; weight = how much it presses, "
                     "tension = a contradiction held unresolved):\n";
            block += thoughts;
        }

        if (exp_surfacing) {
            const unsigned seed =
                static_cast<unsigned>(std::hash<std::string>{}(ch.name)) ^
                static_cast<unsigned>(turn);
            std::string p = mem.pressing_thought(seed);
            if (!p.empty())
                block += "  Pressing today: " + p + "\n";
        }
        if (exp_crisis) {
            std::string c = mem.charge_state();
            if (!c.empty())
                block += "  Charge: " + c + "\n";
        }

        if (block.empty()) continue;
        body += "- " + ch.name + ":\n" + block;
    }
    if (body.empty()) return {};

    return "### Inner lives\n"
           "(Each character's voice and the interior you are writing FROM. These "
           "are not commands -- perform them. A high-weight thought in tension "
           "pulls toward subtext, a slip, or crisis; a high-weight thought "
           "standing alone pulls toward action. You decide what surfaces.)\n"
           + body;
}

// Route the turn's new facts into the minds that perceive them.  An explicit
// `audience` restricts perception to the named characters; an empty audience is
// a public beat, perceived by everyone present (on-stage, alive, non-player).
// The narrator is the perceivability oracle -- nothing reaches a mind unless it
// is routed here.
void route_perception(Scene& scene, const std::vector<Node>& new_nodes, int turn) {
    int deliveries = 0;
    std::unordered_set<std::string> minds;
    auto route_to = [&](const std::string& name, const Node& n) {
        auto it = scene.character_memories.find(name);
        if (it != scene.character_memories.end()) {
            it->second.route_fact(n.fact, n.entities, turn);
            ++deliveries;
            minds.insert(it->first);
        }
    };
    for (const auto& n : new_nodes) {
        if (!n.audience.empty()) {
            for (const auto& a : n.audience) route_to(a, n);
        } else {
            for (const auto& c : scene.characters) {
                if (c.is_player || c.dead || !c.on_stage) continue;
                route_to(c.name, n);
            }
        }
    }
    std::cerr << "  [perceive] " << new_nodes.size() << " new_node(s) -> "
              << deliveries << " perception(s) routed to " << minds.size()
              << " mind(s)\n" << std::flush;
}

SceneMessage make_message(const std::string& kind,
                          std::string content,
                          const std::string& speaker = {}) {
    SceneMessage msg;
    msg.role     = Role::Assistant;
    msg.content  = std::move(content);
    msg.metadata = {{"scene_kind", kind}};
    if (!speaker.empty())
        msg.metadata["speaker"] = speaker;
    return msg;
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

void SceneLoop::set_prompt_callback(PromptCallback cb)         { prompt_cb_       = std::move(cb); }
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
            std::cerr << "  [bg] background work failed: " << e.what() << "\n";
        }
    }

    last_weave_result_ = std::move(bg_weave_result_);
    bg_weave_result_ = {};
    completed_expiry_ops_ = std::move(bg_expiry_ops_);
    bg_expiry_ops_.clear();

    if (!last_weave_result_.connected.empty()
        || !last_weave_result_.disconnected.empty()
        || !last_weave_result_.reweighted.empty()) {
        std::cerr << "  [weave] +"
                  << last_weave_result_.connected.size()
                  << " -" << last_weave_result_.disconnected.size()
                  << " ~" << last_weave_result_.reweighted.size() << "\n";
    }
}

std::vector<ExpiryOp> SceneLoop::take_completed_expiry_ops() {
    return std::exchange(completed_expiry_ops_, {});
}

void SceneLoop::dispatch_background() {
    std::string ctx = build_scene_context();
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
                std::cerr << "  [bg] full graph weave (cloud)...\n" << std::flush;
                bg_weave_result_ = weaver_->weave(turn, ctx);
            } else {
                std::cerr << "  [bg] quick graph weave (local)...\n" << std::flush;
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
                std::cerr << "  [downsampler] summarized_up_to " << before
                          << " -> " << after << "\n";
                auto rendered = scene_->downsampler.render();
                if (!rendered.empty())
                    std::cerr << "  [downsampler] story_so_far (" << rendered.size()
                              << " chars): " << rendered.substr(0, 200)
                              << (rendered.size() > 200 ? "..." : "") << "\n";
            } catch (const std::exception& e) {
                std::cerr << "  [downsampler] process_turn failed: " << e.what() << "\n";
            }
        }
    });
}

// -- SceneLoop -- private helpers -------------------------------------

void SceneLoop::emit_output(SceneMessage msg) {
    scene_->history.append(std::move(msg));
    last_turn_outputs_.push_back(scene_->history.messages().back());
    if (turn_complete_cb_)
        turn_complete_cb_(scene_->history.messages().back());
}

std::string SceneLoop::build_scene_context() const {
    std::string ctx = scene_->title;
    for (const auto& msg : scene_->history.snapshot(4)) {
        ctx += '\n';
        ctx += (msg.role == Role::User ? "user: " : "assistant: ");
        ctx += truncate(msg.content, 300);
    }
    return ctx;
}

// -- SceneLoop::advance -- the four-phase turn pipeline --------------

void SceneLoop::advance() {
    if (!prompt_cb_) throw std::runtime_error("No prompt callback registered");
    if (!llm_cb_)    throw std::runtime_error("No LLM callback registered");

    // --- Turn setup -----------------------------------------------------
    join_background();

    const int turn = scene_->turn_index;
    std::cerr << "\n====== Turn " << turn << " ======\n";
    last_turn_outputs_.clear();

    // --- Phase 1: Build merged Director+Narrator prompt -----------------
    std::string system_msg;
    std::string user_msg;
    {
        state_ = LoopState::BuildingPrompt;
        std::cerr << "[1/4] Building merged prompt...\n" << std::flush;

        const std::string scene_ctx = build_scene_context();
        const std::string focus_text = director_
            ? director_->focus_payload_text(turn, scene_ctx) : "";
        const std::string inner_states = build_inner_states(*scene_, turn);

        const size_t win = resuming_ ? resume_window_size_ : window_size_;
        const auto history = scene_->history.snapshot(win);
        resuming_ = false;

        std::tie(system_msg, user_msg) =
            prompt_cb_(history, *scene_, last_director_out_, focus_text, inner_states);
        ++scene_->turn_index;

        std::cerr << "  [prompt] system=" << system_msg.size()
                  << " user=" << user_msg.size() << " chars\n" << std::flush;
        if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
            std::cerr << "--- NARRATOR SYSTEM ---\n" << system_msg << "\n"
                      << "--- NARRATOR USER ---\n" << user_msg << "\n"
                      << "--- END NARRATOR PROMPT ---\n" << std::flush;
        }
    }

    // --- Phase 2+3: Call narrative LLM + apply (retry on rejection) -----
    std::string prose;
    nlohmann::json plan;
    std::vector<SpeechCue> cues;
    {
        const auto call_narrator = [&](const std::string& sys, const std::string& usr) -> std::string {
            if (narrator_llm_cb_)
                return narrator_llm_cb_(sys, usr);
            return llm_cb_(sys + "\n\n" + usr);
        };

        {
            state_ = LoopState::RunningLLM;
            std::cerr << "[2/4] Calling narrative LLM...\n" << std::flush;

            auto raw_response = call_narrator(system_msg, user_msg);
            std::cerr << "  [narrator] response=" << raw_response.size() << " chars\n" << std::flush;
            if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
                std::cerr << "--- NARRATOR RESPONSE ---\n" << raw_response
                          << "\n--- END NARRATOR RESPONSE ---\n" << std::flush;
            }

            std::tie(prose, plan) = split_merged_response(std::move(raw_response));
        }

        {
            state_ = LoopState::AppendingResult;
            std::cerr << "[3/4] Applying graph...\n" << std::flush;

            constexpr int kMaxAttempts = 3;
            std::vector<Rejection> all_rejections;
            nlohmann::json graph_snapshot;
            if (director_)
                graph_snapshot = scene_->world_graph.to_json();

            for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
                if (attempt > 0) {
                    if (director_)
                        scene_->world_graph = WorldGraph::from_json(graph_snapshot);

                    // Undo characters dynamically added by the previous attempt
                    auto& chars = scene_->characters;
                    chars.erase(
                        std::remove_if(chars.begin(), chars.end(),
                            [turn](const Character& c) { return c.created_at >= turn; }),
                        chars.end());

                    std::string rewrite_user = user_msg;
                    rewrite_user += "\n\n### REVISION REQUIRED\n"
                                    "The following issues were found in your plan:\n";
                    for (const auto& r : all_rejections)
                        rewrite_user += "- " + r.fact + " -- " + r.reason + "\n";
                    rewrite_user += "\nRewrite your narrative and plan to fix these issues.\n";

                    state_ = LoopState::RunningLLM;
                    auto [new_prose, new_plan] = split_merged_response(
                        call_narrator(system_msg, rewrite_user));
                    prose = std::move(new_prose);
                    plan  = std::move(new_plan);
                    state_ = LoopState::AppendingResult;
                }

                last_director_out_ = {};
                if (director_)
                    last_director_out_ = director_->apply_planned_turn(turn, plan);

                // Register new characters BEFORE validating active_cast,
                // so freshly introduced NPCs are recognized as valid cast.
                if (plan.contains("new_characters") && plan["new_characters"].is_array()) {
                    for (const auto& ch_j : plan["new_characters"]) {
                        Character ch;
                        ch.name = ch_j.value("name", "");
                        ch.description = ch_j.value("description", "");
                        ch.dialogue_instructions = ch_j.value("dialogue_instructions", "");
                        ch.role = ch_j.value("role", "minor_npc");
                        ch.on_stage = true;
                        ch.created_at = scene_->turn_index;
                        if (!ch.name.empty() && !scene_->find_on_stage(ch.name)) {
                            std::cerr << "  [enter] " << ch.name << " enters the stage\n";
                            scene_->enter_character(std::move(ch));
                        }
                    }
                }

                const auto cast_rejections = validate_active_cast(plan, *scene_);
                all_rejections = last_director_out_.rejections;
                all_rejections.insert(all_rejections.end(),
                                      cast_rejections.begin(), cast_rejections.end());

                if (all_rejections.empty())
                    break;

                std::cerr << "  [retry] attempt " << (attempt + 1) << "/" << kMaxAttempts
                          << ": " << all_rejections.size() << " issue(s)\n";
                for (const auto& r : all_rejections)
                    std::cerr << "    - " << r.fact << " -- " << r.reason << "\n";
                std::cerr << std::flush;
            }

            cues = extract_speech_cues(plan);
        }
    }

    // --- Apply active_cast ----------------------------------------------
    {
        apply_active_cast(plan, cues, *scene_);
    }

    // --- Emit narrator + route perception -------------------------------
    const std::string narration = prose;
    {
        emit_output(make_message("narrator", std::move(prose)));
        route_perception(*scene_, last_director_out_.new_nodes, turn);
    }

    // --- Phase 4: Emit authored dialogue --------------------------------
    {
        std::cerr << "[4/4] Emit authored dialogue...\n" << std::flush;

        for (const auto& cue : cues) {
            const auto* ch = resolve_cast_name(cue.character, scene_->characters);

            std::string spoken = trim(cue.field("line"));
            std::string action = trim(cue.field("action"));
            if (!action.empty())
                spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");

            if (spoken.empty())
                spoken = "(" + cue.character + " is at a loss for words.)";

            // Distill the authored line into the speaker's own interior so it
            // carries its own words; background reflection relates and re-weights it.
            if (ch) {
                auto mem_it = scene_->character_memories.find(ch->name);
                if (mem_it != scene_->character_memories.end())
                    mem_it->second.route_fact(spoken, {ch->name}, turn);
            }

            emit_output(make_message("character", std::move(spoken), cue.character));
        }
    }

    // --- Post-turn cleanup ----------------------------------------------
    {
        const auto death_candidates = scene_->scan_death_candidates();
        if (!death_candidates.empty())
            confirm_deaths(death_candidates, narration);

        if (weaver_) {
            std::vector<std::string> prio;
            for (const auto& n : last_director_out_.new_nodes)
                for (const auto& e : n.entities)
                    prio.push_back(e);
            weaver_->rebuild_expiry_queue(prio);
        }

        if (!saves_dir_.empty())
            scene_->save(saves_dir_);

        dispatch_background();
    }

    std::cerr << "====== Turn " << turn << " done ======\n" << std::flush;
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
            std::string lower;
            lower.reserve(response.size());
            for (auto c : response)
                lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            if (lower.find("yes") != std::string::npos) {
                for (auto& ch : scene_->characters) {
                    if (ch.name == dc.character_name) {
                        ch.dead = true;
                        ch.on_stage = false;
                        std::cerr << "  [dead] " << ch.name
                                  << " (confirmed by LLM)\n";
                        break;
                    }
                }
            } else {
                std::cerr << "  [death-scan] " << dc.character_name
                          << " -- keyword match but LLM says alive\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "  [death-scan] LLM confirmation failed for "
                      << dc.character_name << ": " << e.what()
                      << " -- skipping (fail-safe)\n";
        }
    }
}

}  // namespace rhapsode
