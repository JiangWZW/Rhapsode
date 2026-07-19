---
sources:
  - core/include/rhapsode/node.h
  - core/include/rhapsode/scene.h
  - core/include/rhapsode/world_graph.h
  - core/include/rhapsode/character_memory.h
  - core/include/rhapsode/character.h
  - core/src/scene_loop.cpp
  - core/src/character_memory.cpp
  - core/src/world_graph.cpp
  - core/src/scene.cpp
  - core/src/director.cpp
  - bindings/bind_rhapsode.cpp
  - server/rhapsode/app.py
  - server/rhapsode/llm.py
  - server/scenarios/siege.json
last_updated: 2026-07-18
confidence: proposed
tier: episodic
related:
  - "[[scene-loop]]"
  - "[[subjective-character-minds]]"
  - "[[plot-graph]]"
  - "[[narrator-and-mind-weakpoints]]"
tags:
  - design
  - cross-layer
---

# Episode — Parallel scenes as authored judgment, expressed through tools

The [push agenda][push] needs **parallel scenes**: storylines the player is not watching, advanced by the
engine so the world stops being a function of player attention.

Building them raises four lifecycle questions.

- **Fork** — when does a new scene begin, and what is in it?
- **Pause** — when does a scene stop being advanced, and is anything thrown away?
- **Terminate** — when does a scene *end*, as opposed to merely resting?
- **Merge** — when do two scenes become one, which ones, and how does the joined scene read?

This page answers all four from one principle. The engine already runs the narrator on a tool-use loop, so
every lifecycle decision becomes an LLM tool call over infrastructure the engine provides.

---

## 1. The governing principle

**The engine does not decide the scene lifecycle with a rule. It provides tools and context, and lets the
LLM judge.**

This is the division of labour the engine already runs everywhere else. Cheap mechanical signals are
exposed as context or tools; the authored call is the model's.

The narrator already runs on tools. A pull loop drives it — `complete_with_tools` in `llm.py:333`.

The narrator calls `query_graph`, `query_mind`, and `query_history` from `app.py:96-148` to fetch what it
needs. Those dispatch into C++ methods in `scene.cpp:526-643`. Context is pulled, not pushed.

The same split holds for authorship. The narrator is the sole world-author; the Director only applies its
plan through `apply_planned_turn` in `director.cpp:210`.

The narrator is also the perceivability oracle: a fact reaches a mind only when the narrator names it in
`audience`, routed by `scene_loop.cpp:291`. Even death is the narrator's judgment, confirmed by
`confirm_deaths` in `scene_loop.cpp:680`. Signal, flag, judgment — everywhere.

So the design problem is not to find the importance scalar that decides which scenes live. It is a
different question: what tools and context make the LLM's fork, pause, terminate, and merge calls reliable?
Charge, entity overlap, and staleness all survive as columns the LLM reads, never as deciders.

What the engine must provide is the rest of this page: the durable substrate (§2, §7), the read tools that
expose it (§5), the decision tools that mutate it (§5), and the scheduler call that picks the next scene
(§6). The thinking is the model's; the plumbing is ours.

---

## 2. Durable state vs. an ephemeral scene

A scene owns no world state. It is a projection: a running narration over state that lives elsewhere and
persists regardless.

The durable half is a `World`. It persists and owns state:

- **One shared world graph** — a `WorldGraph` of facts, from `world_graph.h`.
- **The minds** — one `CharacterMemory` per character, each a subjective belief graph. See
  `character_memory.h:114`.
- **The roster** — every `Character`, tagged with the scene it is on stage in.
- **Intentions** — a character's forward drive (§3); what a scene is *about*.

The ephemeral half is a `Scene`, re-derivable and disposable:

- `{ scene_id, cast, prose History, Dialogue, downsampler, story-clock, cadence }` over the durable state.
  Its cast is the roster filtered by scene.

Because scenes own nothing, fork, pause, and merge move projections over durable state that never has to be
reconciled. Discarding a scene loses nothing; it can be re-derived from the graph and the minds.

This split does not exist yet. A single `Scene` owns the graph, the minds, the history, and the cast
together — see `scene.h:20-33`, bound at `bind_rhapsode.cpp:82-121`. Making it real is the one substrate
refactor (§7).

---

## 3. Two terms that are new or were ambiguous

Other terms — `World`, scene, read tool, decision tool, "where" — are defined in their home sections: §2,
§5, §7, §8.

- **intention** — a belief `Node` whose `fact` is a forward goal, such as "I have sent trackers to hunt
  the captain." Today a goal and a memory both use `type=="belief"`, so they are indistinguishable; Voss's
  seeded pursuit is one such belief, from `scene.cpp:264-274`. The one new piece of state is a
  `type=="intention"` value, which gives a scene a nameable, queryable drive whose charge is its
  `Node::weight`.

- **beat** — not a struct but a unit: one scene's authored turn. One beat is a single run of the narrator
  tool-use loop through `scene_loop.cpp:481`, producing prose plus an applied plan and ticking that scene's
  clock once. The retry loop re-authors the same beat rather than starting a new one; a beat is the unit
  the narrator authors, the lifecycle tools tag, and the scheduler hands out.

---

## 4. The structural cut: in-beat vs. cross-scene

The four questions are not four mechanisms. They split by where the sufficient context lives, and that
split decides which tool serves them.

### Fork and terminate are authored inside a beat

Take the beat where Voss dispatches the trackers and turns to other matters. That narrator, at that moment,
holds the scene's whole context, so it is the only authority that knows an independent locus just budded.

Endings work the same way. When an autonomous beat advances the rebel war, only that narrator knows whether
the war is over or still unfolding. No scalar can draw that line, because a war stays maximally charged
right to its end.

So fork and terminate are decision tools the narrator calls in-beat. Signatures are in §5.

- **Fork** → `fork_scene`. The narrator, not the engine, names the new scene's driving intention and cast.
  The design work is the instruction that sets the bar: "Voss sends trackers" forks, "Voss orders more
  wine" does not.
- **Terminate** → `conclude_scene`. A scene ends when its driving intention expires.

### Pause and merge are cross-scene

No single beat can decide "advance Voss next" or "the trackers reach the player" — that beat cannot see the
other scenes. These decisions span scenes, so they need the `list_scenes` read tool (§5) and the scheduler
call (§6).

---

## 5. The tool surface (the infrastructure to build)

Everything the LLM does to the lifecycle is a tool call. Tools follow the existing pattern: a C++ method on
the shared `World`, bound via pybind11, described in a Python schema, routed by the dispatcher.

The template to mirror is `app.py:96-161` and `bind_rhapsode.cpp:112-114`.

### Read tools (pull context; no mutation)

| tool | params | returns | backing (new/exists) | caller |
|---|---|---|---|---|
| `query_graph` | `query` | entity-timeline / free-text nodes | `tool_query_graph`, **exists**, `scene.cpp:526`; rehome to `World` | narrator, scheduler |
| `query_mind` | `character` | that mind's thoughts, beliefs, voice | `tool_query_mind`, **exists**, `scene.cpp:578` | narrator, scheduler |
| `query_history` | `query` | matching past messages of this scene | `tool_query_history`, **exists**, `scene.cpp:618` | narrator |
| `list_scenes` | *(none)* | JSON array, one object per live scene | `World::tool_list_scenes`, **new** | scheduler |

`list_scenes` returns structured rows, not a formatted report. The scheduler reads the rows and drills in
with `query_graph` or `query_mind` when a row is ambiguous.

Each row carries these fields:

| field | source |
|---|---|
| `scene_id` | the projection's id |
| `cast` + `player_present` | roster filtered by `scene_id`; flag if a player is in it |
| `driving_intention` | `fact` text of the highest-`weight` `type=="intention"` node among the cast's minds |
| `charge` | that intention's `weight` |
| `where` | current co-presence entities from `chain_predecessors` and `entity_groups`; see §8 |
| `staleness` | that scene's story-clock delta since last advanced |
| `last_beat` | the one or two facts this scene most recently wrote |

The rows are rebuilt fresh on every call, never cached, so each reflects the current graph. This mirrors
the discipline `focus_payload_text` follows in `director.cpp:145`.

### Decision tools (stage a lifecycle change; applied transactionally)

| tool | params | effect when drained | backing (new) | caller |
|---|---|---|---|---|
| `fork_scene` | `driving_intention`, `cast[]` | create a `Scene`; seed the intention as a `type=="intention"` node in the lead cast mind; move named cast onto it via `Character::scene_ids` | `World::stage_fork` → `Story::apply_pending_ops` | narrator |
| `conclude_scene` | `scene_id`, `reason` | retire the projection; return its cast to no scene | `World::stage_conclude` → `Story::apply_pending_ops` | narrator |
| `merge_scene` | `into_scene_id` | move this scene's cast onto the target's stage; retire this projection; the joined scene's next beat is authored by its narrator | `World::stage_merge` → `Story::apply_pending_ops` | narrator |
| `advance_scene` | `scene_id` | record the scheduler's pick of the next scene to run; consumed by the orchestrator | Python scheduler dispatcher (not staged) | scheduler |

Why staged, not immediate:

- The narrator tool loop runs during generation. A beat can be rejected and retried under the existing
  whole-graph snapshot and rollback, seen in `scene_loop.cpp:546-553`.
- If `fork_scene` mutated state mid-loop, a retry would double-apply it.
- So the three narrator decision tools append a `LifecycleOp` to `World::pending_ops_` and return an ack.
- `SceneLoop` clears `pending_ops_` before each narrator attempt, so a rejected/retried beat's staged ops
  are discarded; `Story::apply_pending_ops` drains and applies what the accepted attempt left.
- `advance_scene` is the scheduler's terminal choice, so it is not staged. It records the pick, and the
  orchestrator executes it.

Facts, `audience`, `speech_turns`, and `active_cast` stay on the existing JSON-plan write path:
`split_merged_response` feeding `apply_planned_turn`. This design adds a lifecycle tool surface; it does not
rip out the working fact-writing machinery.

---

## 6. The scheduler, and sequential ticking

The scheduler is a new LLM call, the second new organ after the tool surface of §5. Its shape is
deliberately narrow.

- **Input:** nothing pushed. It is a `complete_with_tools` call given `list_scenes`, plus `query_graph` and
  `query_mind` for drill-down, plus the decision tool `advance_scene`.
- **Output:** one `advance_scene(scene_id)` call — which single scene advances next. One bounded choice.
- **When:** once per turn, after the player's scene advances, to pick the one off-stage scene to interleave.
- **Execution:** none of its own. The scene it picks runs through the existing narrator pipeline as a
  player-less beat (§7). The scheduler chooses; the chosen scene's beat authors.

The two cross-scene decisions fall out of that one answer.

- **Pause** is a non-event. A scene the scheduler does not pick is simply not advanced this turn. Nothing
  is stored per scene, so nothing is thrown away. The cast and intentions sit untouched in shared state,
  ready to be re-derived.
- **Merge** is the scheduler advancing a scene because `list_scenes` shows it converging on another. This is
  a judgment over the whole row — cast, intention, `where`, last beat — not a threshold on one column. The
  chosen scene's beat then calls `merge_scene`, moving the converging cast onto the joined stage.

Scenes advance sequentially, not in parallel. Off-stage beats are coarse and infrequent by design, so each
turn the player's scene advances and the engine interleaves the one scene the scheduler picks.

One writer at a time keeps the existing whole-graph snapshot and rollback in `scene_loop.cpp:546-553`
untouched, and the concurrency problem evaporates. Parallel scenes do not require parallelism.

The orchestration is a thin loop over the existing `SceneLoop` in `scene_loop.cpp:481`. It runs the
player's scene beat, calls the scheduler, then runs the picked scene's beat player-lessly, whose input is
graph-focus rather than a user turn. It drains each beat's `pending_ops_`, and needs no new turn engine.

---

## 7. The substrate (the one real refactor)

Split today's monolithic `Scene` into a shared `World` and a per-scene `Scene`. The starting point is
`scene.h:20-33`, bound at `bind_rhapsode.cpp:82-121`.

| today (`Scene`) | becomes |
|---|---|
| `world_graph` | **`World`** — one shared `WorldGraph` |
| `character_memories` | **`World`** — minds persist regardless of scene |
| `characters[]` + `on_stage` bool | **`World`** roster; `on_stage` becomes `Character::scene_id` |
| `history` | **`Scene`** per-scene prose stream |
| `dialogue` | **`Scene`** per-scene |
| `downsampler` | **`Scene`** per-scene |
| `turn_index` | **`Scene`** per-scene story-clock |
| `scene_id` | **`Scene`** — already exists, `bind_rhapsode.cpp:89` |
| none today | **`Scene`** `driving_intention`, `charge`, `last_advanced` |
| none today | **`World`** `pending_ops_` and the `stage_*` tool methods |
| none today | **`Story`** the scene set, the active scene, and the apply/scheduler methods |

The three concrete moves:

- **`Character::on_stage` (bool) becomes `Character::scene_id` (string).** A scene's cast is the roster
  filtered by `scene_id`. Rebind the field, and update `apply_active_cast` in `scene_loop.cpp:199-209` to
  set a scene id rather than a bool.
- **A new `World` C++ class** owns `world_graph`, `character_memories`, the roster, and `pending_ops_`. It
  hosts the read tools and the `stage_*` decision tools.
- **A separate `Story` hub** owns the scene set and the shared `World`. Each `Scene` co-owns that `World`
  through a `shared_ptr`, so the scenes cannot live inside the `World` without an ownership cycle. `Story`
  hosts `fork_scene`, `merge_scene`, `conclude_scene`, `apply_pending_ops`, and `tool_list_scenes`.
- The pybind dispatcher in `app.py` targets `World` for reads and `stage_*`, and `Story` for `list_scenes`.
  The schema list gains `list_scenes` plus the decision tools.
- **`SceneLoop` reads shared state through `World`** rather than owning it. The per-scene fields it mutates
  — history, clock, downsampler — stay on `Scene`.

Acceptance for the refactor alone: with exactly one scene — `scene_id = "main"`, all cast on it — the
tavern and siege scenarios run identically, no lifecycle tools wired yet.

The off-stage autonomous beat is not new machinery. It is the existing narrator tool-use pipeline with a
player-less prompt, whose context comes from graph-focus via `focus_payload_text` in `director.cpp:145`.

---

## 8. "Where" is a column, not a gate

It is tempting to treat "where each scene is" as a gate: make it accurate by construction, or merge is
unreliable however it is prompted. That reverses §1.

The scheduler is an LLM reading `list_scenes`. It never computes a collision, and the engine never knows
two scenes have met.

So "where" is just one column, beside cast, driving intention, staleness, and last beat. A fuzzy value is
fine: the LLM weighs it with the rest, and drills in with `query_graph` when a row is ambiguous. The only
obligation is the one §5 states — report it accurately and fresh on every call.

There is one genuine subtlety, and it is small. A scene is *about* its target long before it is *with* it.

Voss's trackers name the player in their driving intention from the start, per `scene.cpp:264-274`. A
"where" built from raw entity mention would read "converged" at once, because trackers and player share the
entity "Player" before a single tracker has moved.

So the column must express one thing and refuse another:

- **co-presence** — who is physically together — is the signal to report. It is already modeled as scene
  membership, `Character::scene_id` from §7.
- **aboutness** — who a scene is about — is not. It converges far too early, as the tracker case shows.

Arrival is not the scheduler's to detect. It is authored in the beat via `merge_scene`, and the off-stage
narrator can author it truthfully because it can `query_graph` the other scene's state. The scheduler's
only job is to keep advancing the pursuing scene so that beat can happen, which the staleness column tracks.

So nothing gates the build. Report `where` as the freshest co-presence read from `chain_predecessors`,
label it co-presence rather than target-mention, and let the scheduler and the in-beat narrator judge.

If that proves too coarse, promoting co-presence to something the narrator asserts is a later, local
upgrade, not a precondition. The `where` argument on `fork_scene` already carries the hook for it.

---

## 9. What to build, in order

Each phase is independently testable. Earlier phases do not depend on later ones.

> **Implementation:** Phases 0–4 are built and unit-tested at the mechanical level
> (`core/tests/test_substrate_golden.cpp`, `[substrate]` tags) and wired through `app.py`. The `Story` hub
> owns the scene set; `World` holds the substrate and the staged `pending_ops_`. The open item is a live
> N=1 run (§10) against a scenario that authors an off-stage intention.

- **Phase 0 — Substrate (§7).** Extract `World`, change `on_stage` to `scene_ids`, rebind, and retarget the
  dispatcher to `World`. Acceptance: tavern and siege run identically at one scene. **Done.**
- **Phase 1 — Intention and visibility.** Add `type=="intention"` (seedable from a scenario belief via an
  `intention` flag). Build the `list_scenes` read tool, schema, and dispatch. Acceptance: `list_scenes`
  returns correct rows with driving intention and charge. **Done** (engine + tool; scenario authoring open).
- **Phase 2 — Fork and autonomous beat.** Add `fork_scene`, its stage-and-apply path with a `pending_ops_`
  drain, and the player-less beat (`SceneLoop::submit_autonomous`). Acceptance: a `fork_scene` call creates
  a second scene, and a player-less beat advances it, landing facts in the shared graph. **Done.**
- **Phase 3 — Scheduler.** Add the `advance_scene` tool and the scheduler call, then wire the sequential
  interleave above `SceneLoop` in `app.py`. Acceptance: with the player elsewhere, the scheduler picks and
  advances an off-stage scene across turns. **Done.**
- **Phase 4 — Merge and conclude.** Add `merge_scene` and `conclude_scene`. Acceptance: the N=1 proof
  (§10) — the trackers arrive on the player's stage unprompted, and a resolved scene retires. **Done**
  (mechanics); the live proof run is the open item.

---

## 10. The N=1 proof

This is the proof the push agenda has carried from the start, recast as tools and judgment.

1. **Seed.** Voss's authored belief "sent trackers into the eastern woods" is marked `type=="intention"`
   (§3). It is the driving line of a candidate off-stage scene, which the narrator forks with `fork_scene`.
2. **Player elsewhere.** Each turn the scheduler calls `list_scenes`. The Voss scene's `where` advances —
   "trackers enter the woods," then "pick up the trail" — as player-less beats land facts in the graph.
3. **Convergence.** When `list_scenes` shows the Voss scene arriving where the player is, in co-presence
   rather than mere pursuit, the scheduler calls `advance_scene` on it. That beat calls `merge_scene` onto
   the player's scene, so the trackers' cast moves on stage and the narrator renders, at full detail,
   "three of Voss's trackers step from the treeline."
4. **The world pushed.** The authored plot fired without the player ever naming Voss's entities, the exact
   failure the push agenda identified. Every fork, merge, and conclusion was an LLM tool call over
   engine-provided context, not a rule.

Run this for Voss alone on the siege scenario. If the trackers arrive unprompted, the bet is proven.

---

## 11. Relationship to the prior plans

This keeps the shared pool from the [earlier parallel-scenes plan][shared-pool] and drops the rest. That
plan made an importance metric and a `SceneManager` state machine the deciders of spawn, pause, and merge.

This page makes the lifecycle an LLM judgment expressed as tool calls. That removes the importance metric,
the manager state machine, the latent-locus type, the spawn gate, and the parallel-write concurrency gate.

What remains to build is small: the `World` and `Scene` split (§7), the `intention` field (§3), and one
organ in two parts — the tool surface (§5) and the sequential scheduler (§6).

---

## See Also

- [[scene-loop]] — the turn pipeline each beat runs through.
- [[subjective-character-minds]] — the per-character belief graphs that hold intentions.
- [[plot-graph]] — the shared world graph the tools read and write.
- [push agenda][push] — the diagnosis this plan answers.
- [earlier parallel-scenes plan][shared-pool] — the shared-pool design this one revises.

[push]: 2026-06-09-pull-vs-push-and-three-growth-directions.md
[shared-pool]: 2026-06-10-parallel-scenes-shared-pool-lod-lifecycle.md
