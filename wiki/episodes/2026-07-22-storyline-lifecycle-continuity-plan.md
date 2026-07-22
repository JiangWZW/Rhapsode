---
title: Storyline lifecycle continuity plan
date: 2026-07-22
tags:
  - story
  - lifecycle
  - llm
---

# Storyline lifecycle continuity plan

## Implementation status

Implemented on 2026-07-22. Fork synthesis, owned-intention expiry, compact closures, lifecycle read
tools/verdict validation, detached Python scene values, and manifest-scoped save cleanup are live.
Topology-aware whole-Story undo remains the explicitly deferred follow-up described below.

## Goal

Make fork and conclusion preserve narrative continuity and domain invariants without introducing a
new lifecycle subsystem. `Story` remains the mutation boundary. Each operation follows one rule:

> Validate first, synthesize only when new prose is needed, then commit once.

This extends the transaction shape already used by narrator-synthesized scene merge.

## Scope

- A fork receives a compact, narrator-authored starting context derived from its parent.
- A conclusion persists why and where the storyline ended, and expires the intention owned by it.
- Public domain methods enforce cast, Player, and last-storyline invariants.
- Lifecycle judgment can inspect the same read-only history, graph, and mind tools as scheduling and
  narration.

This change does **not** add a second narrator call for conclusion, a generic workflow framework, or
full topology-aware undo. Whole-story undo is a separate follow-up because it changes the meaning
and persistence of the existing active-scene undo command.

## 1. Share one continuity input shape

Generalize the current merge context builder into a private scene-continuity representation:

- scene ID, title, driving intention, and cast;
- rendered downsampled `story_so_far`;
- a bounded chronological tail of narration and dialogue.

Merge continues to consume two such contexts. Fork consumes the parent context plus the validated
departing cast and proposed intention. Both narrator calls retain the existing call-scoped read
tools: `query_history`, `query_graph`, and `query_mind`.

This is a refactor of the existing merge payload, not a new public abstraction.

## 2. Make fork a transactional narrator operation

Add `TurnExecutor::synthesize_fork_context(...)`. Its prompt asks the narrator to isolate the
departing characters' relevant continuity without advancing time or inventing a beat, returning
only:

```json
{"fork_story_so_far":"..."}
```

`Story::fork_scene` then performs these steps in order:

1. Validate parent/new IDs, a non-empty intention, and a unique non-empty cast.
2. Resolve every requested name to a living, non-Player member of the parent scene.
3. Reject a split that would leave the parent with no cast.
4. Call the narrator with the parent continuity, selected cast, intention, and read tools.
5. Parse and validate a non-empty `fork_story_so_far`.
6. Prepare the child with the parent's title/system prompt and the synthesized text as its
   downsampled context; do not duplicate the parent's verbatim history or dialogue.
7. Seed the child-owned intention, move the cast, and publish the child as one commit.

Any validation, callback, or parse failure leaves the parent, World, and scene collection unchanged.
The first autonomous beat therefore starts with the departure context while retaining its own clean
transcript and turn index.

## 3. Give a fork-owned intention an identity

Change `World::seed_character_intention` to return the new subjective-memory node ID (`0` on
failure). Persist the owner character and node ID on the child `SceneData` alongside its display
intention.

Add a narrow World command to expire that exact node. Do not expose mutable character-memory graphs
or infer ownership later by matching intention text. Use the Story beat clock for new lifecycle
timestamps so repeated forks do not all appear to originate at zero.

Older saves simply load with no owned intention ID; their conclusions remain valid but have nothing
specific to expire.

## 4. Make conclusion a durable retirement

Do not call the narrator again. The beat that triggered conclusion already authored the ending.

Before mutation, `Story::conclude_scene` validates that:

- the reason is non-empty;
- the scene exists and contains no Player;
- at least one other live scene will remain.

It then records a compact `SceneClosure` in the Story manifest containing the scene ID, reason,
cast, driving intention, rendered story-so-far, final narration, and conclusion beat. It expires the
scene-owned intention, clears membership, and retires the scene. Save/load preserves the closure,
and save removes only scene files named by the previous manifest that are no longer live.

This retains an audit trail without keeping an inactive `SceneData` or its full transcript in the
live scheduler set.

## 5. Harden lifecycle judgment

Change `LifecycleCallback` to receive a call-scoped read-tool function, matching the scheduler. The
plain beat payload should also include the dialogue emitted by the completed beat; applied graph
effects can be inspected through tools.

Validate the verdict before applying it:

- merge and conclude are mutually exclusive with every other operation;
- fork and exit may coexist only for disjoint characters;
- unknown characters, empty intentions, unknown merge targets, and Player moves are invalid;
- `other_storylines` excludes the scene being judged.

An invalid or contradictory verdict is a logged no-op. The public `Story` methods repeat the
important invariants so direct C++ and Python calls cannot bypass policy checks.

## 6. Binding and save hygiene

- Return detached `SceneData` values from Python `get_scene`, `active_scene`, and `fork_scene` so a
  retained Python object cannot dangle after merge or conclusion.
- Teach `save()` and `delete_save()` to use the prior manifest's scene IDs when removing retired
  scene files; do not glob arbitrary JSON files.
- Update `server/verify_fork.py` so its scripted narrator distinguishes normal turns, fork
  synthesis, and merge synthesis.

## Implementation order

1. Characterize current failures and add strict domain-validation tests.
2. Extract the shared continuity payload and implement fork synthesis with failure atomicity.
3. Track/expire the owned intention and persist `SceneClosure` during conclusion.
4. Add lifecycle read tools and verdict consistency checks.
5. Make Python scene reads detached and clean stale save files.
6. Run native Release/Debug tests, Python tests, Ruff, and the standalone lifecycle verifier.

Each step should leave merge behavior and the existing save schema backward-compatible.

## Acceptance criteria

- A successful fork's first autonomous narrator call sees the synthesized departure context.
- A failed fork narrator call changes no scene, membership, or character memory.
- Invalid/unknown/Player cast members and empty intentions cannot create a child.
- Conclusion cannot remove a Player-bearing or final live scene.
- Conclusion expires exactly the fork-owned intention and survives save/reload as a compact closure.
- Contradictory lifecycle verdicts are rejected rather than resolved by branch order.
- Retained Python scene values remain safe after their native scene is retired.
- Retired scene JSON files are removed without touching unrelated files.

## Deferred follow-up: lifecycle-aware undo

The current undo truncates one active transcript and reverts graph turns; it cannot reconstruct a
retired scene or undo membership transfer. Address that with one whole-Story snapshot per player
advance (scenes, World, active ID, beat clock, and closures), designed and tested as a separate
change. Do not bolt inverse operations onto individual fork/merge/conclude branches in this patch.
