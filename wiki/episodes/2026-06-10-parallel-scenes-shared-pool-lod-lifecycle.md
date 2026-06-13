---
sources:
  - wiki/episodes/2026-06-09-pull-vs-push-and-three-growth-directions.md
  - core/src/scene_loop.cpp
  - core/src/director.cpp
  - core/src/weaver.cpp
  - core/src/validator.cpp
  - server/rhapsode/app.py
  - server/rhapsode/prompt.py
  - server/rhapsode/validator.py
last_updated: 2026-06-10
confidence: proposed
tier: semantic
related:
  - pull-vs-push-and-three-growth-directions
  - narrator-and-mind-weakpoints
  - subjective-character-minds
  - plot-graph
  - scene-loop
tags:
  - design
  - episode
  - parallel-scenes
  - lod
  - world-graph
  - narrator
---

# Episode — Parallel scenes: the shared pool, LOD by character importance, and scene-thread lifecycle (2026-06-10)

This session takes **direction (1) — parallel scenes** from the [2026-06-09 episode](2026-06-09-pull-vs-push-and-three-growth-directions.md) and turns it into an implementable plan. It also **corrects several wrong assumptions** about the current code by reading the source directly (the Validator, the narrator, the Weaver). No code was changed by the discussion; this page is the spec to build against.

The plan was reached by iteration — each correction below replaced an earlier, worse idea, and they are recorded so the reasoning is auditable, not just the conclusion.

---

## 1. The thesis: "Rhapsode" is the architecture

**峥ノ毕堘砍未蠈蟼** (*rhapsōidós*) = **峥ノ€蟿蔚喂谓** (*rhaptein*, "to stitch / sew together") + **峋犖次?* (*ōidē, "song"). A rhapsode was a performer who **stitched songs together** — splicing Homeric lays into one continuous recitation.

So the engine does **not model a world and read narration off it.** It **generates storylines (songs) and stitches them into one cloth.** The organ that does the stitching already exists and is already named: the **Weaver**.

This is the cost argument, and it is load-bearing for every decision below:

- A **world simulator** costs `O(agents × ticks)` — every agent's state advanced every tick. It explodes, and it is what made [Generative Agents](#7-survey-grounding-what-the-prior-art-says) cost "thousands of dollars in token credits" for 25 agents over 2 sandbox days.
- A **storyline generator** costs `O(scenes × beats)` where scenes are few and beats are coarse. A narrative beat ("over three days the trackers close the distance") is *already a compression* of thousands of simulation ticks. We never simulate the substrate; we narrate at the altitude that matters.

> **An earlier framing in this session was wrong:** I first proposed a "level-of-detail *simulation*" with cheap symbolic off-stage state promoted to full narration on collision. That imported the world-simulator cost model. It was rejected. LOD survives, but with a different meaning — see §3.

---

## 2. The model (after correction)

> **One shared pool of facts + minds. Scenes are storyline loops/lenses over it, one thread each. Each loop's LOD — its narrative zoom, story-time step, and cadence — is set by the player-anchored importance of the characters present in it. Loops write through the Validator (coherence + serialization); the Weaver keeps the one cloth coherent and stitches loops where important characters converge. A scene with no player is pushed forward by its own narrator off the cast's intentions + tension. Rising importance is the single curve behind zoom-in, push, spawn, and convergence.**

The pieces, with the intuition for each:

| Piece | What it is | Why |
|---|---|---|
| **Shared pool** | One `WorldGraph` (all facts) + one set of `CharacterMemory` minds, global. Scenes reference it. | A fact produced in the trackers' scene lands in the one graph immediately. **No divergent per-scene worlds to merge** — the failure mode of the shared-nothing alternative I first proposed and then rejected. |
| **Scene = loop/lens** | Not a container of state. A cast-*presence* + a narrator + its own prose stream + its own clock + an LOD, all reading/writing the shared pool. | The unit already exists (`Scene`), it just currently *owns* the graph + minds. We hoist those out. |
| **LOD = narrative zoom** | How finely + how often a scene is told (§3). | The cost dial and the pacing dial, in one. |
| **Importance** | Player-anchored weighted graph-centrality + decay (§4). | The single signal driving LOD, spawn, stitch, destroy. |
| **Autonomous driver** | A narrator call with **no player input**, emitting the same JSON plan. | An off-player scene is just a *pushed* scene — and "push" was the whole point of the 06-09 episode. Reuses the entire existing pipeline. |
| **Transactional gate** | The Validator/commit path, made per-commit (§6, correction #5). | The single serialized writer to the shared pool — coherence gate *and* concurrency gate. |
| **Weaver stitch** | Cross-scene merge on convergence + atomic presence migration. | "Stitch songs together," literally. |

---

## 3. LOD is narrative zoom, not cheap simulation

The LOD we want governs **how each scene loop progresses**:

- **High LOD** (the player's scene): small story-time step per tick, full-detail prose, cloud narrator, every turn. A beat covers a minute.
- **Low LOD** (a distant scene): large story-time step, summarized narration, local model, infrequent ticks. A beat covers a week ("over the following days the trackers close the distance").

`LOD = (story-time step per tick) × (prose granularity) × (loop cadence)`. It modulates *how the story is told*, never a simulation substrate.

**LOD is already native to Rhapsode.** The `TextDownsampler`'s MIP summarization is LOD over *history within a scene* (recent = fine, old = coarse). Scene LOD is the same grain on a new axis: LOD over *attention across scenes*. The 06-09 episode's "backfill is the inverse of the downsampler" is the same density axis a third time. This is the engine's existing grain extended, not a bolt-on.

---

## 4. Importance is the one signal (player-anchored)

**Decision (locked): importance = weighted graph-centrality measured from the `Player` node, with decay. Pure C++ graph math, no LLM in the hot path.**

A character is important to the degree they are densely / heavily connected (via Weaver edge weights — `0.3` weak, `0.6` moderate, `1.0` strong causal) to the `Player` node and to other important characters. Importance decays when a character goes narratively quiet, so the world does not stay maximally lit everywhere.

This one number drives **four** mechanisms at once:

1. **LOD allocation** — a scene's LOD = a saturating max over the importance of its present cast (one major character is enough to make a scene worth telling in detail; a room of extras stays coarse).
2. **Why the player's scene is always top-LOD** — the `Player` is the most important character *by definition*; the scene they're in inherits it. Not a special-cased "focus" flag — it falls out of the rule.
3. **Salience / push** — Voss's trackers start as minor pursuers (low importance → coarse beats). As their facts link, through the woods, *toward the `Player` node*, their importance rises → the scene zooms in → at the top it stitches into the player's scene and they walk on-stage. **Zoom-in, push, and convergence are one curve.**
4. **Lifecycle** (§5) — spawn/destroy admission is keyed on the same importance.

The closure: the **Weaver already owns the edge weights** importance reads, so importance is recomputed naturally whenever the Weaver reweights. The stitcher and the lens-allocator are the same organ.

**Importance anchors on the intention's *target*, not the place.** "Trackers hunt **Player**" is a direct edge to the `Player` node, so the pursuit stays important wherever the player goes — escaping the woods does *not* decay it. A thread only fades when it loses its link to the player (two guards gossiping with no stake in the player decay to nothing). This distinction is what makes pause/resume (§5) behave correctly.

---

## 5. Scene-thread lifecycle — spawn and destroy

> This section was reworked after stress-testing against examples (see §5.4). An earlier draft modeled lifecycle as a `Nascent鈫扐ctive鈫扗ormant鈫扲etired` state machine with top-K admission, hysteresis, and a cooldown. **That was over-built and is rejected.** Because state lives in the shared pool, a scene thread is a **disposable worker** — killing it loses nothing. The corrected model below is what to build.

### 5.1 The unit: a latent locus

A **latent locus** = a set of off-stage entities + a live *driving intention* fact, sitting in the shared pool for free (it's just nodes). The narrator's "Voss sends trackers to hunt you" creates one the moment it's emitted; it costs nothing until it gets a thread. **A scene is just the ticking form of a latent locus, given a worker when it's worth ticking.**

### 5.2 Spawn — narrator-authored seed, importance-gated worker

Spawn is **LLM intelligence at authoring time**: the narrator tags **seed fact(s) + entities** in its plan when it judges a thread worth following has begun (someone goes off to do something that matters; "the innkeeper whispers to a hooded stranger" becomes a thread only because the narrator marks it). This is the spawn authority — *not* a mechanical entity-scan, which can't tell "trackers hunt you" (spawn) from "Voss orders more wine" (don't).

The tag creates the **latent locus**. A **worker thread spawns lazily** when that locus's importance (player-anchored, §4) crosses a floor — so we don't spin threads for every off-hand mention, only the ones that grow to matter to the player. Voss's trackers sit latent until the pursuit's importance rises; *then* they tick.

### 5.3 Two different things I had conflated: PAUSE vs. DESTROY

- **PAUSE / RESUME — mechanical, by importance. Reversible, lossless. NOT destroy.** A locus whose importance drops below the floor simply stops being ticked; its state stays in the pool and resumes (with a catch-up beat) if importance rises again. This is attention/LOD management — two gossiping guards pause when the player walks off.
- **DESTROY (a thread *ending*) — an LLM judgment, never an importance threshold.** Importance cannot tell "still unfolding" from "over" (a war the player cares about stays high-importance through to its end). Ending is always a narrative call, and it arrives through **three paths, all reusing intelligence we already have**:
  1. **Conclude inline** — the autonomous narrator, *as it writes a beat*, declares the thread resolved / failed / abandoned ("rebels crushed, the general returns"). No extra call — it's part of the beat. Sets the driving intention `resolved` (`valid_until`, `director.cpp:329`).
  2. **Converge** — the locus reaches the player → the **Weaver** merges it into the player's scene (the trackers burst in). The *payoff*, not a death (Phase 6).
  3. **Supersede** — an event elsewhere kills the premise (the player imprisons the spymaster mid-errand) → the Weaver's existing LLM supersession (`check_group`) sets `valid_until` → no live intention → the thread ends.

**The correction:** importance governs *pause* and *LOD* (mechanical); the narrator/Weaver governs *ending* (LLM). The earlier "destroy = importance decays below floor" was wrong — it would freeze a loaded, still-running thread the moment the player looked away.

### 5.4 The examples that forced this

- **Trackers hunt the player; player flees to town.** Importance must anchor on the intention's *target* (`Player` edge), not the woods, so the hunt stays hot and closes in. (Fixed a wrong "decays when player leaves the place" assumption.)
- **Two guards gossip.** No `Player` link → importance decays → *pause* (correct, mechanical).
- **The general marches on the rebels** → ticks to victory. Only the narrator knows it's *over* → *conclude inline*. (This is the case that proves destroy needs an LLM.)
- **Player imprisons the spymaster** whose off-stage errand is running → premise dies elsewhere → *supersede* via the existing Weaver path.

### 5.5 The one new mechanism

The autonomous beat's output carries a **thread status** so "conclude inline" has a channel:

```
beat → { prose, plan(new_nodes/transitions/...), thread_status: continuing | concluded | converging }
```

`concluded` → end the thread; `converging` → hand to the Weaver to merge; `continuing` → keep ticking. Spawn needs no new schema beyond a seed tag on the narrator's existing plan.

### 5.6 Still open

- **Long-dormant latent loci** the player never re-engages and the narrator never concludes: leave them free in the pool, or add an occasional LLM "janitor" to retire stale ones? (Lean: leave for now.)
- **A thread that should pay off though the player never looks at it** — is it covered by "converge toward the player," or does it need its own push? (Unexplored.)
- **Two off-stage threads colliding with *each other*** rather than with the player. (Unexplored.)
- Does the narrator's seed tag ever **force** an immediate thread, or always just create the latent locus that importance gates? (Lean: always latent + gated.)

---

## 6. Corrections from reading the source

The 06-09 plan and my first design draft both inherited an architectural map produced by a search agent. Reading the actual source corrected the following — recorded because they change the plan.

**Correction #1 — the Validator is already wired and live (was claimed missing).**
`app.py:480-484` builds `Validator(scene.world_graph)`, sets its LLM + ChromaDB-search + dead-check callbacks, and calls `director.set_validator(validator)`. Then `director.cpp:362-370` runs `validator_->check(node)` on **every** narrator-proposed `new_node`; a rejection is collected and the node skipped. **"Wire the Validator first" is not a task — it already gates the one current producer.**

**Correction #2 — the narrator is the sole world-author; the Director is an *applicator*.**
In the live loop the Director's own LLM path (`tick()`, `director.cpp:114`) is **never called**. The narrator LLM emits prose + a JSON plan (`scene_loop.cpp:558-565`), and `director.apply_planned_turn()` (`director.cpp:210`) applies it: `transitions` (state changes; `resolved` → `set_valid_until`) and `new_nodes` (each Validator-gated). The plan schema is the contract in `prompt.py:14-18` (`transitions` / `new_nodes` / `speech_turns` / `new_characters` / `active_cast`). **Consequence:** an autonomous scene = a narrator call with no player line, emitting the same plan. It reuses `narrator → split → apply_planned_turn → Validator → route_perception → actors` wholesale. The only new ingredient is a prompt variant.

**Correction #3 — the coherence gate already has a retry-on-contradiction loop.**
`scene_loop.cpp:570-638`: snapshot graph → apply plan → collect Validator + cast rejections → on rejection, restore + append `### REVISION REQUIRED` with reasons + re-prompt the narrator (up to 3 attempts). "Rejected facts force a rewrite" is real and working.

**Correction #4 — the Weaver does NOT compute importance today.**
`weaver.cpp` is (a) a content-based **edge editor** — connect/disconnect/reweight on a degree-biased ≈0-node *sample* (`weaver.cpp:52-171`), edges originating from an entity-overlap heuristic — and (b) an **entity-group supersession detector** (`rebuild`/`drain_expiry_queue`, `check_group`, `weaver.cpp:302-450`). No centrality, no character-importance, nothing cross-scene. Importance (§4) is a **new** computation — but cheap, pure-graph, and naturally Weaver-adjacent because the Weaver owns the edge weights it reads.

**Correction #5 — the real concurrency problem: whole-graph snapshot/restore.**
The current gate rolls back by snapshotting and restoring the **entire** `world_graph` (`scene_loop.cpp:574` `to_json()` / `:579` `from_json()`). With a *shared* graph and *concurrent* scene loops, one scene's rejection-rollback would clobber every other scene's just-committed facts. The multi-scene gate must become a **serialized, per-commit transaction**: apply this scene's plan as a unit, roll back only *this plan's* nodes on rejection. This is the engineering core of "Validator-as-serialization-point," and it is more than a relabel of what exists. (The minds side is covered by the exclusive-presence invariant: a character is on-stage in at most one scene, so only one loop writes a given mind at a time.)

---

## 7. Survey grounding (what the prior art says)

> Sourcing note: the Generative Agents and Dwarf Fortress points were verified against primary sources this session. The Left 4 Dead / RimWorld points are from established knowledge — their hosts blocked automated fetching — and are worth a verification pass if they become load-bearing.

- **Generative Agents (Park et al., 2023, [arXiv:2304.03442](https://arxiv.org/abs/2304.03442)) — verified.** The closest analog; our `CharacterMemory` is already a partial reimplementation (memory stream; retrieval = recency `0.995` decay × importance `1—0` × relevance cosine; reflection at an importance-sum threshold). **The piece we're missing is planning** — top-down plans, recursively decomposed, *replanned from the reaction point forward* on new observation. Plans exist *independent of perception* — that is the "forward intention" the engine lacks, and it is exactly what the autonomous driver supplies. **The cost lesson:** 25 agents × 2 days = "thousands of dollars," because calls were on the cloud critical path. Our equivalent runs on the local llama.cpp background lane → compute/latency, not dollars. **Failure mode to expect:** instruction-tuned models drift to over-cooperative, bland consensus when left to deliberate — so autonomous beats must be prompted *adversarially* (intention + obstacle), paired with the 06-09 episode's intent-vs-outcome adjudication.
- **Dwarf Fortress ([Wikipedia](https://en.wikipedia.org/wiki/Dwarf_Fortress)) — verified.** Fidelity follows attention: the active site is simulated in fine detail, off-site civilizations advance abstractly, and an entity is "promoted" to detail only when engaged. This is our LOD-by-importance + promotion-on-convergence, validated as a 20-year-old shipping pattern.
- **Left 4 Dead "AI Director" — prior knowledge.** Tension is a *managed scalar* (an intensity estimate) driving a build-up → peak → relax → rest cycle. Validates: a cheap signal (rising importance / an aging clock) is enough to pace without scripting.
- **RimWorld "Storyteller" — prior knowledge.** A points/threat budget tied to colony wealth, sampling weighted incidents on a pacing curve. Validates the **budget-admission** model for spawn (§5) over absolute thresholds.

---

## 8. What maps onto existing code (hooks)

| Component | Location | New/Mod | Job |
|---|---|---|---|
| `World` (shared pool) | `core/.../world.{h,cpp}` | new | owns `WorldGraph` + `map<name, CharacterMemory>` |
| `Scene` slim-down | `scene.{h,cpp}` | mod | drop owned graph/minds; add `World` ref + presence + LOD + clock |
| Transactional commit | `director.cpp`, `scene_loop.cpp:574-638` | mod | rollback only this plan's nodes, not the whole graph |
| Importance metric | `core/.../importance.{h,cpp}` | new | player-anchored weighted centrality + decay |
| LOD policy | with importance | new | importance → {narrator model, cadence, story-time step} |
| Autonomous driver | `scene_loop.cpp`, `prompt.py` | mod | player-less `advance()` + push prompt |
| `SceneManager` | `core/.../scene_manager.{h,cpp}` | new | owns scenes + threads + lifecycle + budget admission |
| Locus detector | in `SceneManager` / `weaver` | new | cluster graph (`entity_groups` + components) → spawn/evict/merge candidates |
| Cross-scene stitch | `weaver.{h,cpp}` | mod | merge loci on convergence + atomic presence migration |

---

## 9. Build plan — phased, each ships independently

Tracked as tasks Phase 0—.

### Phase 0 — Shared pool refactor
Mechanical, no behavior change, still one scene.
- [ ] Create `World`; move `WorldGraph` + `character_memories` out of `Scene`; `Scene` holds a reference.
- [ ] Fix `app.py` wiring + save/load.
- **Accept:** the tavern scenario runs identically to today.

### Phase 1 — Transactional coherence gate
- [ ] Replace whole-graph `to_json()`/`from_json()` rollback (`scene_loop.cpp:574, 579`) with per-plan node-id rollback (track ids added by this plan; remove only those on rejection).
- **Accept:** the contradiction-retry loop still works; one scene's rollback cannot touch another scene's facts.

### Phase 2 — Player-anchored importance + LOD
- [ ] Implement importance (weighted centrality from `Player` + decay), pure C++.
- [ ] Map importance → LOD tier (narrator model, cadence, story-time step).
- [ ] Surface importance on `/analyze` (or `/minds`) for inspection.
- **Accept:** on `siege`, Voss ranks high while his trackers link to `Player`, and the rank decays when the link is cut.

### Phase 3 — Autonomous narrator driver
- [ ] Player-less `advance()` path + push prompt variant: "no player acted; advance this storyline from the cast's intentions + tension."
- **Accept:** an off-player scene advances **one coarse beat** that lands valid facts in the shared graph (the N=1 Voss beat).

### Phase 4 — SceneManager + spawn/destroy lifecycle  → the hard part (reworked, see §5)
- [ ] Latent locus = off-stage entities + a live driving-intention fact, sitting free in the pool.
- [ ] **Spawn:** narrator tags seed fact(s)+entities in its plan (LLM authoring); the latent locus gets a worker thread *lazily* when its importance crosses the floor.
- [ ] **Pause/resume (mechanical):** stop ticking below the floor; state stays in the pool; resume with a catch-up beat. NOT destroy.
- [ ] **Destroy (LLM judgment) via three paths:** conclude inline (narrator's `thread_status: concluded`), converge (→ Weaver merge, Phase 6), supersede (Weaver `check_group` kills the premise).
- [ ] Add `thread_status: continuing|concluded|converging` to the autonomous beat output.
- [ ] Importance anchors on the intention's *target* (`Player` edge), not the place.
- **Accept:** the Voss/trackers scene auto-spawns from a seed fact, advances, pauses/resumes correctly when the player leaves, and ends only by conclude/converge/supersede — end-to-end, without the player naming Voss's entities.

### Phase 5 — Threads + scheduling
- [ ] One thread per Active scene; tick cadence set by LOD; all writes serialized through the transactional gate (single committer).
- [ ] Break the `join_background()` coupling (`scene_loop.cpp:512`) so off-player scenes advance across player turns.
- **Accept:** 2+ scenes advance concurrently with no graph corruption.

### Phase 6 — Cross-scene stitching
- [ ] Extend the Weaver to merge loci on convergence and migrate character presence between scenes atomically (exclusive-write invariant).
- **Accept:** the trackers scene converges into the player scene cleanly, one thread absorbed.

---

## 10. Open decisions

1. **Spawn-tag channel** — how the narrator marks a seed fact + entities in the existing plan schema (new optional field vs. a node flag).
2. **Importance floor** — single global floor vs. a hard cap N (run at most N threads, drop the lowest) as a cost backstop. (Cap is a one-line backstop, not the admission subsystem the first draft proposed.)
3. **Catch-up on resume** — frozen vs. one interim "what happened while you were away" beat across elapsed story-time (lean: the catch-up beat).
4. **Long-dormant janitor** — leave stale latent loci free in the pool, or occasionally LLM-retire them (lean: leave for now).
5. **Autonomous beat model** — cloud narrator (quality) vs. local model (cost) for off-player scenes. A *model* choice, distinct from the rejected simulation-LOD.
6. **Unexplored cases** — a thread that should pay off though the player never looks at it; two off-stage threads colliding with *each other*.

---

## 11. The N=1 proof, restated

End-to-end with the design above (validates Phases 0—):

1. **Seed.** Voss's existing intention "sent trackers into the woods" forms a low-importance locus → low-LOD candidate.
2. **Player goes elsewhere.** Each player turn advances story-time. The Voss locus's importance rises as its facts link, through the woods, toward `Player`. At low LOD the autonomous driver emits coarse beats — "trackers enter the woods," "trackers pick up the trail" — landing as facts in the shared graph through the transactional gate.
3. **Promotion / convergence.** Importance crosses the floor → a worker spawns and LOD rises → as the locus reaches the player, the Weaver stitches the trackers' scene into the player's (the autonomous beat's `thread_status: converging`) → the narrator renders, at full detail, "three of Voss's trackers step from the treeline, your trail clearly in hand."
4. **The world pushed.** The authored "good" plot fired **without the player naming Voss's entities** — the exact failure the 06-09 diagnosis identified.
