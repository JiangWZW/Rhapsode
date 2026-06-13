---
sources:
  - wiki/research/narrator-and-mind-weakpoints.md
  - core/src/director.cpp
  - core/src/validator.cpp
  - core/src/scene_loop.cpp
  - core/src/character_memory.cpp
  - server/rhapsode/prompt.py
last_updated: 2026-06-08
confidence: proposed
tier: semantic
related:
  - narrator-and-mind-weakpoints
  - subjective-character-minds
  - plot-graph
  - narrative-philosophy
tags:
  - design
  - episode
  - narrator
---

# Episode — Freedom, consequence, and the two axes (design session, 2026-06-08)

A design discussion that started from the [narrator weak points](../research/narrator-and-mind-weakpoints.md)
(no spine; off-stage minds inert) and converged on a model for *how a player's freedom and the world's
agenda should coexist*. No code was changed by the discussion itself; it sets direction. The two
self-contained bugs it surfaced were fixed immediately after (see the log entry of the same date).

## The core model: two axes, only one of which the engine has

- **Coherence** — what is *true and live*. This is the `WorldGraph`; the engine serves it well
  (validator, memory, chaining).
- **Tension** — what is *at stake and unresolved*. Barely represented. Nothing in the data model
  encodes force, stakes, or what *wants* to happen. The engine is a consistency engine, not a story
  engine.

## Principles we landed on

1. **Freedom is the complement of the live constraint set.** A blank scenario (e.g. `siege`) starts
   nearly unconstrained → high freedom; a book-based scenario starts dense → low freedom. Same engine,
   different initial density. Freedom *tapers automatically* as the world fills in, and `valid_until`
   is the throttle — resolved constraints return freedom.
2. **The fine line is intent vs. outcome, not possibility vs. constraint.** The player owns the
   *attempt* (never refused); the world owns the *result*. Outcomes run
   `yes-and → yes-but → no-and → incoherent`, biased hard toward the top; every "no" carries an "and."
3. **A legitimate "no" is the narrator consulting an authority:** continuity (graph), capability
   (sheet), or another mind (the minds). The fourth case — authoring world/time ("a month passed") — +   is **reconciliation, not refusal**: granted, *and here is what it cost* (the keep fell while you
   fished).
4. **Free declarations must crystallize.** Act-1 improvisation ("I'm an arch-magician") is fine while
   the world is blank — but once accepted it should harden into binding capability + a ceiling +
   exploitable cost. The bug was never locking it.
5. **The arc breathes, emergently:** tension accumulates → climax (max constraint) → resolution frees
   space → new act. Not scripted; it falls out of the constraint lifecycle.
6. **Two owners, clean split:**
   - **Validator = coherence lifecycle** — entry (contradict) + resolution (supersede). Governs what
     is true and live → therefore when freedom *returns*.
   - **Director = tension lifecycle** — generation + payoff pressure. Governs what is at stake → +     therefore the dramatic *curve*.
   Freedom is the inverse of live *coherence*; the curve rides on *tension*. **Conflating the two axes
   is why a naive "freedom ∝1/node-count" auto-taper underperforms:** a graph accretes coherence
   (trivia, texture), while a competent player *destroys* tension (resolves every threat). The two
   move in opposite directions — the `siege` save is the proof (44 nodes, zero tension). The curve
   must ride on *open dramatic pressure*, and that only rises if the world actively *generates* it.
7. **graph density = world spine.** A sparse graph is a weak constraint set is a pushover world. So
   "sparse extraction" and "narrator has no spine" were the same problem.

## Six symptoms collapse into three levers

- *No spine* + *minds don't drive* + *sparse graph* → the **missing tension layer** (+ density).
- *self_state absorbs unwitnessed events* + *beliefs never retire* → the **missing supersede
  adjudication** (same principle, two places).

## Improvement levers, by cost / leverage

**Cheap, independent bugs (fixed this session):**
- `self_state` must fold the character's *own perceptions*, not the shared narration (the leak that
  made Voss "taste the potion" she never saw).
- Strip the `"My belief:"` echo from reflected beliefs; constrain reflection output length/voice.

**Small, high-leverage keystone (next):**
- **Validator gains a third verdict: `supersede`.** Today accept/reject; add "this new fact *retires*
  that live one" → `valid_until`. Hands freedom back (the breath), and the *same* principle retires
  stale beliefs in the minds. Reuses the context-gathering it already does.

**The real build (owns the curve):**
- A **Director that generates tension:** a *clock* on foreshadowed/threat nodes (overdue-ness) and a
  per-turn **"world advances"** step that spends off-stage minds' intentions + overdue threads into
  events, fed to the narrator as a *first-class input it must reconcile* — not optional flavor. Reuses
  the minds; produces the tension the curve needs.
- **Capability crystallization** + **intent-vs-outcome adjudication** so the narrator stops
  auto-succeeding.

**Policy on top:**
- A **freedom dial** — one scalar tuning how hard the live set bites. Optionally drifting down, but
  driven by *tension*, not node count.

## Sequencing
Land the two bugs + the validator `supersede` verdict first (small; `supersede` fixes a world bug and
a mind bug at once and is the hinge the breathing arc depends on). Treat the Director-as-tension-layer
as the next real project — it is the thing that changes how the game *feels*; the dial sits on top
once tension exists to tune.
