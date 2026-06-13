#include "rhapsode/scene_loop.h"
#include "rhapsode/scene.h"
#include "rhapsode/character_memory.h"
#include "rhapsode/director.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/json_util.h"

#include <algorithm>
#include <cctype>
#include <cstring>
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

/// Append a markdown section.  No-op if body is empty.
void section(std::string& out, const char* heading, const std::string& body) {
    if (body.empty()) return;
    out += "\n### ";
    out += heading;
    out += '\n';
    out += body;
    out += '\n';
}

/// Append a tagged line.  No-op if value is empty.
void tagged_line(std::string& out, const char* tag, const std::string& value) {
    if (value.empty()) return;
    out += tag;
    out += value;
    out += '\n';
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

constexpr char kJsonMarker[] = "\n<<<RHAPSODE_JSON>>>\n";

std::pair<std::string, nlohmann::json> split_merged_response(std::string raw) {
    auto marker = raw.find(kJsonMarker);
    if (marker != std::string::npos) {
        auto prose = trim(raw.substr(0, marker));
        auto json  = trim(raw.substr(marker + std::strlen(kJsonMarker)));
        return {std::move(prose), json.empty() ? nlohmann::json::object()
                                               : try_parse_json(json)};
    }

    // Fallback: find last top-level JSON object.
    auto brace = raw.rfind('{');
    if (brace != std::string::npos) {
        std::string fragment;
        if (extract_balanced_json(std::string_view(raw).substr(brace), fragment)) {
            try {
                auto plan = nlohmann::json::parse(fragment);
                return {trim(raw.substr(0, brace)), std::move(plan)};
            } catch (...) {}
        }
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

// -- Actor prompt assembly --------------------------------------------

std::string format_history(const Scene& scene, size_t limit = 8) {
    auto msgs = scene.history.snapshot(limit);
    std::string out;
    for (const auto& msg : msgs) {
        const auto& md = msg.metadata;
        auto kind    = md.value("scene_kind", "");
        auto speaker = md.value("speaker", "");

        if (msg.role == Role::User)
            out += "Player: ";
        else if (kind == "character" && !speaker.empty())
            out += speaker + ": ";

        out += truncate(msg.content, 400);
        out += '\n';
    }
    return out;
}

// Advance each on-stage character's persistent self-state and collect the
// first-person states into a block for the decision (narrator) prompt.  Mutates
// the CharacterMemory (update_self_state folds the state forward), so it takes
// a non-const Scene&.  Returns "" if no character has a self-state (section
// omitted entirely).
std::string build_inner_states(Scene& scene, int turn) {
    std::string body;
    for (const auto& ch : scene.characters) {
        if (ch.is_player || ch.dead || !ch.on_stage) continue;
        auto it = scene.character_memories.find(ch.name);
        if (it == scene.character_memories.end()) continue;

        CharacterMemory& mem = it->second;
        mem.update_self_state(turn);
        const std::string& state = mem.self_state();
        if (state.empty()) continue;

        body += "- " + ch.name + ": " + truncate(state, 400) + "\n";
    }
    if (body.empty()) return {};

    return "### Inner states\n"
           "(Each is the character's own first-person state of mind; make their "
           "speech_turns emotional_state and dramatic_intent consistent with it.)\n"
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

std::string build_actor_prompt(const Character& character,
                               const SpeechCue&  cue,
                               const std::string& narration,
                               const Scene&       scene,
                               CharacterMemory* char_mem = nullptr) {
    std::string prompt;
    prompt.reserve(2048);

    prompt += "You are **" + character.name
           +  "** in \"" + scene.title + "\".\n";

    // -- Identity --
    section(prompt, "Character",    character.description);
    section(prompt, "Voice & style", character.dialogue_instructions);

    if (!character.example_dialogue.empty()) {
        std::string lines;
        for (const auto& ex : character.example_dialogue)
            lines += "- " + character.name + ": " + ex + "\n";
        section(prompt, "Example lines", lines);
    }

    // -- Scene --
    std::string others;
    for (const auto& c : scene.characters)
        if (c.name != character.name && c.on_stage && !c.dead && !c.is_player) {
            if (!others.empty()) others += ", ";
            others += c.name;
        }
    section(prompt, "Others present", others);

    // -- Knowledge: ONLY this character's own mind.  Never the narrator's
    //    omniscient world graph -- the actor cannot read what it hasn't been
    //    told or perceived.
    if (char_mem) {
        // Who I am right now (carried across turns), first person.
        section(prompt, "Inner state", char_mem->self_state());

        // What I believe about the people in front of me -- drawn from my own
        // subjective belief graph, keyed on who is present.
        std::vector<std::string> subjects;
        for (const auto& c : scene.characters) {
            if (c.name == character.name || c.dead || !c.on_stage) continue;
            subjects.push_back(c.name);
            if (c.is_player && !c.description.empty())
                subjects.push_back(c.description);
        }
        std::string views = char_mem->view_of(subjects);
        if (!views.empty())
            section(prompt, "What I know about who's here", views);
    }

    section(prompt, "Recent events", format_history(scene));

    // -- Direction --
    section(prompt, "Current narrator beat", truncate(narration, 600));

    std::string stage = cue.field("cue");
    tagged_line(stage, "\nDramatic intent: ",    cue.field("dramatic_intent"));
    tagged_line(stage, "Your emotional state: ", cue.field("emotional_state"));
    tagged_line(stage, "Responding to: ",        cue.field("responds_to"));
    section(prompt, "Stage direction", stage);

    // -- Task --
    prompt += "\n### Task\n"
              "Write ONLY what " + character.name + " says. Wrap every spoken line in "
              "straight double quotes, e.g. \"We hold the gate till dawn.\"\n"
              "Put brief actions in (parentheses), kept minimal. Do NOT use *asterisks*.\n"
              "Do NOT narrate the scene, describe surroundings, or write other characters' lines.\n"
              "Stay faithful to your character's voice and emotional state.\n";

    return prompt;
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
void SceneLoop::set_actor_llm_callback(LLMCallback cb)         { actor_llm_cb_    = std::move(cb); }
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

    // -- Join previous background work (weave + expiry + reflections) --
    join_background();

    const int turn = scene_->turn_index;
    std::cerr << "\n====== Turn " << turn << " ======\n";
    last_turn_outputs_.clear();

    // -- Phase 1: Build merged Director+Narrator prompt --

    state_ = LoopState::BuildingPrompt;
    std::cerr << "[1/4] Building merged prompt...\n" << std::flush;

    std::string scene_ctx  = build_scene_context();
    std::string focus_text = director_
        ? director_->focus_payload_text(turn, scene_ctx) : "";

    // Advance + collect each on-stage character's persistent first-person
    // self-state, using the same scene context as "what just happened".
    std::string inner_states = build_inner_states(*scene_, turn);

    size_t win   = resuming_ ? resume_window_size_ : window_size_;
    auto history = scene_->history.snapshot(win);
    resuming_    = false;

    auto [system_msg, user_msg] =
        prompt_cb_(history, *scene_, last_director_out_, focus_text, inner_states);
    ++scene_->turn_index;

    std::cerr << "  [prompt] system=" << system_msg.size()
              << " user=" << user_msg.size() << " chars\n" << std::flush;
    if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
        std::cerr << "--- NARRATOR SYSTEM ---\n" << system_msg << "\n"
                  << "--- NARRATOR USER ---\n" << user_msg << "\n"
                  << "--- END NARRATOR PROMPT ---\n" << std::flush;
    }

    // -- Phase 2+3: Call narrative LLM + apply (with retry on contradiction) --

    auto call_narrator = [&](const std::string& sys, const std::string& usr) -> std::string {
        if (narrator_llm_cb_)
            return narrator_llm_cb_(sys, usr);
        return llm_cb_(sys + "\n\n" + usr);
    };

    state_ = LoopState::RunningLLM;
    std::cerr << "[2/4] Calling narrative LLM...\n" << std::flush;

    auto raw_response = call_narrator(system_msg, user_msg);
    std::cerr << "  [narrator] response=" << raw_response.size() << " chars\n" << std::flush;
    if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
        std::cerr << "--- NARRATOR RESPONSE ---\n" << raw_response
                  << "\n--- END NARRATOR RESPONSE ---\n" << std::flush;
    }

    auto [prose, plan] = split_merged_response(std::move(raw_response));

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
            plan = std::move(new_plan);
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

        auto cast_rejections = validate_active_cast(plan, *scene_);
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

    auto cues = extract_speech_cues(plan);

    // -- Apply active_cast (exits absent, re-enters returning) --
    apply_active_cast(plan, cues, *scene_);

    const std::string narration = prose;
    emit_output(make_message("narrator", std::move(prose)));

    // -- Route this turn's facts into the minds that perceive them.
    //    Reflection (perception -> belief) runs later in the background. --
    route_perception(*scene_, last_director_out_.new_nodes, turn);

    // -- Phase 4: Actor synthesis --

    std::cerr << "[4/4] Actor synthesis...\n" << std::flush;
    const auto& actor_llm = actor_llm_cb_ ? actor_llm_cb_ : llm_cb_;

    for (const auto& cue : cues) {
        const auto* ch = resolve_cast_name(cue.character, scene_->characters);
        std::string spoken;

        if (ch) {
            CharacterMemory* char_mem = nullptr;
            auto mem_it = scene_->character_memories.find(ch->name);
            if (mem_it != scene_->character_memories.end())
                char_mem = &mem_it->second;

            auto actor_prompt = build_actor_prompt(*ch, cue, narration, *scene_, char_mem);
            std::cerr << "  [actor] " << ch->name << " prompt=" << actor_prompt.size() << " chars\n" << std::flush;
            if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
                std::cerr << "--- ACTOR PROMPT: " << ch->name << " ---\n"
                          << actor_prompt << "\n--- END ACTOR PROMPT ---\n" << std::flush;
            }
            try {
                spoken = trim(actor_llm(actor_prompt));
                std::cerr << "  [actor] " << ch->name << " response=" << spoken.size() << " chars\n" << std::flush;
                if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
                    std::cerr << "--- ACTOR RESPONSE: " << ch->name << " ---\n"
                              << spoken << "\n--- END ACTOR RESPONSE ---\n" << std::flush;
                }
            } catch (const std::exception& e) {
                std::cerr << "  [actor] FAILED " << ch->name << ": " << e.what() << "\n";
            }

            // A character's mind is no longer written here: it learns only
            // through narrator-routed perception (route_perception above), which
            // reflection folds into belief in the background.
        }

        if (spoken.empty())
            spoken = "(" + cue.character + " is at a loss for words.)";

        emit_output(make_message("character", std::move(spoken), cue.character));
    }

    // -- Post-turn: run downsampler on history --
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

    // -- Post-turn: death detection (keyword pre-filter + LLM confirmation) --
    auto death_candidates = scene_->scan_death_candidates();
    if (!death_candidates.empty())
        confirm_deaths(death_candidates, narration);

    // -- Post-turn: prepare expiry queue for background drain --
    if (weaver_) {
        std::vector<std::string> prio;
        for (const auto& n : last_director_out_.new_nodes)
            for (const auto& e : n.entities)
                prio.push_back(e);
        weaver_->rebuild_expiry_queue(prio);
    }

    // -- Save while graph is still consistent (before bg mutations) --
    if (!saves_dir_.empty())
        scene_->save(saves_dir_);

    // -- Kick off background thread: weave + expiry drain + reflections --
    dispatch_background();

    std::cerr << "====== Turn " << turn << " done ======\n" << std::flush;
    state_ = LoopState::WaitingForInput;
}

void SceneLoop::confirm_deaths(const std::vector<DeathCandidate>& candidates,
                               const std::string& narration) {
    const auto& llm = actor_llm_cb_ ? actor_llm_cb_ : llm_cb_;
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
