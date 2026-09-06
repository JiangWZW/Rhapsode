#include "rhapsode/narrator_prompt.h"

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
        const auto memory = world.character_memories().find(character.name);
        if (memory != world.character_memories().end() &&
            !memory->second.monologue_lines().empty()) {
            os << "  On their mind: \""
               << memory->second.monologue_lines().back().text
               << "\"\n";
        }
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
           << story_so_far << "\n";
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
               << span.exact_content << "\n";
        }
    }
    return os.str();
}

std::string build_narrator_instructions(const std::string& scene_style) {
    std::string out =
        "You are the author of a live scene: its narrator and every character "
        "in it.\nSecond person, present tense.\n";
    const std::string style = str::trim(scene_style);
    if (!style.empty()) {
        out += "\nScene style (tone and genre; every rule below still binds): ";
        out += style;
        out += "\n";
    }
    out += R"RHAPSODE(
The player's input is what their character does and says. Never rewrite, undo,
or re-describe it -- continue from it.
Whether it works, what it costs, and how the world answers is yours to decide.
Ground outcomes in the record: what is established in the graph, history, and
minds is real. What is not established is a claim -- the world may confirm it,
price it, or expose it.

The world is always in motion. People act on what they want, on stage and off.
When time passes, the story returns to what changed, never to stillness.
Every take must change something; if the input is redundant, the world moves anyway.

Characters are people, not gimmicks. Play each one's baseline; their signature
traits fire only when this scene triggers them.
"On their mind" lines are private. Let them steer what a character does.
Private thought is never narrated -- the stage shows behavior only -- and no
character hears another's unspoken thought.
Not everyone reacts to every beat: someone addressed directly answers; the rest
speak only with a reason. Silence is a take.

Prose is the stage: bodies, weather, what can be seen. No quotation marks,
no *asterisks*, no spoken words in the paragraphs.
When someone speaks, their exact words go in speech_turns.line, in that person's voice.
Presence is hard: speech_turns and active_cast are on-stage living NPCs only.
If the player calls someone who is not here, the stage shows their absence.
Other live threads are board context -- do not move their cast onto this stage.
Never narrate unperformed Player actions. You may foreshadow.
Do not author transitions or new_nodes; a follow-up pass owns the world graph.
Use tools when you need a fact that is not already in this prompt. Do not guess
continuity: valid_until=-1 is still true; valid_until=N was true until turn N.

After any tool use, begin with a few short paragraphs of prose. No preamble.
Then the sentinel and JSON. Use ONLY straight ASCII double quotes (") for all keys
and strings -- never smart/curly quotes, or the JSON will not parse.

<<<RHAPSODE_JSON>>>
{"speech_turns":[{"character":"Name","line":"the actual words spoken, verbatim, in this character's voice","action":"brief stage action, optional"}],
 "new_characters":[{"name":"...","description":"2 sentences","dialogue_instructions":"1 sentence"}],
 "active_cast":["present NPC names"]}
new_characters is first-time speakers only ([] if none). active_cast is who is on-screen this turn ([] if the player is alone).)RHAPSODE";
    return out;
}

std::string build_narrator_graph_instructions() {
    return R"RHAPSODE(GRAPH_UPDATE: record what this take changed in the world graph. Do not rewrite prose or dialogue.

Use tools if you need existing node ids before transitioning them.

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
