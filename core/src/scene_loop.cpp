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
#include <optional>
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

constexpr size_t kGraphSeedMessages = 4;

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
        out += cap_per_msg ? truncate(msg.content, *cap_per_msg) : msg.content;
    }
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
    const auto history = scene_->history.snapshot(window_size_);
    const std::string ctx = format_graph_seed(history, scene_->title, 300);
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

void SceneLoop::emit_output(SceneMessage msg, OutputBucket bucket) {
    History& target = bucket == OutputBucket::Story ? scene_->history : scene_->dialogue;
    target.append(std::move(msg));
    last_turn_outputs_.push_back(target.messages().back());
    if (turn_complete_cb_)
        turn_complete_cb_(target.messages().back());
}

namespace {

constexpr size_t kVerbatimTail   = 6;
constexpr size_t kMaxMsgChars    = 400;
constexpr size_t kMaxFacts       = 8;
constexpr size_t kMaxStoryChars  = 1500;

const char* role_name(Role role) {
    switch (role) {
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        case Role::System:    return "system";
    }
    return "user";
}

void append_part(std::vector<std::string>& parts, const std::string& text) {
    if (!text.empty()) parts.push_back(text);
}

std::string build_memory_query(const std::vector<SceneMessage>& history,
                               const Scene& scene,
                               const DirectorOutput& director_out) {
    std::string query = scene.title;
    const size_t start = history.size() > 4 ? history.size() - 4 : 0;
    for (size_t i = start; i < history.size(); ++i) {
        query += '\n';
        query += role_name(history[i].role);
        query += ": ";
        query += history[i].content;
    }
    for (const auto& block : director_out.context_blocks)
        query += '\n' + block;
    return query;
}

std::vector<std::string> extract_entity_queries(const std::vector<SceneMessage>& history,
                                                const Scene& scene) {
    std::string last_user;
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        if (it->role == Role::User) {
            last_user = it->content;
            break;
        }
    }
    if (last_user.empty()) return {};

    const std::string text_lower = to_lower(last_user);
    std::vector<std::string> queries;

    for (const auto& c : scene.characters) {
        if (!c.name.empty() && text_lower.find(to_lower(c.name)) != std::string::npos)
            queries.push_back(c.name);
    }

    std::unordered_set<std::string> seen;
    for (const auto& node : scene.world_graph.all_nodes(true)) {
        for (const auto& ent : node.entities) {
            const std::string ent_lower = to_lower(ent);
            if (seen.count(ent_lower)) continue;
            if (text_lower.find(ent_lower) != std::string::npos) {
                seen.insert(ent_lower);
                queries.push_back(ent);
            }
        }
    }
    return queries;
}

std::vector<std::string> build_established_facts(
    MemorySystem* memory,
    const std::vector<SceneMessage>& history,
    const Scene& scene,
    const DirectorOutput& director_out) {
    if (!memory) return {};

    try {
        std::unordered_set<std::uint64_t> seen_ids;
        std::vector<std::string> facts;

        auto collect = [&](const std::vector<std::uint64_t>& ids) {
            for (auto nid : ids) {
                if (!seen_ids.insert(nid).second) continue;
                const Node* node = scene.world_graph.get_node(nid);
                if (node && !node->fact.empty() && node->valid_until != -1)
                    facts.push_back(node->fact);
            }
        };

        collect(memory->search_nodes(build_memory_query(history, scene, director_out), 6));
        for (const auto& entity_q : extract_entity_queries(history, scene))
            collect(memory->search_nodes(entity_q, 4));

        if (facts.size() > kMaxFacts)
            facts.resize(kMaxFacts);
        return facts;
    } catch (const std::exception& e) {
        std::cerr << "  [prompt] established facts retrieval failed: " << e.what() << "\n";
        return {};
    }
}

std::string build_prompt__narrator_turn_state(
    const std::vector<SceneMessage>& history,
    const Scene& scene,
    const DirectorOutput& director_out,
    MemorySystem* memory,
    const std::string& world_graph_context,
    const std::string& inner_lives) {
    std::vector<std::string> parts;

    const auto cast_lines = scene.build_prompt__cast();
    if (!cast_lines.empty()) {
        parts.push_back("### Cast");
        for (const auto& line : cast_lines)
            parts.push_back(line);
    }

    if (!world_graph_context.empty()) {
        append_part(parts, "");
        parts.push_back("### Graph");
        parts.push_back(world_graph_context);
    }

    const auto established = build_established_facts(memory, history, scene, director_out);
    if (!established.empty()) {
        append_part(parts, "");
        parts.push_back("### Past");
        for (const auto& fact : established)
            parts.push_back("- " + fact);
    }

    const std::string story_so_far = scene.downsampler.render();
    if (!story_so_far.empty()) {
        append_part(parts, "");
        parts.push_back("### Story so far");
        parts.push_back(truncate(story_so_far, kMaxStoryChars));
    }

    if (!inner_lives.empty()) {
        append_part(parts, "");
        std::string block = inner_lives;
        while (!block.empty() && (block.back() == '\n' || block.back() == '\r'))
            block.pop_back();
        parts.push_back(block);
    }

    const std::vector<SceneMessage> conv =
        story_so_far.empty()
            ? history
            : [&]() {
                  const size_t start = history.size() > kVerbatimTail
                                           ? history.size() - kVerbatimTail
                                           : 0;
                  return std::vector<SceneMessage>(history.begin() + start, history.end());
              }();

    append_part(parts, "");
    parts.push_back("### Turn transcript");
    for (const auto& msg : conv) {
        parts.push_back(std::string(role_name(msg.role)) + ": "
                        + truncate(msg.content, kMaxMsgChars));
    }

    append_part(parts, "");
    parts.push_back("### Remember");
    parts.push_back(
        "- Prose is narration only: never a character's spoken words or *actions* -- "
        "each character's words go in speech_turns.line, written in their own voice.");
    parts.push_back(
        "- Give a speech_turn only to a character who can speak right now "
        "(not asleep, unconscious, incapacitated, dead, or absent).");

    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += '\n';
        out += parts[i];
    }
    return out;
}

std::string build_prompt__narrator_instructions() {
    return R"RHAPSODE(OUTPUT: 2-4 paragraphs of second-person present-tense prose, sensory grounding.
PROSE IS NARRATION ONLY -- describe what happens; never write what anyone says.
No quotation marks, no character dialogue, and no *asterisks* / stage directions in the
prose. Each character's spoken words go in speech_turns.line, written verbatim in that
character's own voice; writing speech in the prose duplicates it and breaks the display.
No markdown formatting in prose.

Then output the sentinel line verbatim on its own:
<<<RHAPSODE_JSON>>>
Then raw JSON (no fences). Use ONLY straight ASCII double quotes (") for all keys
and strings -- never smart/curly quotes (" " ' '), or the JSON will not parse:
{"transitions":[{"id":<node_id>,"state":"dormant|foreshadowed|active|resolved"}],
 "new_nodes":[{"fact":"<=15 words, atomic","type":"plot|scene|world|relationship","state":"dormant|foreshadowed|active|resolved","foreshadow_ctx":"...","active_ctx":"...","entities":[],"audience":[]}],
 "speech_turns":[{"character":"Name","line":"the actual words spoken, verbatim, in this character's voice","action":"brief stage action, optional"}],
 "new_characters":[{"name":"...","description":"2 sentences","dialogue_instructions":"1 sentence"}],
 "active_cast":["present NPC names"]}

RULES:
- Never narrate unperformed Player actions. May foreshadow options.
- Facts: each one atomic proposition, <=15 words. Emit a new_node for EVERY development the turn introduces -- a state change, a revealed intention, a threat, a death, a relationship shift, a thing learned -- not for sensory description or mood. Do NOT drop a real development to stay under a count; capture them all (typically 3-6, more on eventful turns).
- entities: the canonical subject(s) this fact is about. Use the EXACT name from the Cast for any NPC -- never a title, synonym, or description ("Warden Elara Voss", not "the warden"). For the player character (the "you" of the narration), ALWAYS use "Player". Coin a new string only for a genuinely new, unnamed thing/place/faction with no Cast entry; once you name it, reuse that exact string every time. This is how a fact reaches the right character's memory -- inconsistent names splinter one subject into several.
- audience: which characters perceive this fact (by name). Omit/[] for a public beat everyone present perceives. Name a narrow audience only when something is private -- a fact only one character learns or witnesses. This decides who knows what; the unlisted stay ignorant. (This never means writing dialogue in the prose.)
- speech_turns: one entry per NPC who speaks this turn; `line` is their exact words in their own voice, `action` an optional brief stage action. [] if ambience-only. Only characters who CAN speak right now -- never one who is asleep, unconscious, incapacitated, dead, or no longer present. If your prose just put someone to sleep or under, they get no speech_turn.
- new_characters: first-time speaking NPCs only. [] if none introduced.
- active_cast: all living NPCs physically present this scene. [] if player alone.)RHAPSODE";
}

}  // namespace

// -- SceneLoop::advance -- the four-phase turn pipeline --------------

void SceneLoop::advance() {
    if (!llm_cb_)    throw std::runtime_error("No LLM callback registered");
    if (!director_)  throw std::runtime_error("Null director in scene");

    // --- Turn setup -----------------------------------------------------
    join_background();

    const int turn = scene_->turn_index;
    std::cerr << "\n====== Turn " << turn << " ======\n";
    last_turn_outputs_.clear();

    // --- Phase 1: Build merged Director+Narrator prompt -----------------
    NarratorPrompt narrator_prompt;
    {
        state_ = LoopState::BuildingPrompt;
        std::cerr << "[1/4] Building merged prompt...\n" << std::flush;

        const size_t win = resuming_ ? resume_window_size_ : window_size_;
        const std::vector<SceneMessage> history = scene_->history.snapshot(win);
        const std::string graph_seeds = format_graph_seed(history, scene_->title); 
        resuming_ = false;

        narrator_prompt.instructions = build_prompt__narrator_instructions();
        narrator_prompt.turn_state   = build_prompt__narrator_turn_state(
            history, *scene_, last_director_out_, scene_->memory(),
            director_->build_prompt__world_graph_context(turn, graph_seeds),
            scene_->build_prompt__inner_lives(turn));

    	++scene_->turn_index;

        std::cerr << "  [prompt] instructions=" << narrator_prompt.instructions.size()
                  << " turn_state=" << narrator_prompt.turn_state.size() << " chars\n" << std::flush;
        if (std::getenv("RHAPSODE_VERBOSE_LOG")) {
            std::cerr << "--- NARRATOR INSTRUCTIONS ---\n" << narrator_prompt.instructions << "\n"
                      << "--- NARRATOR TURN STATE ---\n" << narrator_prompt.turn_state << "\n"
                      << "--- END NARRATOR PROMPT ---\n" << std::flush;
        }
    }

    // --- Phase 2+3: Call narrative LLM + apply (retry on rejection) -----
    std::string prose;
    nlohmann::json plan;
    std::vector<SpeechCue> cues;
    {
        const auto call_narrator = [&](const std::string& instructions,
                                       const std::string& turn_state) -> std::string {
            if (narrator_llm_cb_)
                return narrator_llm_cb_(instructions, turn_state);
            return llm_cb_(instructions + "\n\n" + turn_state);
        };

        {
            state_ = LoopState::RunningLLM;
            std::cerr << "[2/4] Calling narrative LLM...\n" << std::flush;

            auto raw_response = call_narrator(narrator_prompt.instructions, narrator_prompt.turn_state);
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

                    std::string rewrite_turn_state = narrator_prompt.turn_state;
                    rewrite_turn_state += "\n\n### REVISION REQUIRED\n"
                                           "The following issues were found in your plan:\n";
                    for (const auto& r : all_rejections)
                        rewrite_turn_state += "- " + r.fact + " -- " + r.reason + "\n";
                    rewrite_turn_state += "\nRewrite your narrative and plan to fix these issues.\n";

                    state_ = LoopState::RunningLLM;
                    auto [new_prose, new_plan] = split_merged_response(
                        call_narrator(narrator_prompt.instructions, rewrite_turn_state));
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
        emit_output(make_message("narrator", std::move(prose)), OutputBucket::Story);
        route_perception(*scene_, last_director_out_.new_nodes, turn);
    }

    // --- Phase 4: Emit authored dialogue --------------------------------
    {
        std::cerr << "[4/4] Emit authored dialogue...\n" << std::flush;

        for (const auto& cue : cues) {
            std::string spoken = trim(cue.field("line"));
            std::string action = trim(cue.field("action"));
            if (!action.empty())
                spoken += (spoken.empty() ? "" : " ") + ("(" + action + ")");

            if (spoken.empty())
                spoken = "(" + cue.character + " is at a loss for words.)";

            // Dialogue is UI-only (dialogue log). Subjective minds learn beats via
            // route_perception(new_nodes), not by re-ingesting the speaker's line.

            auto msg = make_message("character", std::move(spoken), cue.character);
            msg.metadata["turn"] = turn;
            emit_output(std::move(msg), OutputBucket::Dialogue);
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
