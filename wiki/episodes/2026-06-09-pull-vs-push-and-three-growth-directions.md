---
sources:
  - wiki/episodes/2026-06-08-freedom-tension-and-the-two-axes.md
  - wiki/research/narrator-and-mind-weakpoints.md
  - core/src/director.cpp
  - core/src/weaver.cpp
  - core/src/character_memory.cpp
  - server/scenarios/siege.json
last_updated: 2026-06-09
confidence: proposed
tier: semantic
related:
  - freedom-tension-and-the-two-axes
  - narrator-and-mind-weakpoints
  - subjective-character-minds
  - plot-graph
tags:
  - design
  - episode
  - narrator
  - world-simulation
---

# Episode — Pull vs. push, and the three directions the world can grow (2026-06-09)

A design session that started from a live symptom (`[weave] cloud LLM returned empty response`) and a
re-read of `siege.json`, and converged on a sharper diagnosis of the missing "tension layer" plus a
three-part architecture for a self-enriching world. No code was changed *by the discussion*; three
small unrelated fixes shipped alongside it (see the log entry of the same date).

## The diagnosis: the engine is 100% *pull*

All plot causation today runs one way — **player input → narrator**. Traced concretely:

- `Director::focus_payload_text` (`director.cpp:145`) seeds the narrator's context only from nodes
  whose entity/fact text literally appears in the recent scene text (+2-hop). So a `Foreshadowed`
  stake is **invisible to the narrator unless the player is already discussing its entities.**
- The narrator decides `transitions`/`new_nodes`; the Director rubber-stamps (`apply_planned_turn`).
  The narrator prompt frames it as scene-renderer + fact-extractor — it is told to *react*, not push.
- There is **no third source**: `trigger`/`arc_position` are dead (serialize-only, `node.cpp`); the
  Weaver pushes nothing plot-wise (edges + expiry only); off-stage minds are write-only.

So the authored dramatic spine (relic, Maren's wound, Voss's secret orders) is **inert pull-content
gated behind the player naming the right entities.** The 18-turn `siege` is the proof: every
foreshadowed thread is still `Foreshadowed` at turn 18 — not *destroyed* by the player, never
*activated* by the engine. Plot ≈f(player attention). The canonical evidence: Voss's mind holds the
intention *"I've sent trackers into the eastern woods"* while the player sits in those woods, and
**nothing happens** — a loaded gun no one fires.

## Reframing the "tension layer"

It is not a *layer*, it is the missing half of a *lifecycle*. We have one lifecycle owner working — +**resolution**: the Weaver ages facts out of liveness (`valid_until`) → freedom returns. The
symmetric half — **activation**: firing a ripe stake so it bites → pressure rises, freedom tapers — +has no owner. We built only the freedom-*returning* half, which is exactly why play drifts into
consequence-free wish-fulfilment. A **clock** (turns / story-time since a node was planted) is the
cheapest possible tension signal — deterministic, observable — and the symmetric twin of the Weaver's
expiry.

The single author (the narrator) can be made to push *without rules that force "interestingness"* — +by changing its **inputs + mandate**, not constraining its output: (L1) always-visible stakes,
annotated with age; (L2) a push mandate ("the world does not wait for the player; advance a thread");
(L3) a pacing/pressure signal so it escalates one beat at a time. Push needs a complement — +**intent-vs-outcome adjudication** — or the player just steamrolls; pressure without consequence is
noise.

## The three growth directions (the larger vision)

The graph today grows only **present-forward, reactively**. Three components open the other
directions:

1. **Parallel scenes — forward, autonomously.** A level-of-detail simulation: the player's scene runs
   at full resolution every turn; off-stage scenes (Voss's council, Aldric's crypt, Harren's army)
   run **coarse and infrequent** on a shared **story-time** clock (not player-turns), advancing each
   off-stage agent's *intention* against the world. **Collision is both the event-generator and the
   salience filter**: an off-stage thread surfaces precisely when advancing it intersects the player's
   scene (Voss's trackers reach the woods *because* the player lingered there — the dawdling itself
   spends the pursuit). Requires a capability the minds lack today: **off-stage deliberation** — minds
   forming/advancing forward intentions *absent new perception* (`reflect_perceptions` early-returns
   with no new perceptions, which is *why* the world freezes when unobserved). **N=1 test:** run this
   loop for Voss alone on `siege`; if the trackers arrive without the player mentioning her, the bet
   is proven.

2. **Backfill — outward, into depth.** Generate the context/history *around* existing facts so a
   sparse assertion becomes a grounded world. It is the operator that walks a world **along the
   freedom/density axis** (blank → dense spine). Tractable via **just-in-time** depth (deepen only
   what is touched — same LOD logic) and routing output **through the Validator** (generated history
   can't contradict established history). It is the inverse of the downsampler.

3. **Ingestion — backward, from imported canon.** Parse an existing book/anime/film into a dense world
   graph + characters (personas, beliefs, intentions) + plot spine. This is where the dead
   `arc_position`/`trigger` fields finally earn their keep. It sets the **initial constraint set =
   starting freedom level** (book = low/dense; blank = high/thin). The book is a **seed, not rails** — +   play diverges from canon via (1). Reuses the identity-authority model; a natural **offline, batch,
   multi-agent** job (chapter readers → fact/arc extractors → character profilers → assembler).

## The unifying architecture

The narrator, parallel-sim, backfill, and ingestion are all **fact producers feeding one shared world
graph through one coherence gate** (Validator + supersede/expiry). Today only the narrator feeds it;
the generalization is to give each fact a **provenance** and flow all of them through the same
validation. **The substrate barely changes — you add producers, not a new engine.** This recontextual-
izes recent "small" work (entity identity, validator, supersede/expiry): that *is* the coherence gate,
and it is the prerequisite plumbing for letting four producers write to one world instead of one.

## Sequencing (proposed)

Prove **(1)** on `siege` via the N=1 Voss/trackers test (validates the central bet — do emergent
off-stage events become arcs? — before any big refactor) → **(3)** to get a genuinely dense world
worth simulating → **(2)** as the connective tissue that fills (3)'s gaps and (1)'s thin spots.
(3)-first is defensible if simulating a real world beats a four-character sketch.

## Open questions for the planning chat

- The **provenance / intake-gate** design (the shared API all producers plug into).
- How much of the deferred **Session + multi-loop + `on_stage` refactor** (1) forces even at N=1.
- The **story-time model** (the spine everything hangs on) and the **salience/router** (what surfaces).
- Off-stage **deliberation** as the first concrete build — the world is inert without it.
