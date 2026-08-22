---
title: C++ runtime data model
last_updated: 2026-08-22
confidence: verified
tier: semantic
sources:
  - core/include/rhapsode/story.h
  - core/include/rhapsode/story_data.h
  - core/include/rhapsode/story_data_ops.h
  - core/include/rhapsode/story_lifecycle.h
  - core/include/rhapsode/turn_contracts.h
  - core/include/rhapsode/turn_pipeline.h
  - core/include/rhapsode/scene_data.h
  - core/include/rhapsode/world.h
  - core/src/story.cpp
  - core/src/story_advance.cpp
  - core/src/story_data_ops.cpp
related:
  - "[[architecture/scene-loop]]"
  - "[[architecture/system-overview]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/memory-system]]"
tags:
  - cpp-core
  - cross-layer
---

# C++ runtime data model

The runtime uses composition and explicit data flow. `Story` owns two aggregate values and passes
them to free operations. No orchestration class inherits from another, and no turn object retains a
reference to live story state.

## Ownership graph

```mermaid
flowchart TD
    Session["Python WsSession"] -->|"owns"| Story
    Story -->|"owns"| Data["StoryData"]
    Story -->|"owns"| Services["TurnServices"]
    Story -->|"owns while maintenance is pending"| Pending["PendingTurn"]
    Data -->|"owns"| World
    Data -->|"owns"| Observations["WorldGraph observations"]
    Data -->|"owns stable allocations"| Scenes["SceneData[]"]
    Data -->|"owns"| Closures["SceneClosure[]"]
    World --> Roster["Character[]"]
    World --> Minds["CharacterMemory map"]
    Services --> Weaver
    Services --> Memory["shared MemorySystem"]
    Execute["execute_turn"] -.->|"one-call borrow"| Data
    Execute -.->|"one-call borrow"| Services
```

The `unique_ptr<SceneData>` elements keep scene addresses stable for the existing C++ API. They do
not create a second owner or a scene-to-Story back-reference.

## Owned aggregates

### `StoryData`

`StoryData` holds the values that describe a playthrough:

| Field | Meaning |
|---|---|
| `world` | Roster, scene membership, life status, and character memories |
| `observations` | Non-authoritative shared narrative graph |
| `scenes` | Live scene records with stable allocation |
| `scene_closures` | Compact records for concluded scenes |
| `active_scene_id` | Scene receiving player input |
| `turn_clock` | Cross-scene scheduling clock |
| `transaction_version` | Monotonic version for committed turn and lifecycle mutations |

The transaction version is not a content hash. Post-turn graph maintenance, downsampling, and service
state do not each advance it.

### `TurnServices`

`TurnServices` holds execution dependencies and runtime state:

- model, narrator, delivery, scheduler, lifecycle, reflection, and downsampling callbacks;
- history-window and resume settings;
- temporary storyline-board and timing counters;
- the stateful `Weaver` service;
- shared `MemorySystem` ownership;
- save-directory configuration;
- a re-entry guard flag.

It contains no `World*`, `SceneData*`, or `WorldGraph*`. `Weaver` receives the graph explicitly for
each operation.

### `Story`

`Story` is a compatibility façade and composition root. Its only owned state is `StoryData`,
`TurnServices`, and an optional private `PendingTurn`. It exposes setup, queries, lifecycle commands,
turn sequencing, undo, and persistence.

`Story` does not contain scene algorithms, scheduling algorithms, graph-application logic, or a
turn-executor object. Those operations accept explicit data arguments.

## Turn values

| Type | Lifetime | Purpose |
|---|---|---|
| `TurnInput` | One call | Player/autonomous kind, scene ID, exact input text |
| `TurnResult` | Return value | Committed outputs, versions, delivery failures, graph effects, audit shadow |
| `PendingTurn` | Between `advance_player` and `complete_turn` | Scene, exact player input, post-turn index |
| `ActorProposal` | Diagnostic result | Typed view adapted from legacy narrator speech |
| `TurnDecision` | Diagnostic result | Typed view of accepted proposals and legacy mechanical operations |
| `LegacyTurnShadow` | Diagnostic result | Proposals, decision, graph plan, and adapter issues |

There are no `TurnWork`, `PreparedTurn`, or `CompletedSceneTurn` wrapper layers. Candidate World,
scene, prompt, narrator result, and output values are local variables inside `execute_turn`.

## Functional modules

| Module | Responsibility |
|---|---|
| `turn_pipeline.*` | Stage, version-check, commit, deliver, and observe one narrative turn |
| `story_data_ops.*` | Scene lookup, summaries, frozen reads, cast resolution, scheduling selection |
| `story_lifecycle.*` | Fork, merge, conclude, exit, and apply parsed lifecycle decisions |
| `graph_plan.*` | Apply parsed node transitions/additions to an explicit observation graph |
| `scene_history.*` | Append, order, query, and revert transcript messages |
| `story_serialization.cpp` | Convert between the live split representation and the old save shape |

`apply_graph_plan` is stateless. `Weaver` remains a class because it owns a callback, random generator,
interval, stop flag, and expiry queue. `MemorySystem` remains a class shared with Python integration.

## World and observation compatibility

A live `StoryData` does not keep its observation graph inside `World`. The split is:

```text
StoryData::world         = coded roster and character state
StoryData::observations  = fallible shared narrative observations
```

`World` still serializes a graph and `state_version` for compatibility with existing standalone C++
and Python callers. `import_world` moves those fields out when a Story is created or loaded.
`snapshot_world` copies them back into a detached World for saving and Python inspection. The live
Story never exposes a mutable `World&`.

## Mutation rules

- `execute_turn` is the only narrative-turn entry point for both player and autonomous turns.
- Lifecycle commands mutate `StoryData` through named free functions and advance the transaction
  version when they change authoritative state.
- Read tools operate on copied snapshots and retain no live pointers after their lease closes.
- Graph application receives only `WorldGraph&`; it has no access to Story, scenes, or coded World
  mutation methods.
- Python receives copied SceneData and World values. Native mutation goes through `Story` commands.

## Limitations

- `StoryData` is an aggregate but not trivially copyable because scene allocations use `unique_ptr`.
- `story_data_ops.*` groups several small read/policy helpers; splitting it further would add module
  boundaries without removing ownership or authority.
- Post-turn semantic mutations do not share the turn transaction or a durable checkpoint.
- Compatibility fields in standalone `World` duplicate the live Story representation at save and
  binding boundaries.

## See also

- [[architecture/scene-loop]]
- [[architecture/system-overview]]
- [[architecture/plot-graph]]
- [[architecture/memory-system]]
