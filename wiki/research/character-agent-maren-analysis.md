# The Character Agent Problem — A Code-Grounded Analysis of Sergeant Maren

**Scenario:** `server/scenarios/siege.json` — *The Siege of Ashenmoor*
**Subject:** Sergeant Maren (companion, the Player's oldest friend)
**Question:** Why does the system give the LLM *facts about* a character rather than letting it *be* the character?

---

## TL;DR

The character memory subsystem is not where Maren's agency lives. **The Director is the only agent.** It decides what Maren feels and intends by reading the *world graph*, never her memory. Her `CharacterMemory` is a downstream flavor cache consulted only to phrase a decision already made elsewhere. On top of that structural fact, four mechanical defects cause her authored self to *degrade over a session* rather than persist:

1. Her beliefs are severed from the world at load time (`source_fact` dropped).
2. Her wound exists twice — as a plot token and as an interior belief — and the two never sync.
3. A retrieval "ratchet" makes her authored, first-person core monotonically *less* retrievable as play continues.
4. Her memory stream is polluted with non-first-person POV (narrator's second person, reflections' third person).

The result is not a character who "knows the right things to reference" — it is worse than that: a puppet whose strings are pulled by a subsystem that has never read her mind.

---

## 1. The reframe: Maren is a render target, not an agent

Follow the locus of decision-making.

`Director::tick` (`core/src/director.cpp:114`) builds its prompt in `build_prompt` (`director.cpp:244-309`) from **the world graph only** — it serializes graph nodes and BFS-expands from entity matches against the scene text. It never reads `scene.character_memories`. The Director's output produces Maren's `cue`, `dramatic_intent`, `emotional_state`, and `responds_to` (the `speech_turns` schema, `server/rhapsode/prompt.py:12`).

Only *afterward* does `build_actor_prompt` (`core/src/scene_loop.cpp:251`, invoked at `:602`) query her memory and ask a local LLM to render words that fit a decision already taken.

```
World graph ──▼Director decides Maren feels X, intends Y
            ──▼Maren's memory retrieved by keyword
            ──▼actor LLM phrases it
```

Her memory has **zero influence on what she does, intends, or feels** — only on diction. She cannot decide to raise her wound, refuse Warden Voss's order, or break under pressure, because those are plot decisions and the only entity making plot decisions reads the graph, not her.

This is the root cause. The familiar symptoms — no persistent inner state, third-person framing, data drowning voice, broken emotional continuity — are all *downstream* of the fact that the character has no agentic surface.

---

## 2. Her beliefs are severed from the world at load time

The scenario authors deliberately linked Maren's *subjective* beliefs to *canonical* world facts via `source_fact` (`siege.json:46-50`):

| Maren's belief (`content`) | `source_fact` (world node) |
|---|---|
| "鈥 can see the smoke from the battlements" | "Lord Harren's army approaches Ashenmoor from the east." |
| "Voss sealed the gate with refugees still on the road" | "Warden Voss sealed the eastern gate." |
| "The Thornfield refugees are trapped outside" | "Thornfield refugees are stranded on the eastern road." |
| "My wound from Thornfield is getting worse but I can't show weakness" | *(none — pure interior)* |

The loader throws the linkage away (`core/src/scene.cpp:263-264`):

```cpp
for (const auto& bj : im.value("beliefs", nlohmann::json::array()))
    mem.seed_from_graph(bj.value("content", ""), 0);   // source_fact ignored
```

Only `content` is read. Maren's memory graph and the world graph are two **disjoint** Boost graphs with no cross-edges. When the Director later transitions a world node (e.g. the gate opens; node resolved/expired at `director.cpp:329-332`), **Maren's belief never updates.** She remembers the gate as sealed forever. The authors encoded the bridge between her mind and the world; the code discards it.

---

## 3. Two wounds that never sync

The wound is modeled twice, in systems that don't talk:

- **World node** (`siege.json:120-125`): `type:"relationship"`, `state:"Foreshadowed"`,
  `foreshadow_ctx: "Maren favors her left side and winces when she thinks no one is looking"`,
  `active_ctx: "...infected and worsening. She refuses treatment."`
  This is what the **Director** sees and uses to time the dramatic reveal.
- **Her belief** (`siege.json:50`): `"My wound from Thornfield is getting worse but I can't show weakness"` — the one belief with **no `source_fact`**, anchored to nothing.

The escalation of the wound is therefore driven by the Director flipping the world node Foreshadowed鈫扐ctive when the *plot* wants it — **not** by pain accumulating in Maren's own memory. The two are independent variables. The system tracks her wound *better as an external plot token* (a clean foreshadow鈫抋ctive arc) than as her own lived experience (a single frozen, unanchored string). For a character whose entire arc *is* the wound, the simulation models everything about it except her relationship to it.

---

## 4. The retrieval ratchet: her authored self decays out of reach

This is the core "development over a session" finding. In `retrieve()` (`core/src/character_memory.cpp:264-291`):

```
combined   = recency + relevance + importance
recency    = 0.995 ^ (now_turn - last_accessed)
importance = weight / 10
```

Every seed — including the first-person line `"The arrow wound throbs with every step. Can't let the captain see"` (`siege.json:54`, loaded at `scene.cpp:266`) — is created with **weight 0** (`seed_from_graph`). Its `importance` contribution is permanently **0**; it competes on recency + relevance alone.

Meanwhile each turn `speak()`/`observe()` (`scene_loop.cpp:621-622`) add fresh memories scored 1—0 by `score_importance`, and `reflect()` adds more. New memories are recent *and* weighted, so they dominate the ranking.

The ratchet: `last_accessed` is refreshed **only when a memory is retrieved** (`character_memory.cpp:339-342`). The wound seed surfaces only when a cue is semantically about the wound. During any stretch of strategy/gate/Voss cues it is never retrieved → `last_accessed` stays frozen → `recency` decays monotonically → it sinks further → even less likely to be retrieved. **Once it leaves the top-k window it can never refresh itself back in.** Her defining interior line has a one-way exit from her own head.

Across a session, Maren's retrievable "self" predictably shifts:

```
turn 0:   {authored beliefs + first-person interiority}   weight 0, fragile
            → +            ▼ (chatter + reflections accumulate, recent & weighted)
late game: {recent dialogue + 3rd-person reflections}     dominant
```

Her foundational character data is the *least* retrievable thing she owns, and gets monotonically less so.

---

## 5. POV corruption of the memory stream

`observe(narration, turn)` (`scene_loop.cpp:622`, `character_memory.cpp:181`) feeds the **narrator's prose** into Maren's memory. That prose is second-person, addressed to the *Player*: `"From the eastern battlement you can see the smoke"` (`siege.json:83`). So Maren's memories of her own life are recorded in the Player's point of view.

Then:
- `reflect()` (`character_memory.cpp:521`) writes insights in the **third** person (`"insight about what Sergeant Maren thinks/feels/knows"`).
- `briefing()` (`character_memory.cpp:364`) re-summarizes in the **third** person (`"what Sergeant Maren knows and feels"`).

Her stream accumulates three POVs, **none of them hers**. `distill()` (`character_memory.cpp:130`, `"as Sergeant Maren would remember it"`) is the only first-person-leaning step, and it fires only on text over 200 chars. The single authored line in her own voice is outnumbered from turn 1 and, per §4, decaying out.

---

## 6. An unresolved contradiction the prompt cannot adjudicate

The actor prompt (`scene_loop.cpp:284-299`) concatenates three independently-sourced signals with no reconciliation:

1. **Director-assigned** `emotional_state` — derived from the *world graph* BFS.
2. **Her memory** — `char_mem->briefing(cue, 5)`, retrieved by cue keyword.
3. **World facts** — `retrieve_memories(scene.memory(), —  cue)` (`:289`), which can contradict her own (severed, stale) beliefs from §2.

When the Director assigns `emotional_state: "wry, deflecting"` (light cue topic) but her retrieved memory is dread and concealment, both land in the prompt with no tie-breaker and no feedback path back to the Director. The local LLM resolves the clash arbitrarily, turn to turn. This is the mechanical source of the "no felt continuity": not merely that emotion isn't carried, but that two unsynchronized subsystems each assert an emotion and the tie breaks at random.

---

## 7. Implications for a fix

Surface-level fixes (first-person prompt wording, rebalancing voice-vs-data in the actor prompt) treat symptoms. The cross-reference points at structure:

1. **Give the Director a read of each character's current self-state**, so `emotional_state` / `dramatic_intent` are *derived from* the character rather than imposed on it. This is the only change that converts Maren from render target to agent.
2. **Restore the `source_fact` edges** at load (`scene.cpp:263`) so beliefs track world-node transitions instead of freezing at turn 0.
3. **Protect authored seeds from the ratchet** (`character_memory.cpp` retrieval) — give them nonzero weight or pin them, so foundational interiority cannot decay out of retrieval.
4. **Normalize POV on ingest** — rewrite `observe()` input to first person before storing, and make `briefing()`/`reflect()` emit first person.
5. **Reconcile the three prompt signals** (§6) with a single character-owned state, removing the random tie-break.

Without (1) and (2), the rest is polishing the diction of a puppet whose strings are pulled by something that has never read her mind.

---

## Appendix: Maren's turn lifecycle (as currently coded)

```
1. Director::tick(world graph) ───────▼speech_turn cue for Maren
                                          {cue, dramatic_intent,
                                           emotional_state, responds_to}
2. build_actor_prompt(Maren, cue):
     - description / dialogue_instructions / 2 example lines   (static voice)
     - briefing(cue)        ├── 3rd-person summary of memory   (recomputed fresh)
     - retrieve_memories(cue) ├── world facts (separate graph)
     - recent history + narrator beat (≈00 chars)
     - stage direction (Director's emotion/intent)
3. actor LLM ─────────────────────────▼spoken line
4. char_mem.speak(line) + char_mem.observe(narration)  ── append memories
5. (every ~kReflectionInterval/avg-weight memories) reflect() ── 3rd-person insights
```

Persistent across turns: an **episodic memory graph**. *Not* persistent: a **self** — there is no carried "who I am right now"; it is re-derived by similarity search every turn (§1, §4).