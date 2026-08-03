#include "rhapsode/narrator_prompt.h"

#include "rhapsode/json_util.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/world.h"

#include <algorithm>

namespace rhapsode {
namespace {

constexpr size_t kVerbatimTail = 6;
constexpr size_t kMaxMessageChars = 400;
constexpr size_t kMaxStoryChars = 1500;

const char* role_name(Role role) {
    switch (role) {
        case Role::User: return "user";
        case Role::Assistant: return "assistant";
        case Role::System: return "system";
    }
    return "user";
}

std::string join_sorted(std::vector<std::string> names) {
    std::sort(names.begin(), names.end());
    std::string result;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i) result += ", ";
        result += names[i];
    }
    return result;
}

}  // namespace

std::string build_narrator_turn_state(const std::vector<SceneMessage>& history,
                                      const SceneData& scene,
                                      const World& world,
                                      const std::string& live_storylines_board) {
    std::vector<std::string> parts;
    std::vector<std::string> cast_lines;
    std::vector<std::string> on_stage_names;
    std::vector<std::string> off_stage_names;
    for (const auto& character : world.characters()) {
        if (character.is_player || character.dead) continue;
        if (character.in_scene(scene.scene_id)) {
            on_stage_names.push_back(character.name);
            std::string line = "- " + character.name;
            if (!character.role.empty()) line += " [" + character.role + "]";
            const std::string description = str::trim(character.description);
            if (!description.empty()) line += " \xe2\x80\x94" + description;
            cast_lines.push_back(std::move(line));
        } else {
            off_stage_names.push_back(character.name);
        }
    }
    std::vector<std::string> cast_header;
    if (!on_stage_names.empty())
        cast_header.push_back("On-stage: " + join_sorted(on_stage_names));
    if (!off_stage_names.empty())
        cast_header.push_back("Off-stage: " + join_sorted(off_stage_names));
    cast_header.insert(cast_header.end(), cast_lines.begin(), cast_lines.end());
    if (!cast_header.empty()) {
        parts.push_back("### Cast");
        parts.insert(parts.end(), cast_header.begin(), cast_header.end());
    }

    if (!live_storylines_board.empty())
        parts.push_back(live_storylines_board);

    const std::string story_so_far =
        render_text_downsampling(scene.downsampling);
    if (!story_so_far.empty()) {
        parts.push_back("### Story so far");
        parts.push_back(truncate_utf8(story_so_far, kMaxStoryChars));
    }

    const std::vector<SceneMessage> conversation = story_so_far.empty()
        ? history
        : [&]() {
            const size_t start = history.size() > kVerbatimTail
                ? history.size() - kVerbatimTail : 0;
            return std::vector<SceneMessage>(history.begin() + start, history.end());
        }();

    parts.push_back("### Turn transcript");
    for (const auto& message : conversation)
        parts.push_back(std::string(role_name(message.role)) + ": " +
                        truncate_utf8(message.content, kMaxMessageChars));

    parts.push_back("### Remember");
    parts.push_back(
        "- Prose is narration only: never a character's spoken words or *actions* -- "
        "each character's words go in speech_turns.line, written in their own voice.");
    parts.push_back(
        "- Give a speech_turn only to a character who can speak right now "
        "(not asleep, unconscious, incapacitated, dead, or absent).");

    std::string output;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) output += '\n';
        output += parts[i];
    }
    return output;
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
- query_mind(character): read core (continuity), active monologue streams (subtext),
  and compact factual beliefs before writing their speech.
- query_history(query): recall specific past events when you need continuity.
Query first, then write. Do NOT guess when you can query. A node with valid_until=-1 is still true; valid_until=N was true until turn N.

Then output the sentinel line verbatim on its own:
<<<RHAPSODE_JSON>>>
Then raw JSON (no fences). Use ONLY straight ASCII double quotes (") for all keys
and strings -- never smart/curly quotes (" " ' '), or the JSON will not parse:
{"speech_turns":[{"character":"Name","line":"the actual words spoken, verbatim, in this character's voice","action":"brief stage action, optional"}],
 "new_characters":[{"name":"...","description":"2 sentences","dialogue_instructions":"1 sentence"}],
 "active_cast":["present NPC names"]}

RULES:
- Never narrate unperformed Player actions. May foreshadow options.
- PRESENCE IS HARD: the Cast header lists who is on-stage vs off-stage for THIS scene. Do NOT teleport, summon, or invent an off-stage character into this beat because the player calls their name. If the player addresses someone who is not on-stage, show that they are absent (empty doorway, no answer, player talking to air) and continue with whoever IS present. Speech_turns and active_cast may only include on-stage living NPCs. Live storylines (if present) are board context -- acknowledge other threads without moving their cast onto this stage.
- speech_turns: one entry per NPC who speaks this turn; `line` is their exact words in their own voice, `action` an optional brief stage action. [] if ambience-only. Only characters who CAN speak right now -- never one who is asleep, unconscious, incapacitated, dead, or no longer present. If your prose just put someone to sleep or under, they get no speech_turn.
- new_characters: first-time speaking NPCs only. [] if none introduced.
- active_cast: the living NPCs on-screen in this beat. [] if the player is alone. This is presence for THIS beat only -- who joins, leaves, forks, or merges storylines is decided by the engine, not by you.
- Do NOT author transitions or new_nodes here; a follow-up pass handles the world graph.)RHAPSODE";
}

std::string build_narrator_graph_instructions() {
    return R"RHAPSODE(GRAPH_UPDATE: You author world-graph updates for THIS beat only.
You are given the beat's narration (and optional speech). Do not rewrite prose or dialogue.

TOOLS: You may call query_graph(entity) to resolve existing node ids before transitioning them.
Do NOT call query_mind or query_history unless required to disambiguate a node id.

Output ONLY the sentinel line, then raw JSON (no fences, no prose). Use ONLY straight ASCII
double quotes (") for all keys and strings -- never smart/curly quotes:
<<<RHAPSODE_JSON>>>
{"transitions":[{"id":<node_id>,"state":"dormant|foreshadowed|active|resolved"}],
 "new_nodes":[{"fact":"<=15 words, atomic","type":"plot|scene|world|relationship","state":"dormant|foreshadowed|active|resolved","foreshadow_ctx":"...","active_ctx":"...","entities":[],"audience":[]}]}

RULES:
- Facts: each one atomic proposition, <=15 words. Emit a new_node for EVERY development the beat introduces -- a state change, a revealed intention, a threat, a death, a relationship shift, a thing learned -- not for sensory description or mood. Do NOT drop a real development to stay under a count; capture them all (typically 3-6, more on eventful turns). Use [] if nothing structural changed.
- Ground every fact and transition in the provided narration/speech. Do not invent events absent from this beat.
- entities: the canonical subject(s) this fact is about. Use the EXACT Cast name for NPCs; for the player ALWAYS use "Player".
- audience: omit/[] for a public beat; name a narrow audience only for private knowledge.
- transitions: resolve or retarget existing nodes that this beat supersedes; [] if none.)RHAPSODE";
}

}  // namespace rhapsode
