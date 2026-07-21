---
title: Turn execution
last_updated: 2026-07-21
confidence: verified
tier: semantic
sources:
  - core/include/rhapsode/turn_executor.h
  - core/src/turn_executor.cpp
  - core/src/turn_executor_narrator.cpp
  - core/src/turn_executor_post_turn.cpp
  - core/src/story_advance.cpp
  - server/rhapsode/llm_tools.py
  - server/rhapsode/scheduler.py
related:
  - "[[architecture/cpp-data-model]]"
  - "[runtime architectural decoupling plan](../episodes/2026-07-21-runtime-architectural-decoupling-plan.md)"
tags:
  - cpp-core
  - cross-layer
---

# Turn execution

`TurnExecutor` executes one synchronous, rollback-capable turn. It is not a loop and does not own
runtime composition, policy, persistence, external memory, or SceneData.

## Boundary

```mermaid
flowchart LR
    Story -->|one call| Executor["TurnExecutor"]
    Story -->|owns| Director
    Story -->|owns| Weaver
    Story -->|owns| World
    Executor -.->|borrows| Director
    Executor -.->|borrows| Weaver
    Executor -.->|borrows| World
    Executor -.->|call borrow| SceneData
    Executor -->|TurnResult| Story
```

Director and Weaver must use the injected World's graph. Construction rejects a graph mismatch,
and there are no rebinding setters.

## Transaction order

1. Reject re-entry and snapshot SceneData, World, and resume state.
2. Append the player input or autonomous cue.
3. Build narrator instructions and turn state.
4. Invoke the narrator with a call-scoped read-tool function.
5. Parse and validate the merged prose/plan response.
6. Apply Director graph changes; on rejection, restore World and retry.
7. Resolve cast, emit narration/dialogue, and route perceptions.
8. Detect and confirm deaths.
9. Run post-turn work synchronously: weave, expiry, reflection, downsampling.
10. Return generic effects and emitted messages.

An exception restores the snapshots and leaves the executor reusable. Post-turn failures remain
non-fatal, matching the pre-refactor behavior.

## Callback boundary

Narrator and scheduler callbacks receive a read function with this logical shape:

```cpp
string read_tool(string name, string args_json);
```

Story creates it for one callback call. Python adapters do not capture Story, and retained use after
the call throws `Read tool callback is no longer active`. The function dispatches only the existing
read tools: graph, mind, history, and live-storyline summaries.

Lifecycle has no read tools. `storyline_policy` builds its plain BeatSummary, invokes the callback,
and returns a plain LifecycleDecision for Story to validate and apply.

## Result boundary

`TurnResult` contains only stable public values:

- `scene_id` and `completed_turn`;
- output SceneMessages;
- created and expired Nodes.

Story uses those effects to synchronize MemorySystem. It does not know Director or Weaver result
types.

## Concurrency

Turn execution is synchronous. There is no internal future or background reference capture. The
FastAPI layer may place the complete `Story.advance_scene()` call on a worker thread so the event
loop remains responsive; that does not change native ownership or operation order.
