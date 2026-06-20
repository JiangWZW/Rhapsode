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
  - server/scenarios/siege.json
  - wiki/episodes/2026-06-09-pull-vs-push-and-three-growth-directions.md
  - wiki/episodes/2026-06-15-character-charge-model-realignment.md
  - wiki/episodes/2026-06-10-parallel-scenes-shared-pool-lod-lifecycle.md
last_updated: 2026-06-20
confidence: proposed
tier: semantic
related:
  - pull-vs-push-and-three-growth-directions
  - character-charge-model-realignment
  - parallel-scenes-shared-pool-lod-lifecycle
  - narrator-and-mind-weakpoints
  - subjective-character-minds
  - plot-graph
  - scene-loop
tags:
  - design
  - episode
  - parallel-scenes
  - lifecycle
  - narrator
  - scheduler
---

# Episode — Parallel scenes: the scene lifecycle is an authored judgment; the engine's job is context (2026-06-20)

The [push agenda](2026-06-09-pull-vs-push-and-three-growth-directions.md) needs **parallel scenes** —
storylines the player is not watching, advanced by the engine so the world stops being a function of
player attention. Building them raises four lifecycle questions:

- **Fork** — when does a new scene begin, and what is in it?
- **Pause** — when does a scene stop being advanced, and is anything thrown away?
- **Terminate** — when does a scene *end*, as opposed to merely resting?
- **Merge** — when do two scenes become one, which ones, and how does the joined scene read?

This page answers all four from a single principle, grounds every term in the current code, and separates
what is genuinely new to build from what already exists.

---

## 1. The governing principle

**The engine does not *decide* the scene lifecycle with a rule. It assembles accurate and sufficient
context and lets the LLM judge — the same division of labour this engine already runs everywhere else.**

The pattern is consistent across the codebase: cheap mechanical signals are *rendered as context*; the
authored call is an LLM's.

- The **narrator is the sole world-author**; the Director only applies the narrator's plan
  (`apply_planned_turn`, `director.cpp`).
- The **narrator is the perceivability oracle**: a fact reaches a mind only if the narrator names that
  mind in the node's `audience` (`node.h:39`, `route_perception`, `scene_loop.cpp:291`). Who perceives
  what is judged, not computed from proximity.
- **Death is an LLM judgment** (`confirm_deaths`, `scene_loop.cpp:680`); the keyword scan merely *flags a
  candidate*. Signal → flag → judgment.
- The **charge model** states its own role outright: a thought's pressure is "rendered as context for the
  narrator, **never a command** … You decide what surfaces" (`character_memory.h:83`,
  `scene_loop.cpp:278`).

So the design problem is **not** "find the importance scalar / the clustering function that decides which
scenes live." It is: *what context makes the LLM's fork / pause / terminate / merge calls reliable?*
Charge, entity overlap, and staleness all survive — as **columns of context**, not as deciders.

---

## 2. Durable state vs. an ephemeral scene

A scene owns no world state. It is a *projection*: a running narration over a subset of state that lives
elsewhere and persists regardless.

**Durable (persists, owns state):**

- **One shared world graph** — a `WorldGraph` of facts (`world_graph.h`).
- **The minds** — `CharacterMemory` per character, each a subjective belief graph of charged nodes
  (`character_memory.h:114`).
- **Intentions** — a character's forward drive (§3); what a scene is *about*.

**Ephemeral (a projection, re-derivable, disposable):**

- **A scene** — `{ cast on stage, a prose stream, a story-clock, a cadence }` over the durable state.

The payoff of this split runs through all four answers below: because scenes own nothing, forking,
pausing, and merging move *projections* over durable state that **never has to be reconciled.** Discarding
a scene loses nothing; it can be re-derived from the graph and the minds.

Today this split does not exist: a single `Scene` owns the graph, the minds, the history, and the cast
together (`scene.h:28-33`). Making it real is the one substrate refactor (§7).

---

## 3. Vocabulary, defined against the code

Every load-bearing term tied to a struct/field, marked for what exists vs. what is new:

- **charge** — `Node::weight ∈ [0,10]` (`node.h:31`), maintained only by reflection: reinforce a new
  thought's neighbourhood `+1` (cap 10), decay untouched `×0.9`, cull below `0.05`
  (`character_memory.cpp:301-307, 443-474`). World facts stay at 0; only belief nodes carry it.
  **Exists.**

- **intention** — a belief `Node` whose `fact` is a *forward goal* ("I have sent trackers to hunt the
  captain"). Today this is **indistinguishable** from a belief about the past — both are `type=="belief"`
  (Voss's pursuit is a seeded belief, `scene.cpp:264-274`, from `siege.json` `initial_memory.beliefs`).
  The single piece of **new state** the design needs is a way to tell an intention apart from a belief
  (a `type=="intention"` value, or a flag), so a scene has a nameable drive. Its charge is just its
  `weight`. **New — one field.**

- **"thread"** — *reserved word.* In this codebase `WorldGraph::all_threads()` already means **a connected
  component of the active-edge graph** (`world_graph.cpp:296`), and it is unsuitable as a dramatic unit:
  `add_node_chained` links each fact to the most-recent node sharing *any* entity (`world_graph.h:38`),
  so components fuse into a single blob. This page never uses "thread" for the dramatic sense — it says
  "a character pursuing an intention off-stage," which in data is the pair *(the `CharacterMemory`
  holding the intention, that node)*. **Not a new struct.**

- **scene** — `Scene` exists but there is exactly one (`scene.h`). After §7 a scene is
  `{ cast, prose History, downsampler, story-clock, cadence }` over shared state.
  `Character::on_stage` — a bool today, flipped by `apply_active_cast` (`character.h:17`,
  `scene_loop.cpp:199-209`) — generalizes to *which scene a character is on stage in*. **Generalized.**

- **beat** — *not a struct; a unit.* One scene's authored turn: a single run of the
  `narrator → apply_planned_turn → Validator` pipeline (`scene_loop.cpp:481`) that emits one prose
  passage plus one structured plan (`new_nodes` / `audience` / `transitions` / `speech_turns` /
  `active_cast`, and the new `fork` / `thread_status` tags) and ticks that scene's story-clock once. The
  retry loop re-authors *the same* beat, not a new one (`scene_loop.cpp:550`). It is the unit the narrator
  authors, the unit fork/terminate tag, and the unit the scheduler hands out. (`node.h:38`'s "public
  beat" is a looser, fact-level use of the word — ignore it here.) **Existing pipeline; new tags.**

- **"where" a scene is** — there is no place concept in the code; `Scene` carries only a `title`. "Where"
  is read from `Node::entities` overlap (what `WorldGraph::entity_groups()` and the Weaver already run
  on), made *current* via `chain_predecessors` — the most-recent active node per shared entity
  (`world_graph.h:78`). It is **one column of the digest, not a decider** (§1); §8 names the only subtlety
  — co-presence (*who is together now*) must not be confused with aboutness (*who a scene is about*).
  **Inferred from entity recency.**

---

## 4. The structural cut: in-beat decisions vs. cross-scene decisions

The four questions are not four mechanisms. They split cleanly by **where the sufficient context already
lives** — and that split decides which of them even need anything new.

### Fork and terminate are authored *inside a beat*. Their context is sufficient by construction.

When the narrator writes a beat and Voss dispatches the trackers and turns to other matters, *that
narrator, at that moment, holding that scene's whole context*, is the only authority that knows an
independent locus just budded. Likewise, when an autonomous beat advances the rebel war, only that
narrator knows whether it is *over* or *still unfolding* — a distinction no scalar can draw (a war stays
maximally charged right to its end).

So fork and terminate are **tags the narrator emits inside the plan it already emits**, on the same
channel as `new_nodes` / `audience` / `new_characters`. No new context, no new call.

- **Fork** is a tag on a beat: *a new locus has begun that will act on its own.* The only design work is
  the instruction that sets the bar — "Voss sends trackers" (fork) vs. "Voss orders more wine" (not).
- **Terminate** rides the existing `valid_until` lifecycle: the narrator's `thread_status: concluded`
  sets the driving intention's `valid_until` (`set_valid_until`), or the Weaver's `check_group`
  supersedes the premise when it dies elsewhere. An ended scene is one whose drive is no longer live.

### Pause and merge are cross-scene. This is the only place context is missing.

You cannot decide "cut to Voss now" or "the trackers reach the player now" from inside any single scene's
beat — that beat cannot see the other scenes. These are judgments *about the relationships among scenes*,
so they need context that spans them. That context is the one genuinely new thing to build (§5–6).

---

## 5. The cross-scene digest

The engine assembles, from the shared graph, a **digest**: one line per live scene, rebuilt fresh every
scheduling step — never cached, because *accuracy* means each line reflects the current graph (the same
discipline `focus_payload_text` already follows). Per scene:

| Column | Source | Why the judgment needs it |
|---|---|---|
| **cast** | the scene's on-stage names; flag if the player is present | who is involved; is this the player's scene |
| **driving intention** | `fact` text of the highest-`weight` intention in the scene's minds | *what the scene is trying to do* — the goal, not a number |
| **charge** | that intention's `weight` | the pressure hint (the scalar, as a column) |
| **where** | current co-presence entities (`chain_predecessors`, `entity_groups`) | one convergence signal the scheduler weighs — co-presence, not aboutness (§8) |
| **staleness** | story-time since last advanced | so an ignored scene can be surfaced or rested |
| **last beat** | the 1–2 facts this scene most recently wrote | the scene's trajectory |

This digest is to the **scheduler** what `build_inner_states` + `focus_payload_text` are to the
**narrator**: accurate context assembled from the live graph for one authored judgment — the same organ,
one layer up. *Sufficient* means the LLM never has to guess about a scene it cannot see; *accurate* means
every line is current. Both are load-bearing.

---

## 6. The scheduler, and sequential ticking

**The scheduler is a new LLM call — nothing like it exists in the code today.** It is the only new organ
this design adds besides the digest:

- **Input:** the digest (§5) — one text line per live scene.
- **Output:** one scene id — *which single scene advances next.* Nothing else; it is one bounded choice.
- **When:** once per turn, after the player's scene advances, to pick the one off-stage scene to interleave.
- **It executes nothing itself.** The scene it picks is then run by the *existing*
  `narrator → apply_planned_turn → Validator` pipeline as a player-less beat (§7). The scheduler only
  *chooses*; the chosen scene's beat does the authoring.

In one line: a new LLM call that reads the digest and returns the id of the next scene to advance. The two
cross-scene decisions fall out of that one answer:

- **Pause** is a non-event. A scene the scheduler does not pick is simply not advanced this turn. Nothing
  is stored per scene, so there is nothing to throw away vs. keep — the "discard or pend?" dilemma
  dissolves. The scene's cast and intentions sit untouched in the shared state, ready to be re-derived.
- **Merge** is the scheduler choosing to advance a scene *because* the digest shows it converging on
  another — a judgment over the whole line (cast, intention, `where`, last beat), not a threshold on one
  column. The scheduler only has to be **collision-aware**; the actual collision is then **authored in
  that scene's beat** (`thread_status: converging`, narrator-authored as in §4), which moves the
  converging cast onto the joined stage. How the merged scene reads is therefore the narrator's call,
  framed by the higher-charge intention — not an assembly the engine performs.

**Scenes advance sequentially, not in parallel.** Off-stage beats are coarse and infrequent by design, so
each turn the player's scene advances and the engine interleaves the *one* scene the scheduler picks. One
writer at a time means the existing whole-graph snapshot/rollback (`scene_loop.cpp:548, 553`) is
untouched, and the concurrency problem evaporates entirely. Parallel scenes do not require parallelism.

---

## 7. The substrate (the one real refactor)

Split today's monolithic `Scene` (`scene.h:28-33`) into shared and per-scene:

| `Scene` field today | becomes |
|---|---|
| `world_graph` | **shared** — one global `WorldGraph` |
| `character_memories` | **shared** — minds persist regardless of which scene they are on |
| `characters[]` (+ `on_stage` bool) | **shared roster**; `on_stage` → which scene id |
| `history` | **per-scene** prose stream |
| `downsampler` | **per-scene** |
| `turn_index` | **per-scene** story-clock |
| (none today) | **per-scene** cadence |

Behaviour-preserving at one scene (which equals today). Acceptance: the tavern and siege scenarios run
identically to now.

The off-stage autonomous beat itself is **not** new machinery — it is the existing
`narrator → apply_planned_turn → Validator` pipeline with a player-less prompt whose context is built
from graph-focus instead of player input.

---

## 8. "Where" is a column, not a gate

It is tempting to treat "where each scene is" as a gate: get it accurate *by construction* or merge is
unreliable however it is prompted. That reverses §1. The scheduler is an LLM reading the digest; it never
*computes* a collision and the engine never *knows* two scenes have met. "Where" is one column beside
cast, driving intention, staleness, and last beat — exactly the status §1 assigns charge, entity overlap,
and staleness: "columns of context, not deciders." A fuzzy "where" is fine; the judge weighs it with the
rest, as it weighs every other noisy signal in this engine. The only obligation is the one §5 already
states: render it *accurately and fresh every step*.

There is **one** genuine subtlety, and it is small. A scene is *about* its target long before it is *with*
it: Voss's trackers name the player in their driving intention from t=0 (`scene.cpp:264-274`), so a
"where" built from raw entity mention would read "converged" immediately — the trackers and the player
share the entity "Player" before a single tracker has moved. So the column must express **co-presence**
(*who is physically together now*), never **aboutness** (*who a scene is about*). Co-presence is already
modeled: it is scene membership — `on_stage` generalized to a scene id (§7). Arrival itself is not the
scheduler's to detect; it is **authored in the beat** (`thread_status: converging`, §4/§6), and the
off-stage narrator can author it truthfully because the digest hands its beat the other scene's line. The
scheduler's only job is to keep advancing the pursuing scene so that beat can happen — i.e. to not starve
it (the staleness column).

So nothing gates the build. Render "where" as the freshest co-presence read from entity recency
(`chain_predecessors`), label it co-presence rather than target-mention, and let the scheduler and the
in-beat narrator judge from there. If that proves too coarse in practice, promoting co-presence to
something the narrator *asserts* (a tagged co-presence relation) is a *later, local* upgrade to one digest
column — not a precondition.

Open points, all genuinely deferrable: the channel for the intention/belief distinction (`type` value vs.
flag); whether the autonomous beat runs on the cloud narrator (quality) or the local model (cost); and
the scheduler's exact prompt shape.

---

## 9. The N=1 proof

The same proof the push agenda has carried since 06-09, now driven by context-and-judgment:

1. **Seed.** Voss's authored belief "sent trackers into the eastern woods" is marked an *intention*
   (§3); it is the driving line of a candidate off-stage scene.
2. **Player elsewhere.** Each turn the scheduler reads the digest. The Voss scene's `where` advances —
   "trackers enter the woods," "pick up the trail" — as autonomous beats land facts in the shared graph.
3. **Convergence.** When the digest shows the Voss scene *arriving where the player is* — co-presence, not
   merely still hunting them — the scheduler advances Voss; that beat authors the collision
   (`thread_status: converging`); the trackers' cast moves onto the player's stage and the narrator
   renders, at full detail, "three of Voss's trackers step from the treeline."
4. **The world pushed.** The authored plot fired **without the player ever naming Voss's entities** — the
   exact failure 06-09 identified — and every fork, merge, and conclusion along the way was an authored
   judgment over assembled context, not a rule.

Run this for Voss alone on `siege`; if the trackers arrive unprompted, the bet is proven.

---

## 10. Relationship to the prior plans

This builds directly on the [06-09 diagnosis](2026-06-09-pull-vs-push-and-three-growth-directions.md)
(the engine is 100% pull; off-stage minds neither deliberate nor surface) and on the
[06-15 charge model](2026-06-15-character-charge-model-realignment.md) (charge as an already-computed
pressure signal). It keeps the **shared pool** from the
[06-10 plan](2026-06-10-parallel-scenes-shared-pool-lod-lifecycle.md) and departs from the rest of it:
where 06-10 made a player-anchored *importance* metric and a `SceneManager` state machine the deciders of
spawn/pause/merge, this page makes the lifecycle an **authored judgment over assembled context**, which
removes the importance metric, the manager state machine, the latent-locus type, the spawn gate,
per-scene cached history, and the parallel-write concurrency gate. What remains is one substrate refactor
(§7), one new field (intention), and one new organ (the digest plus a sequential scheduler).
