---
title: Pragmatic turn transaction refactor
last_updated: 2026-08-22
confidence: verified
tier: episodic
sources:
  - core/include/rhapsode/story.h
  - core/include/rhapsode/story_data.h
  - core/include/rhapsode/turn_contracts.h
  - core/include/rhapsode/turn_pipeline.h
  - core/src/story_advance.cpp
  - core/src/story_data_ops.cpp
  - core/src/turn_pipeline.cpp
  - core/src/turn_pipeline_narrator.cpp
  - core/src/turn_pipeline_post_turn.cpp
  - core/tests/test_scene.cpp
  - server/tests/test_bindings_api.py
  - wiki/research/frontier-llm-long-horizon-orchestration.md
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/cpp-data-model]]"
  - "[[research/frontier-llm-long-horizon-orchestration]]"
tags:
  - cpp-core
  - cross-layer
  - design
---

# Pragmatic turn transaction refactor

This plan moves Rhapsode from one narrator-authored response toward explicit character authorship and
consequence-first turns. It keeps the existing Story API and enables one authority change at a time.
It contains no obligation system and no general LLM validator.

## Target turn design

1. Preserve the player's exact input.
2. Read one versioned snapshot.
3. Make raw attributed transcript evidence available.
4. Let characters propose their final actions and dialogue.
5. Select only mechanically compatible proposals.
6. Declare durable consequences before prose.
7. Check enumerated mechanical preconditions.
8. Render narration around accepted actions and exact dialogue.
9. Atomically commit transcript and state.
10. Extract non-authoritative graph observations afterward.

This list defines execution order. It does not justify ten classes or ten model calls.

## Current status

| Step | Status | Exact boundary |
|---|---|---|
| 1. Exact player input | Implemented | Exact bytes enter the candidate transcript before generation |
| 2. Versioned snapshot | Implemented in process | One frozen read copy and one transaction version; not a durable journal |
| 3. Attributed evidence | Native path implemented | `query_transcript` is not advertised to the production narrator |
| 4. Character proposals | Diagnostic shape only | Proposals are adapted from narrator output after generation |
| 5. Mechanical selection | Not implemented | Existing narrator-plan checks remain live |
| 6. Consequences before prose | Not implemented | Prose and speech arrive before graph extraction |
| 7. Mechanical preconditions | Partial | Existing cast/new-character/lifecycle operations have narrow checks |
| 8. Render around exact dialogue | Not implemented | The narrator still authors and formats character lines |
| 9. Atomic transcript/state commit | Implemented in process | World and active scene commit together; maintenance/save do not |
| 10. Post-commit observations | Implemented | Observation graph is separate and cannot directly mutate mechanics |

## Structural foundation now in code

```text
Python -> Story -> execute_turn(StoryData&, TurnServices&, TurnInput)
                         |                 |
                     StoryData        TurnServices
```

- `Story` owns `StoryData`, `TurnServices`, and one optional `PendingTurn`.
- `execute_turn` is the only narrative-turn function for player and autonomous input.
- Candidate World and SceneData values are local variables, not wrapper objects.
- `apply_graph_plan` is a stateless function; no core `Director` class remains.
- `Weaver` retains service state but receives `WorldGraph&` for each operation.
- The live observation graph is separate from coded `World` state.
- Python compatibility names and the old save shape are isolated at their boundaries.

This foundation changes dependency direction and failure containment. It does not transfer authorship
or prove model reliability.

## Existing diagnostic records

`ActorProposal` records a proposed character action and exact line, its actor, optional evidence and
core version, the base transaction version, and its source. `TurnDecision` records accepted proposal
IDs, enumerated mechanical operations, canonical events, evidence, and the same base version.

The legacy adapter populates these records from the narrator response after commit. The records are
not persisted, exposed to Python, or allowed to change output. Missing evidence stays missing.

`TurnDecision` is the intended pre-prose consequence record. It is not an open-ended mutation patch:
only registered operation kinds may alter coded state, and an ordinary narrative turn may contain no
mechanical operations.

## Migration rule

Each remaining change follows the same progression:

1. observe it without authority;
2. measure it against the live path;
3. enable it for a small scenario allowlist;
4. retain one switch back to the live path;
5. remove the old branch only after sequential evaluation.

Optional telemetry must not require a save migration. A failed experiment is removed at its single
boundary rather than compensated for with story-specific rules.

## Phase A — expose attributed transcript retrieval

Advertise the implemented `query_transcript(scene_id, query)` dispatcher in
`server/rhapsode/llm_tools.py`. Return exact text, role, speaker, scene, turn, ordinal, and
`message_ref`. Keep summaries and graph results visibly separate from raw evidence.

Do not inject a full transcript every turn. The model requests a targeted query when current context
is insufficient.

Gate:

- duplicate text spoken by different characters remains distinguishable;
- a missing span is reported as missing rather than reconstructed;
- retrieval at turns 7, 25, 98, 200, and 287 returns the expected text and attribution;
- added tool availability does not increase irrelevant retrieval or prompt fixation beyond the
  prespecified limit.

Rollback: remove one tool schema entry. Stored data does not change.

## Phase B — shadow character proposals

Add one optional actor callback to `TurnServices`. For an allowlisted scene, invoke each eligible
speaker against the same frozen snapshot, that character's current core/mind, targeted transcript
spans, and the exact player input. The actor returns an `ActorProposal` or abstains.

Store these proposals as evaluation telemetry. The narrator output remains live.

Gate:

- actor, scene, base version, and optional core version match the snapshot;
- cited message references resolve to raw spans;
- malformed, late, or failed calls cannot mutate story data;
- at least 20 genuinely sequential player turns measure voice distinction, contradictions,
  abstention, latency, and cost.

Rollback: disable the optional callback. No production transcript or save field depends on it.

## Phase C — shadow proposal selection

Add one pure function:

```text
snapshot + proposals -> TurnDecision or rejection
```

It may check only enumerated facts:

- actor exists, is alive, and is present when presence is required;
- proposal and optional core versions match;
- referenced evidence exists;
- operation kind and arguments are registered;
- the corresponding World operation reports its coded preconditions satisfied.

It must not judge voice, dramatic quality, intent, or general natural-language consistency. No model
call approves another model call.

Start with selecting dialogue/actions and no mechanical operations. Add one operation only when a
current World method already defines its precondition.

Gate: identical inputs produce identical decisions, stale and invalid proposals leave no writes, and
the shadow decision can be compared with the committed legacy result for 20 sequential turns.

## Phase D — allowlisted consequence-first rendering

For one scenario only:

1. collect proposals;
2. select a `TurnDecision`;
3. apply registered operations to candidate values;
4. ask the narrator for connective prose around accepted content;
5. append each accepted `exact_dialogue` value directly;
6. commit transcript and coded state together;
7. extract observations afterward.

The renderer cannot add a coded mutation. A prose claim without a registered operation remains prose.
Structured response parsing may reject malformed output; no general semantic judge is added.

Gate:

- exact dialogue survives proposal, selection, rendering, save/load, and display byte-for-byte;
- every coded mutation appears in the decision before prose generation;
- failure injection at each stage leaves either the prior turn or one complete committed turn;
- the allowlisted path passes the 20-turn sequential evaluation before wider use.

Rollback: disable the scenario flag. Optional telemetry and existing save defaults keep both paths
loadable.

## Phase E — durable recovery

The current in-memory commit does not cover maintenance or the multi-file save. A later persistence
change should write one complete generation, validate it, and switch a manifest pointer only after
all files succeed. Keep the prior generation recoverable.

Treat each autonomous scene turn as a separate transaction. Treat observations, indexes, summaries,
and telemetry as rebuildable derivatives unless a later design explicitly promotes them.

Gate:

- failure at every write point recovers one complete generation;
- recovery reports the last committed transaction and discarded work;
- replay reconstructs coded state and transcript references;
- deleting and rebuilding observations cannot change replayed mechanics.

## Evaluation gates

| Horizon | Purpose | Authority decision |
|---|---|---|
| 20 sequential player turns | Integration, prompt-loop, and immediate authority failures | Required before allowlisted live use |
| 100 sequential player turns | Voice, causal, evidence, and coded-state survival | Required before default use |
| 300 sequential player turns | Long-horizon survival under unrestricted input | Required for a 300-turn claim |

Track first failure turn, survival at each horizon, evidence-recovery rate, stale-proposal rejection,
partial commits, recovery success, dialogue alteration, graph contamination, latency, and cost.
Completion, enjoyment, page count, scene count, aggregate dialogue turns, and unvalidated LLM-judge
scores are not reliability evidence.

## Stop conditions

Stop an authority transfer if raw evidence cannot be recovered exactly, rejected work leaves a write,
accepted dialogue is changed or misattributed, an undeclared prose claim becomes a coded mutation,
graph deletion changes replayed mechanics, or the new path worsens its target failure rate.

Disable or revert that phase. Do not add an obligation prompt, general validator, or story-specific
exception to hide the failure.

## Unproven claims

No existing test establishes that:

- character-authored proposals remain coherent and distinct;
- targeted retrieval consistently finds the needed evidence;
- selection composes several reasonable actors;
- declared consequences match free-form narrative meaning;
- connective prose preserves accepted content;
- rollback removes downstream semantic contamination;
- any combination survives 100 or 300 player turns.

These claims require the program in [[research/frontier-llm-long-horizon-orchestration]].

## See also

- [[architecture/system-overview]]
- [[architecture/scene-loop]]
- [[architecture/cpp-data-model]]
- [[research/frontier-llm-long-horizon-orchestration]]
