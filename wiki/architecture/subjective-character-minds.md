---
sources:
  - core/include/rhapsode/character_memory.h
  - core/include/rhapsode/node.h
  - core/include/rhapsode/world_graph.h
  - core/src/scene.cpp
  - core/src/scene_loop.cpp
  - core/src/director.cpp
  - core/src/memory_system.cpp
  - server/rhapsode/prompt.py
last_updated: 2026-06-07
confidence: proposed
tier: semantic
related:
  - character-system
  - plot-graph
  - memory-system
  - scene-loop
  - character-agent-maren-analysis
tags:
  - cpp-core
  - memory-architecture
  - design
---

# Subjective character minds

**Stale on routing.** Narrator `audience` / `route_perceptions` / `route_fact` are gone. On-stage minds get an objective journal (`take` / `seen`), then monologue. Prefer [monologue-streams.md](monologue-streams.md).

This page was the design of record for re-founding character memory so that each character holds a private,
subjective view of the world and of other characters. It is a planned refactor, delivered in slices;
the build order and status sit at the bottom.

## Why

A character must know only what it has been told or has perceived — Warden Voss's read on the player is
not Sergeant Maren's. The bug that exposed the gap: Maren joked about the player's "promotion" though
she should know he was stripped of rank. Tracing it revealed there is no leak-free "scene context" to
hand a character today. The only structured event store is the omniscient `WorldGraph`, and the only
narrative stream is the narrator's reader-facing prose. Feeding either into a mind is the same
omniscience leak the actor prompt already had through `retrieve_memories` — relocated, not fixed.

The deeper observation is about structure. The `WorldGraph` `Node` is well-built: `entities` for
subject keying, `state` plus `valid_until` for current-versus-historical without deletion, typed edges,
and entity-chaining (`entity_groups`, `chain_predecessors`). The legacy `CharacterMemory` graph is a
stream of text blobs retrieved by embedding similarity, lacking every one of those. Past attempts to
bolt subject tags, revise-edges, and recency tuning onto it were a poor reimplementation of fields the
`Node` already provides. The design re-founds the mind on the structure that works.

## Composition over inheritance

`CharacterMemory` contains a subjective `WorldGraph`; it is not one.

```cpp
class CharacterMemory {
    WorldGraph  beliefs_;     // subjective graph — same Node/chaining/valid_until as the narrator's
    std::string persona_;     // identity and disposition: the interpretive lens
    std::string self_state_;  // carried first-person state, orthogonal to beliefs
};
```

The narrator's omniscient graph and a character's mind are the same type used two ways. An actor holds
a reference only to its own `beliefs_`, so reading the narrator's graph is impossible by construction — +the knowledge boundary is a property of the object model rather than a rule to enforce.

## Three layers, three owners

```
 Truth  ──────────────▼ Perception  ──────────────▼ Interpretation
 world graph             narrator routes facts        reflection
 (omniscient ledger)     to the minds that perceive   (persona + priors → belief)
```

1. **Truth** lives in the world graph: the omniscient ledger, holding everything including secrets.
2. **Perception** is a per-character objective journal. The world graph no longer copies `new_nodes`
   into minds. After the take commits, every living on-stage NPC gets that turn's narrator prose and
   speech as a `take` line. A per-character LLM then writes `seen` lines: what this person could take
   in, judged from who they are, their journal so far, and the latest take. No position tags. The
   player does not get a journal (the screen is the player's memory).
3. **Interpretation** is reflection: routed perception becomes belief through persona and prior
   beliefs. Two minds handed the same fact diverge, because each reads it as who it is.

## Views, history, and retrieval

A character's view of X is the X-entity chain inside its `beliefs_`. The current view is the active
head of that chain (`entity_groups`); the history is the whole chain including `valid_until`-marked
links. A bad first impression is never deleted — it becomes a Dormant link beneath the present view, so
the arc "I despised him, then I understood him" survives, timestamped. The actor prompt's knowledge
section queries the speaker's own graph for the present cast's entity chains. Retrieval is
deterministic; ChromaDB stays available only as an optional fuzzy secondary index.

## Who does what

Node creation and routing are not the `Weaver`'s job. The real responsibilities:

- **Fact creation and routing (perception)** happen where the `Director` ingests narrator output and
  adds nodes to the world graph (`director.cpp`). The `new_nodes` schema and the `Director`'s parsing
  gain an `audience`; after a node is added to the world graph, its fact is routed into each `audience`
  character's `beliefs_` (`scene.character_memories[name]`). `MemorySystem::process_new_nodes`
  continues to do ChromaDB indexing only.
- **Per-mind maintenance** (chaining, supersession, expiry) is a `Weaver` instance built over each
  character's `beliefs_` graph and run in the background. The `Weaver` already operates on a
  `WorldGraph&`; here it maintains a mind, it does not create one.
- **Reflection** (perception to persona-colored belief) is new logic on `CharacterMemory`: an LLM step
  over the mind's existing beliefs about the involved entities, its persona, and the newly routed
  facts, producing belief nodes chained onto the prior view and superseding via `set_valid_until`. It
  runs in the background for minds that received facts, and is fed routed perception, never the
  narrator's prose.
- **Two leaks are removed:** the actor's world-graph read (`retrieve_memories` in `scene_loop.cpp`) and
  the omniscient mind-seeding in `enter_character` (`scene.cpp`), which copies world-graph facts into a
  new mind.
- **Seeding** turns authored `initial_memory.beliefs` into entity-tagged nodes in `beliefs_`. Maren's
  seeded view of the player is a pre-loaded node in the captain chain.

## The failure mode, stated honestly

Correctness now rests on the narrator routing well: under-route and a character is ignorant,
over-route and it knows too much. Nothing floods a mind automatically, so this is a narrator-judgment
error rather than an architectural leak — the better failure mode, and the one the "trust the narrator"
stance accepts deliberately.

## Build order and status

1. **Compose and enforce the boundary** (fixes the Maren bug). Add `beliefs_` to `CharacterMemory`;
   seed authored beliefs as entity-tagged nodes; delete the actor's world-graph read and the
   `enter_character` omniscient seeding; point the actor's knowledge section at the speaker's own
   `beliefs_`; seed Maren's view of the player in `siege.json`.
2. **Narrator-routed perception.** Add `audience` to `new_nodes` and the `Director` parsing; route each
   fact into its `audience` minds.
3. **Background reflection and maintenance.** The new `CharacterMemory` reflection step, plus a per-mind
   `Weaver` for edge and expiry upkeep.
4. **Retire the legacy stream.** Remove the bespoke memory graph, the three-signal scorer, and the
   Generative-Agents reflection pipeline once the slices above subsume them.

Open questions carried into implementation: save-format migration for `CharacterMemory`, whether
reflection runs in C++ or Python, and whether the per-character ChromaDB index survives slice 4.

Status: slices 1— are implemented (`beliefs_` composed; authored beliefs seeded as nodes; the actor
reads only its own graph; narrator `audience` routing in `prompt.py`/`Director`/`route_perception`;
background `reflect_perceptions` folds perception into chained, superseding belief). Reflection runs
in C++. Slice 4 is partial: the legacy stream is detached from the live path (no longer read or
written), but the `MemGraph`/three-signal/Generative-Agents code still exists dormant and awaits
removal. Known rough edges left for tuning: entity aliasing across a character's references
(name vs. role vs. "the captain") can split chains, and `self_state` is still derived from the shared
scene context rather than the mind.
