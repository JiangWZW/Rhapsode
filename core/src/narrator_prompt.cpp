#include "rhapsode/narrator_prompt.h"

#include "rhapsode/json_util.h"
#include "rhapsode/scene.h"

namespace rhapsode {
namespace {

constexpr size_t kVerbatimTail = 6;
constexpr size_t kMaxMessageChars = 400;
constexpr size_t kMaxStoryChars = 1500;

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

}  // namespace

std::string build_narrator_turn_state(const std::vector<SceneMessage>& history,
                                      const Scene& scene) {
    std::vector<std::string> parts;

    const auto cast_lines = scene.build_prompt__cast();
    if (!cast_lines.empty()) {
        parts.push_back("### Cast");
        for (const auto& line : cast_lines) {
            parts.push_back(line);
        }
    }

    const std::string story_so_far = scene.downsampler.render();
    if (!story_so_far.empty()) {
        append_part(parts, "");
        parts.push_back("### Story so far");
        parts.push_back(truncate_utf8(story_so_far, kMaxStoryChars));
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
Begin your response directly with the first paragraph of prose. Do NOT write any
preamble, thinking, commentary, or meta-text before the prose -- not even a single
sentence like "Now I have the context" or "Let me write the response."

TOOLS: You have tools to query the world state before writing. Use them proactively:
- query_graph(entity): trace an entity's timeline -- where they are, what happened to them, what's expired. Query key entities (Player, present NPCs) before narrating to avoid continuity errors.
- query_mind(character): understand what a character is thinking/feeling before writing their speech.
- query_history(query): recall specific past events when you need continuity.
Query first, then write. Do NOT guess when you can query. A node with valid_until=-1 is still true; valid_until=N was true until turn N.

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
