---
sources:
  - core/include/rhapsode/character.h
  - core/include/rhapsode/character_memory.h
  - core/src/character_memory.cpp
  - core/src/scene.cpp
  - core/src/scene_loop.cpp
  - server/rhapsode/prompt.py
last_updated: 2026-06-06
confidence: verified
tier: semantic
related:
  - memory-system
  - scene-loop
  - plot-graph
  - cpp-data-model
  - companion-system
  - character-agent-maren-analysis
tags:
  - cpp-core
  - memory-architecture
  - design
---

# Character system

Rhapsode splits every non-player character into two halves. The **persona** is a small static sheet from the scenario file. The **mind** is a living memory that grows each turn. This page traces both halves and the turn pipeline that turns them into spoken lines.

```
Non-player character
├── Persona  (static, authored in the scenario JSON)
→     Character: name, description, dialogue_instructions,
→                example_dialogue, role, on_stage, dead
→ +└── Mind     (dynamic, grows every turn)
      CharacterMemory: memory graph
                     + self_state   (running first-person monologue)
                     + persona copy (identity for prompts)
```

The persona answers who the character is on paper. The mind answers who they are this turn and what they recall. Two C++ classes own these halves: `Character` and `CharacterMemory`. Both live on the `Scene`. The [[scene-loop]] drives them once per turn.

## The persona: `Character`

`Character` is a plain struct. The scenario file fills it, and it rarely changes during play.

```cpp
struct Character {
    std::string name;
    std::string description;            // 1-2 sentences, the "sheet"
    std::string dialogue_instructions;  // voice & style, 1 sentence
    std::vector<std::string> example_dialogue;
    std::string role;                   // companion | major_npc | minor_npc
    bool is_player  = false;
    bool on_stage   = false;            // present in the scene right now
    bool dead       = false;
    int  created_at = 0;                // turn the character entered
};
```

Each field plays a clear role in the actor prompt. The table below maps them.

| Field | Purpose |
|-------|---------|
| `name` | identity and speaker label |
| `description` | the character sheet; also the memory persona |
| `dialogue_instructions` | voice and diction guidance |
| `example_dialogue` | few-shot samples of how the character talks |
| `role` | cast weighting (`companion`, `major_npc`, `minor_npc`) |
| `on_stage` / `dead` | stage and life flags |

A scenario authors the persona inline. Sergeant Maren from `siege.json` shows the shape, including an `initial_memory` block that seeds her mind.

```json
{
  "name": "Sergeant Maren",
  "description": "A veteran soldier and the Player's oldest friend. ... hiding an infected arrow wound that worsens daily. She commands the night watch ...",
  "dialogue_instructions": "Blunt, warm with the Player, guarded around officers. Uses soldier's shorthand. Deflects concern about her health with dark humor.",
  "example_dialogue": [
    "Same shit, different siege. At least the food's worse this time.",
    "I'm fine. Scratch from Thornfield. Stop looking at me like that."
  ],
  "role": "companion",
  "initial_memory": {
    "beliefs": [
      {"content": "Voss sealed the gate with refugees still on the road",
       "source_fact": "Warden Voss sealed the eastern gate."}
    ],
    "context": ["The arrow wound throbs with every step. Can't let the captain see — there's enough to worry about."]
  }
}
```

## Stage lifecycle: the theatre model

Characters enter and leave a scene like actors on a stage. The `on_stage` flag gates who can speak and who feeds the prompt. `Scene` owns the lifecycle methods.

| Method | Role |
|--------|------|
| `enter_character(ch)` | add an NPC and build its `CharacterMemory` |
| `find_on_stage(name)` | look up a present, living NPC |
| `exit_character(name)` | clear the `on_stage` flag |
| `scan_death_candidates()` | keyword pre-filter for likely deaths |

Three forces move characters across this stage:

- **Authoring.** The scenario marks who starts `on_stage`.
- **The plan.** The narrator plan carries `new_characters` and `active_cast`. The loop turns each new entry into a `Character` and calls `enter_character`.
- **Death.** A post-turn scan flags candidates, and an LLM confirms each one.

The loop registers fresh NPCs straight from the plan, stamping the entry turn.

```cpp
Character ch;
ch.name        = ch_j.value("name", "");
ch.description = ch_j.value("description", "");
ch.role        = ch_j.value("role", "minor_npc");
ch.on_stage    = true;
ch.created_at  = scene_->turn_index;
if (!ch.name.empty() && !scene_->find_on_stage(ch.name))
    scene_->enter_character(std::move(ch));
```

When `enter_character` runs, `Scene` builds a memory for the newcomer and attaches its persona. The same construction seeds memories from the world graph and from `initial_memory`.

## The mind: `CharacterMemory`

`CharacterMemory` is a per-character cognitive layer. It follows the Generative Agents loop of observe, reflect, and plan. See [[memory-system]] for the storage internals and scoring math.

Its store is a directed graph of memory nodes. Each node carries a type, a content string, an importance weight, and timing fields.

```cpp
enum MemoryType { kEvent = 0, kThought = 1, kChat = 2, kUndefined = 3 };

struct MemoryNode {
    std::uint64_t id;
    MemoryType    type;
    std::string   content;
    std::int64_t  created_at;
    float         weight;        // importance, 1..10
    short         depth;         // 0 = base memory, 1+ = reflection
    short         last_accessed; // drives the recency signal
};
```

| Type | Meaning | Source |
|------|---------|--------|
| `kEvent` | something the character perceived | `observe()` |
| `kChat` | a line the character spoke | `speak()` |
| `kThought` | a reflection drawn from other memories | `reflect()` |

### Intake: observe and speak

Two calls write memories. `observe` records the narrator beat. `speak` records the character's own line. Both run after the character speaks each turn.

```cpp
void CharacterMemory::observe(const std::string& scene_context, int turn) {
    MemoryNode node;
    node.type       = kEvent;
    node.content    = distill(scene_context);          // condense, first person
    node.created_at = turn;
    node.weight     = score_importance(node.content);  // local LLM, 1..10
    auto& stored = add(std::move(node));
    reflection_countdown_ -= static_cast<int64_t>(stored.weight);
    embed_and_store(collection_for(char_name_), stored, embed_cb_, store_cb_);
}
```

Each intake does three jobs. It condenses long text through `distill`. It rates the memory through `score_importance`. It drains a reflection countdown by that weight. A heavier memory pushes the character toward reflection sooner.

### Retrieval: three signals plus graph hops

`retrieve(query, top_k)` ranks memories against a query string. It blends three signals into one score.

```
combined  = recency + relevance + importance
recency   = 0.995 ^ (now_turn - last_accessed)
relevance = clamp(1 - cosine_distance, 0, 1)   # vector match
importance = weight / 10                        # weight in [1, 10]
```

```cpp
float relevance  = std::clamp(1.0f - dist, 0.0f, 1.0f);
int   age        = now_turn - node->last_accessed;
float recency    = std::pow(0.995f, std::max(age, 0));
float importance = node->weight / 10.0f;
float combined   = recency + relevance + importance;
```

After ranking, a one-hop graph walk pulls in neighbors. A reflection pulls its evidence; that evidence pulls related reflections. The `briefing` call then asks the local LLM for a short first-person summary.

### Reflection: building higher-order thoughts

Reflection compresses raw memories into insights. It fires from the background thread once the countdown drains. The diagram traces the pipeline.

```
countdown <= 0   (drained by memory weight on each observe / speak)
   → +   ▼+1. gather recent base memories (depth < 3)
2. LLM  → 3 focal questions "about myself"
3. per question: retrieve evidence → LLM → 2 first-person insights
4. dedup each insight against existing thoughts (embedding + LLM check)
5. store survivors as kThought (depth = max evidence depth + 1)
6. link insight → evidence, reset countdown = 60
```

The scene loop triggers this step per character after the turn settles.

```cpp
for (auto& [name, mem] : scene_->character_memories)
    if (mem.needs_reflection())
        mem.reflect();
```

### Self-state: a running first-person mind

A query-driven summary loses the emotional thread when the topic shifts. The self-state fixes that. Each `CharacterMemory` carries one persistent first-person monologue, `self_state_`. The loop folds it forward each turn instead of rebuilding it from a search.

```
self_state(t-1) ─→ +                 ├──▼ LLM fold  ──▼ self_state(t)
recent_events ───→ +seed:  self_state(0) = initial_memory.context   (authored, first person)
```

The fold keeps what still matters and drops only what truly resolves. Maren's hidden wound survives a stretch of pure gate logistics, because the previous state carries it.

```cpp
std::string prompt =
    "You are " + char_name_ + ".\n"
    + persona_line() +
    "A moment ago, my state of mind was:\n\"" + self_state_ + "\"\n\n"
    "What has just happened:\n" + recent_events +
    "\n\nWrite my updated inner state in the first person (3-5 sentences)...";

auto trimmed = trim_llm_response(reflection_llm_cb_(prompt), prompt.size());
if (!trimmed.empty()) self_state_ = std::move(trimmed);   // keep prior on failure
```

`Scene` seeds the opening state from the authored `initial_memory.context`, which scenario writers already phrase in the first person.

### Persona grounding and first-person voice

Two rules keep the mind in character. First, every prompt speaks in the first person. Memories read as I rather than she. Second, every prompt carries the persona. The persona gives the model the right identity and pronouns.

```cpp
std::string persona_line() const {
    return persona_.empty() ? std::string()
                            : "Who I am: " + persona_ + "\n";
}
```

`Scene` attaches the persona from `Character.description` at every load path. The save format skips it, so the description stays the single source of truth. The deeper rationale lives in [[character-agent-maren-analysis]].

## Where the mind meets the turn

One pipeline ties the persona, the mind, and the LLMs together. The diagram shows a single turn end to end. The [[scene-loop]] owns the phases.

```
player input
   → +   ▼+[Phase 1] build the decision prompt
   →   build_inner_states(scene, recent_events, turn):
   →       for each on-stage NPC → update_self_state()
   →       emit "### Inner states" block (first person)
   →   prompt_cb_ → (system, user)
   →       sections: Cast | Graph | Story so far | Inner states | Conversation
   ▼+[Phase 2] narrator / decision LLM → prose + JSON plan
   →   plan.speech_turns = [{character, cue, dramatic_intent,
   →                         emotional_state, responds_to}]
   ▼+[Phase 3] apply plan: world graph, new_characters, active_cast
   ▼+[Phase 4] actor synthesis — per speech cue:
   →   build_actor_prompt(character, cue, narration, scene, char_mem)
   →   actor LLM → spoken line
   →   char_mem.speak(line)  +  char_mem.observe(narration)
   ▼+post-turn (background thread):
   weave → expiry drain → per memory: needs_reflection() → reflect()
```

The character's mind touches two phases. Phase 1 advances and reads the self-state. Phase 4 renders dialogue and writes new memories.

## Two prompts, one decision-maker

Rhapsode runs no separate Director LLM. One merged prompt asks the narrator model for prose and a plan in the same call. That plan fixes each NPC's intent and emotion before any actor speaks. See [[scene-loop]] for the merged-response split.

The plan carries a `speech_turns` array. Each entry is a stage direction, not a line of dialogue.

```json
{"speech_turns": [
  {"character": "Sergeant Maren",
   "cue": "warn the captain the gate decision will cost lives",
   "dramatic_intent": "press her case without insubordination",
   "emotional_state": "strained loyalty",
   "responds_to": "the sealed-gate order"}]}
```

The decision prompt carries the `### Inner states` block. So the narrator grounds each `emotional_state` in the character's own monologue. The `prompt.py` layer splices the block into the user message.

```python
if inner_states:
    parts += ["", inner_states.rstrip("\n")]   # already carries its header
```

## Anatomy of the actor prompt

Phase 4 assembles one prompt per speaking NPC. `build_actor_prompt` stacks the persona, the mind, the scene, and the stage direction. The table lists every section in order.

| Section | Source |
|---------|--------|
| `You are **Name**` | `character.name` |
| Character | `description` |
| Voice & style | `dialogue_instructions` |
| Example lines | `example_dialogue` |
| Others present | on-stage NPC names |
| Inner state | `char_mem->self_state()` |
| Relevant memories | `char_mem->briefing(cue)` |
| What you know | world-graph facts for this cue |
| Recent events | recent history tail |
| Current narrator beat | this turn's prose |
| Stage direction | cue + intent + emotion + responds_to |
| Task | output rules (dialogue only) |

The knowledge block pairs identity with recall. The inner state says who the character is. The briefing says what the character remembers about this beat. Both read in the first person.

```cpp
section(prompt, "Inner state", char_mem->self_state());   // free read
std::string retrieved = char_mem->briefing(cue.field("cue"), 5);
if (!retrieved.empty())
    section(prompt, "Relevant memories", retrieved);
```

The same self-state flows into both prompts. Phase 1 builds it for the decision. Phase 4 reads it for the line. The `build_inner_states` helper drives the Phase 1 pass.

```cpp
for (const auto& ch : scene.characters) {
    if (ch.is_player || ch.dead || !ch.on_stage) continue;
    auto it = scene.character_memories.find(ch.name);
    if (it == scene.character_memories.end()) continue;
    CharacterMemory& mem = it->second;
    mem.update_self_state(recent_events, turn);
    body += "- " + ch.name + ": " + truncate(mem.self_state(), 400) + "\n";
}
```

## Persistence

A save file keeps the mind alive across sessions. The table splits what the save stores from what the loader rebuilds.

| State | Saved? | Restored by |
|-------|--------|-------------|
| memory graph (nodes + edges) | yes | `CharacterMemory::from_json` |
| `self_state` monologue | yes | `from_json` |
| reflection countdown, id counter | yes | `from_json` |
| `persona` | no | re-attached from `Character.description` |
| ChromaDB vectors | rebuilt | `sync_to_chroma` at startup |

The persona stays out of the save on purpose. The `Character.description` already holds it, so re-attaching avoids a second copy that could drift.

## Source map

| File | Responsibility |
|------|----------------|
| `core/include/rhapsode/character.h` | the `Character` persona struct |
| `core/include/rhapsode/character_memory.h` | the mind's public surface |
| `core/src/character_memory.cpp` | graph, retrieval, reflection, self-state |
| `core/src/scene.cpp` | builds + seeds memories, stage lifecycle |
| `core/src/scene_loop.cpp` | turn pipeline, both prompts, actor synthesis |
| `server/rhapsode/prompt.py` | decision-prompt assembly, `speech_turns` schema |

## Related pages

- [[memory-system]] — storage internals, retrieval scoring, reflection details
- [[scene-loop]] — the turn FSM that drives both prompts
- [[plot-graph]] — the world graph that grounds character knowledge
- [[cpp-data-model]] — the full `Character` and `Scene` type tables
- [[companion-system]] — the protagonist-companion design built on this layer
- [[character-agent-maren-analysis]] — why the self-state and first-person rules exist
