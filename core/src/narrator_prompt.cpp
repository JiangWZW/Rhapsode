#include "rhapsode/narrator_prompt.h"

#include "rhapsode/json_util.h"
#include "rhapsode/log_util.h"
#include "rhapsode/memory_system.h"
#include "rhapsode/scene.h"
#include "rhapsode/str_util.h"

#include <cstdint>
#include <exception>
#include <unordered_set>

namespace rhapsode {
namespace {

constexpr size_t kMemoryQueryTail = 4;
constexpr size_t kVerbatimTail = 6;
constexpr size_t kMaxMessageChars = 400;
constexpr size_t kMaxEstablishedFacts = 8;
constexpr size_t kMaxStoryChars = 1500;
constexpr int kMemoryQueryHits = 6;
constexpr int kEntityQueryHits = 4;

const char* role_name(Role role) {
    switch (role) {
        case Role::User:
            return "user";
        case Role::Assistant:
            return "assistant";
        case Role::System:
            return "system";
    }
    return "user";
}

void append_part(std::vector<std::string>& parts, const std::string& text) {
    if (!text.empty()) {
        parts.push_back(text);
    }
}

std::string build_memory_query(const std::vector<SceneMessage>& history,
                               const Scene& scene,
                               const DirectorOutput& director_out) {
    std::string query = scene.title;
    const size_t start = history.size() > kMemoryQueryTail ? history.size() - kMemoryQueryTail : 0;
    for (size_t i = start; i < history.size(); ++i) {
        query += '\n';
        query += role_name(history[i].role);
        query += ": ";
        query += history[i].content;
    }
    for (const auto& block : director_out.context_blocks) {
        query += '\n' + block;
    }
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
    if (last_user.empty()) {
        return {};
    }

    const std::string text_lower = str::to_lower(last_user);
    std::vector<std::string> queries;

    for (const auto& c : scene.characters) {
        if (!c.name.empty() && text_lower.find(str::to_lower(c.name)) != std::string::npos) {
            queries.push_back(c.name);
        }
    }

    std::unordered_set<std::string> seen;
    for (const auto& node : scene.world_graph.all_nodes(true)) {
        for (const auto& ent : node.entities) {
            const std::string ent_lower = str::to_lower(ent);
            if (seen.count(ent_lower)) {
                continue;
            }
            if (text_lower.find(ent_lower) != std::string::npos) {
                seen.insert(ent_lower);
                queries.push_back(ent);
            }
        }
    }
    return queries;
}

std::vector<std::string> build_established_facts(MemorySystem* memory,
                                                 const std::vector<SceneMessage>& history,
                                                 const Scene& scene,
                                                 const DirectorOutput& director_out) {
    if (!memory) {
        return {};
    }

    try {
        std::unordered_set<std::uint64_t> seen_ids;
        std::vector<std::string> facts;

        auto collect = [&](const std::vector<std::uint64_t>& ids) {
            for (auto nid : ids) {
                if (!seen_ids.insert(nid).second) {
                    continue;
                }
                const Node* node = scene.world_graph.get_node(nid);
                if (node && !node->fact.empty() && node->valid_until != -1) {
                    facts.push_back(node->fact);
                }
            }
        };

        collect(memory->search_nodes(build_memory_query(history, scene, director_out),
                                     kMemoryQueryHits));
        for (const auto& entity_q : extract_entity_queries(history, scene)) {
            collect(memory->search_nodes(entity_q, kEntityQueryHits));
        }

        if (facts.size() > kMaxEstablishedFacts) {
            facts.resize(kMaxEstablishedFacts);
        }
        return facts;
    } catch (const std::exception& e) {
        log() << "  [prompt] established facts retrieval failed: " << e.what() << "\n";
        return {};
    }
}

}  // namespace

std::string build_narrator_turn_state(const std::vector<SceneMessage>& history,
                                      const Scene& scene,
                                      const DirectorOutput& director_out,
                                      MemorySystem* memory,
                                      const std::string& world_graph_context,
                                      const std::string& inner_lives) {
    std::vector<std::string> parts;

    const auto cast_lines = scene.build_prompt__cast();
    if (!cast_lines.empty()) {
        parts.push_back("### Cast");
        for (const auto& line : cast_lines) {
            parts.push_back(line);
        }
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
        for (const auto& fact : established) {
            parts.push_back("- " + fact);
        }
    }

    const std::string story_so_far = scene.downsampler.render();
    if (!story_so_far.empty()) {
        append_part(parts, "");
        parts.push_back("### Story so far");
        parts.push_back(truncate_utf8(story_so_far, kMaxStoryChars));
    }

    if (!inner_lives.empty()) {
        append_part(parts, "");
        parts.push_back(str::trim(inner_lives));
    }

    const std::vector<SceneMessage> conv = story_so_far.empty()
                                               ? history
                                               : [&]() {
                                                     const size_t start =
                                                         history.size() > kVerbatimTail
                                                             ? history.size() - kVerbatimTail
                                                             : 0;
                                                     return std::vector<SceneMessage>(
                                                         history.begin() + start, history.end());
                                                 }();

    append_part(parts, "");
    parts.push_back("### Turn transcript");
    for (const auto& msg : conv) {
        parts.push_back(std::string(role_name(msg.role)) + ": " +
                        truncate_utf8(msg.content, kMaxMessageChars));
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
        if (i) {
            out += '\n';
        }
        out += parts[i];
    }
    return out;
}

std::string build_narrator_instructions() {
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

}  // namespace rhapsode
