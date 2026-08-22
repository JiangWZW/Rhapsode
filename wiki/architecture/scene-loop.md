---
title: Turn execution
last_updated: 2026-08-22
confidence: verified
tier: semantic
sources:
  - core/include/rhapsode/story.h
  - core/include/rhapsode/story_data.h
  - core/include/rhapsode/story_data_ops.h
  - core/include/rhapsode/turn_pipeline.h
  - core/src/story_advance.cpp
  - core/src/turn_pipeline.cpp
  - core/src/turn_pipeline_narrator.cpp
  - core/src/turn_pipeline_post_turn.cpp
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/pragmatic-turn-transaction-refactor]]"
  - "[[architecture/cpp-data-model]]"
tags:
  - cpp-core
  - cross-layer
---

# Turn execution

Rhapsode has one narrative-turn function:
`execute_turn(StoryData&, TurnServices&, const TurnInput&)`. `TurnInput::Kind` selects player or
autonomous input; both paths use the same staging and commit logic. `Story` sequences this function
with later maintenance but does not forward through another executor object.

## Direct call structure

```mermaid
flowchart LR
    Advance["Story::advance_player"] --> Execute["execute_turn"]
    Complete["Story::complete_turn"] --> Execute
    Execute -.-> Data["StoryData&"]
    Execute -.-> Services["TurnServices&"]
    Execute --> Narrator["narrator callback"]
    Execute --> GraphPlan["apply_graph_plan"]
    Complete --> Post["process_post_turn"]
    Complete --> Lifecycle["apply_lifecycle_decision"]
```

`advance_player` rejects a new input while a prior turn still awaits `complete_turn`. After a
successful commit it stores only the scene ID, exact player input, and post-turn index in
`PendingTurn`.

## One `execute_turn` call

The implementation order in `core/src/turn_pipeline.cpp` is:

1. Reject re-entry and resolve the requested live scene.
2. Copy the live World, observation graph, active SceneData, and mutable service fields needed for
   rollback.
3. Build a `ReadToolLease` from a copy of World, observations, every live scene, and scene summaries.
4. Append the exact input to the candidate scene. Autonomous input is labeled `director_cue`; player
   input is labeled `player`.
5. Build the narrator prompt and run the existing narrator response/retry path.
   Phase A system is stage craft plus the speech/cast JSON schema. The user sheet is who is on
   this stage (with voice), who is not, other live threads, story so far, then the last attributed
   Player / Narrator / character spans from history **and** dialogue. Phase B (`GRAPH_UPDATE`)
   receives only this take's prose and speech.
6. Apply accepted legacy cast additions to the candidate World.
7. Stage narrator prose and formatted character messages in the candidate scene and `TurnResult`.
8. Confirm that `StoryData::transaction_version` still equals the captured base version.
9. Move the candidate World and scene into `StoryData` and advance the version once.
10. Notify the output callback. Notification failures are recorded; the committed turn is not replayed.
11. Extract and apply non-authoritative graph observations on copies of the committed values.
12. Build the inert `LegacyTurnShadow` for diagnostics.

An exception before step 9 restores World, observations, scene, version, resume state, storyline
board, and timing counters. This is an in-process turn transaction, not crash-safe persistence.

## Snapshot reads

`make_frozen_story_read_tools` creates owned copies before generation. The callback may dispatch:

| Tool | Source |
|---|---|
| `query_graph` | Frozen `StoryData::observations` |
| `query_mind` | Frozen `StoryData::world` |
| `query_history` | Frozen narrator/history buffer |
| `query_transcript` | Frozen merged history and attributed-dialogue buffers |
| `list_scenes` | Frozen scene summaries |

`ReadToolLease::close` invalidates copied callback handles. The callback cannot become a long-lived
pointer into Story state.

The native `query_transcript` result preserves exact message content, role, speaker, scene, turn,
ordinal, and `message_ref`. The default narrator beat already uses that attributed timeline (last
16 spans). The Python tool schema still does not advertise `query_transcript`; keyword
`query_history` remains history-buffer only.

## Message identity

Committed turn messages carry:

| Metadata | Value |
|---|---|
| `turn` | Scene-local turn being completed |
| `turn_ordinal` | Input `0`, narrator `1`, characters `2...` |
| `message_ref` | `<scene>:v<transaction-version>:<kind>:<ordinal>` |
| `scene_kind` | `player`, `director_cue`, `narrator`, or `character` |
| `speaker` | Character identity on attributed dialogue |

References are deterministic within a committed turn and need no mutable global ID counter.

## Observation step

Graph extraction runs after transcript and coded-state commit. It asks the narrator callback for
`transitions` and `new_nodes`, then calls the stateless `apply_graph_plan` function over a copied
`WorldGraph`. New nodes may be routed into copied character memories as perceptions.

On success, the updated observations and routed perceptions replace their committed copies. On
failure, the pre-extraction World, scene, and observations are restored while the committed turn
survives. The graph-application function cannot access Story, SceneData, or mechanical World methods.

The observation step is derived semantic work. It does not increment the transaction version and is
not part of an all-or-nothing save transaction.

## `complete_turn`

`Story::complete_turn` consumes `PendingTurn` and then performs:

1. graph weaving and expiry;
2. character monologue updates;
3. text downsampling;
4. lifecycle decision request and deterministic application;
5. turn-clock advancement;
6. selection and execution of up to two off-stage scene turns;
7. saving when configured.

Every selected off-stage scene calls the same `execute_turn` function with
`TurnInput::Kind::Autonomous`, then receives its own maintenance pass. A failure in one off-stage turn
is logged and does not replay the player turn.

## Present authority

The live path remains narrator-authored:

- the narrator produces prose and `speech_turns`;
- character lines are not separate actor calls;
- `ActorProposal` and `TurnDecision` are reconstructed afterward for comparison only;
- cast operations are validated legacy operations, not a general consequence declaration;
- graph nodes remain observations rather than mechanical facts.

The transaction structure contains several failure modes but does not establish long-horizon story
reliability.

## Limitations

- Output delivery occurs after commit and before graph extraction; clients must treat delivery errors
  as retryable notification failures, not failed turns.
- Maintenance and saving occur after the turn transaction and can fail independently.
- The base-version check assumes one synchronous writer; it is not a concurrent transaction manager.
- Narrator retry validation checks the existing enumerated plan shape, not semantic truth or character
  quality.
- No sequential evaluation shows that the current path preserves voice or causality for 100 or 300
  player turns.

## See also

- [[architecture/system-overview]]
- [[architecture/cpp-data-model]]
- [[architecture/pragmatic-turn-transaction-refactor]]
