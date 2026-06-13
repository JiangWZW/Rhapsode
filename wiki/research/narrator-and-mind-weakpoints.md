---
sources:
  - server/scenarios/siege.json
  - server/rhapsode/prompt.py
  - core/src/director.cpp
  - core/src/scene_loop.cpp
  - core/src/character_memory.cpp
  - core/include/rhapsode/node.h
last_updated: 2026-06-08
confidence: verified
tier: semantic
related:
  - character-system
  - subjective-character-minds
  - plot-graph
  - scene-loop
tags:
  - analysis
  - narrator
  - memory-architecture
---

# Weak points: the narrator has no spine, and the minds don't drive the story

Findings from a critical read of an 18-turn `siege` play session (`saves/siege.json`) cross-referenced
against the engine. The prose quality is high; the *system* underneath has structural gaps. Two are
deep (narrator/Director); two are bugs. Listed worst first.

## What the session exposed (evidence)

In 18 turns the player cursed Warden Voss into braying, instantly cured a fatal wound, *declared*
himself an "arch-magician," made the garrison submit, walked out, and narrated a peaceful month — +with **zero resistance** at any step. Consequences visible in the save:

- The premise (Lord Harren's army "two days out") never attacks. It appears at `t16` and by `t17`
  "Lord Harren's army now occupies Ashenmoor" — resolved **off-screen via the player's timeskip**,
  never played.
- Foreshadowed threads never pay off: `Aldric knows Harren seeks a relic` (t0), `Voss has secret
  orders` (t0), `Voss will brand them deserters` (t7) are **still `foreshadowed` at turn 18**.
- The antagonist is removed by one spell at ~t5 and never acts again.

## Problem 1 — There is no Director; the narrator is an unguided yes-and engine (DEEP)

The `Director` class is not a dramatist. It is three mechanical services, none of which exert
narrative pressure:

1. **`focus_payload_text` (`director.cpp:145`) — a passive context courier.** It BFS-seeds from
   entity matches in the scene text and emits the relevant Active+Foreshadowed facts as a flat list
   (`[id] state type "fact" -- ctx`). It does **not** mark which foreshadowed nodes are overdue,
   inject stakes, or signal "advance this." The narrator receives a neutral fact list it is free to
   ignore — and does.
2. **`apply_planned_turn` (`director.cpp:210`) rubber-stamps.** The narrator's JSON decides the
   `transitions` and `new_nodes`; the Director just applies/validates them. The narrator authors its
   own state changes — so it never has to advance a thread it doesn't feel like advancing.
3. **The trigger engine does not exist.** `Node.trigger` and `Node.arc_position` are **dead fields** — +   grep shows they are only (de)serialized in `node.cpp`, never evaluated anywhere. The planned
   dormant鈫抋ctive trigger-predicate system was never built, so dormant/foreshadowed nodes never
   auto-fire on conditions. Threads accumulate and rot.

Meanwhile the narrator prompt (`prompt.py` `FORMAT_AND_RULES`) frames the model purely as a
**scene-renderer + atomic-fact-extractor**. Its only plot-adjacent instructions are "May foreshadow
options" and "never narrate unperformed Player actions." There is no instruction to escalate,
introduce complications, enforce stakes, give the antagonist initiative, or converge foreshadowing
toward payoff.

Net: all narrative authority lives in the narrator LLM, which is told to render the player's wishes
and extract facts — not to direct a story. So a capable player steers trivially into consequence-free
wish fulfilment, and the "siege" becomes a cozy fishing-and-bread slice-of-life. There is no stakes
clock, no antagonist scheduler, no payoff pressure, and no constraint on the player authoring the
world or skipping time.

## Problem 2 — The minds are write-only; they cannot drive the story (DEEP)

Off-stage characters' evolving beliefs and intentions never reach the narrator. Voss's mind at turn
18 holds *"I've sent trackers into the eastern woods—* — active pursuit — yet **no tracker ever
appears in the narration.** Aldric's entire relic plot sits in his head and never surfaces.

Mechanism: the narrator prompt's character context comes from `build_inner_states`
(`scene_loop.cpp`), which iterates **`on_stage` characters only**, and the Cast list is active-only.
The Director plans from the *world graph*, never from characters' minds. So once a character leaves
the stage, their mind is sealed off from the story engine — it keeps updating but can never *act*.
The subjective minds we built are, for anyone not currently present, decorative.

These two problems share one root: **planning reads only the omniscient world graph, and the world
graph records only what *happened* — never what *should pressure the player next* nor what *off-stage
characters intend*.** Giving the Director teeth (antagonist agency + foreshadow-payoff pressure, fed
partly by the minds) is the highest-leverage work in the project.

## Problem 3 — `self_state` absorbs events the character never witnessed (BUG)

Voss's `self_state` reads: *"The potion's bitter tang clings to my tongue—Maren's breath is
shallow—The Player thinks he's spared—* — that is the **tent cure scene**, performed by the Player
alone with Maren outside the walls. Voss was not present, yet her inner monologue absorbed it in
confused first person.

Cause: `update_self_state` is fed `build_scene_context()` — the **shared omniscient narration** — +rather than the character's own perceptions. So the omniscient stream leaks straight into every
on-stage mind's self-state. This is the world-graph leak reborn one layer up, and it directly
violates the subjective-minds design. Fix: feed `self_state` only from the character's perceived
facts (their routed perceptions / own beliefs), never the shared scene context.

## Problem 4 — Belief/memory quality bugs (BUG)

- **`"My belief:"` prompt-echo.** Stored beliefs literally begin `"My belief: The Player is a
  festering wound—` across all three minds. `reflect_perceptions`' prompt ends with `My belief:` and
  the model echoes the label into the content. Strip the echoed prefix (and any leading role label)
  from the reflection output before storing.
- **Beliefs are never retired by reality.** Maren is healed at `t17` ("regained full strength"), yet
  `"My wound from Thornfield is getting worse"` is still a **live** belief. Supersession only fires
  when reflection forms a new belief about the *same subject*; reality moving on does not retire a
  stale belief. Minds carry contradictions indefinitely.
- **Beliefs read like narration, not cognition.** e.g. *"The garrison's fractures—are a greater
  threat than any external enemy, and—* — reflection inherited the narrator's purple voice instead
  of producing terse, atomic belief. Constrain the reflection output (length/style).

## Lower-severity polish (observed, not deep)

- Narrator meta leaked into prose once: a turn opened *"I'll generate the narration and JSON
  now.The fire pops—* — the model's preamble was not stripped.
- Cross-character pronoun error: Maren calls Voss "he" though Voss is she/her everywhere else — the
  actor prompt carries the *speaker's* gender but not the *referenced* character's.
- Dialogue format drift across the session (`*(asterisks)*` early, `(parens)` + quotes later) — the
  format fix landed mid-session, so the transcript is inconsistent.

## Through-line and priority

Problems 1, 2, and 3 are the same flaw seen from three angles: **planning and self-state both draw on
the omniscient world/narration stream, with no dramaturgical layer and no channel for off-stage
intent.** Attack the Director (stakes/antagonist agency/payoff pressure, drawing on the minds) first;
knock out the self-state leak and the `My belief:`/staleness/verbosity bugs as quick, independent
fixes.
