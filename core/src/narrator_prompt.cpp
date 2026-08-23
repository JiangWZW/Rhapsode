#include "rhapsode/narrator_prompt.h"

#include "rhapsode/json_util.h"
#include "rhapsode/scene_data.h"
#include "rhapsode/scene_history.h"
#include "rhapsode/str_util.h"
#include "rhapsode/text_downsampling.h"
#include "rhapsode/world.h"

#include <algorithm>
#include <sstream>
#include <vector>

namespace rhapsode {
namespace {

constexpr size_t kVerbatimSpans = 16;
constexpr size_t kMaxMessageChars = 400;
constexpr size_t kMaxStoryChars = 1500;

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

std::string build_narrator_turn_state(const SceneData& scene,
                                      const World& world,
                                      const std::string& live_storylines_board) {
    std::ostringstream os;

    std::vector<std::string> off_stage_names;
    os << "On this stage:\n";
    bool any_on_stage = false;
    for (const auto& character : world.characters()) {
        if (character.is_player || character.dead) continue;
        if (!character.in_scene(scene.scene_id)) {
            off_stage_names.push_back(character.name);
            continue;
        }
        any_on_stage = true;
        os << "- " << character.name;
        if (!character.role.empty()) os << " [" << character.role << "]";
        const std::string description = str::trim(character.description);
        if (!description.empty()) os << " -- " << description;
        os << "\n";
        const std::string voice = str::trim(character.build_prompt__dialogue_voice());
        if (!voice.empty()) os << voice << "\n";
    }
    if (!any_on_stage)
        os << "(you are alone)\n";

    if (!off_stage_names.empty())
        os << "\nNot on this stage: " << join_sorted(std::move(off_stage_names))
           << "\n";

    if (!live_storylines_board.empty())
        os << "\n" << live_storylines_board;

    const std::string story_so_far =
        render_text_downsampling(scene.downsampling);
    if (!story_so_far.empty()) {
        os << "\nWhat has already happened:\n"
           << truncate_utf8(story_so_far, kMaxStoryChars) << "\n";
    }

    os << "\nWhat was just said and done:\n";
    const auto spans = attributed_transcript(scene, kVerbatimSpans);
    if (spans.empty()) {
        os << "(nothing yet)\n";
    } else {
        for (const auto& span : spans) {
            const std::string speaker = span.speaker.empty()
                ? "Narrator" : span.speaker;
            os << speaker << ": "
               << truncate_utf8(span.exact_content, kMaxMessageChars) << "\n";
        }
    }
    return os.str();
}

std::string build_narrator_instructions() {
    return R"RHAPSODE(You narrate a live scene. Second person, present tense. The player's act is already on the page.

Prose is the stage: bodies, weather, what can be seen. It is not dialogue.
No quotation marks, no *asterisks*, and no spoken words in the paragraphs.
When someone speaks, their exact words go in speech_turns.line, in that person's voice.
Empty speech_turns is a silent take.

Do not invent a private mind. query_mind, query_graph, and query_history when you lack a fact.
Do not guess continuity. A node with valid_until=-1 is still true; valid_until=N was true until turn N.

Presence is hard: speech_turns and active_cast are on-stage living NPCs only.
If the player calls someone who is not here, the stage shows their absence.
Other live threads are board context -- do not move their cast onto this stage.
Never narrate unperformed Player actions. You may foreshadow options.
Do not author transitions or new_nodes; a follow-up pass handles the world graph.

After any tool use, begin with a few short paragraphs of prose. No preamble.
Then the sentinel and JSON. Use ONLY straight ASCII double quotes (") for all keys
and strings -- never smart/curly quotes, or the JSON will not parse.

<<<RHAPSODE_JSON>>>
{"speech_turns":[{"character":"Name","line":"the actual words spoken, verbatim, in this character's voice","action":"brief stage action, optional"}],
 "new_characters":[{"name":"...","description":"2 sentences","dialogue_instructions":"1 sentence"}],
 "active_cast":["present NPC names"]}
new_characters is first-time speakers only ([] if none). active_cast is who is on-screen this turn ([] if the player is alone).)RHAPSODE";
}

std::string build_narrator_graph_instructions() {
    return R"RHAPSODE(GRAPH_UPDATE: record what this take changed in the world graph. Do not rewrite prose or dialogue.

You may query_graph to resolve existing node ids before transitioning them.

Output only the sentinel, then JSON. Use ONLY straight ASCII double quotes (")
for all keys and strings -- never smart/curly quotes:

<<<RHAPSODE_JSON>>>
{"transitions":[{"id":<node_id>,"state":"dormant|foreshadowed|active|resolved"}],
 "new_nodes":[{"fact":"<=15 words, atomic","type":"plot|scene|world|relationship","state":"dormant|foreshadowed|active|resolved","foreshadow_ctx":"...","active_ctx":"...","entities":[]}]}

Each new_node is one atomic fact, <=15 words: a state change, revealed intention, threat, death, relationship shift, or thing learned -- not mood. Capture every real development (often 3-6). [] if nothing structural changed.
Ground facts and transitions in the provided narration and speech. Do not invent events.
entities: exact on-stage name for NPCs; always "Player" for the player.
transitions: resolve or retarget nodes this take supersedes; [] if none.)RHAPSODE";
}

}  // namespace rhapsode
