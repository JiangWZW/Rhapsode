---
sources:
  - core/include/rhapsode/scene_loop.h
  - bindings/bind_rhapsode.cpp
last_updated: 2026-05-12
confidence: verified
tier: episodic
related:
  - "[[architecture/scene-loop]]"
  - "[[decisions/ownership-split]]"
tags:
  - cpp-core
  - python-server
---

# Decision: callback vs pull pattern for SceneLoop-to-Python

**Status:** accepted
**Decided:** early development

## Context

The SceneLoop in C++ needs to invoke Python code at two points during each turn:

1. **Build prompt** — assemble the messages list from history + scenario metadata + Director output.
2. **Run LLM** — send the prompt to an LLM API and get the assistant response.

Two patterns were considered for this cross-language invocation.

## Options

### Option A: Callback registration (chosen)

Python registers `std::function` callbacks via pybind11. The C++ loop calls them synchronously at the appropriate stages.

```
Python                           C++
  ├── set_prompt_callback(fn) ──→│
  ├── set_llm_callback(fn) ─────→│
  │                               │
  ├── submit_input(text) ────────→│
  │                               ├── append user msg
  │                               ├── Director.tick()
  │◄── prompt_callback(hist) ─────┤
  ├── returns prompt ────────────→│
  │◄── llm_callback(prompt) ──────┤
  ├── returns response ──────────→│
  │                               ├── append assistant msg
  │◄── turn_complete(msg) ────────┤
```

**Pros:** C++ controls the flow — the FSM is self-contained and testable. No polling. Natural fit for pybind11 `std::function` wrapping. Easy to unit-test with mock callbacks.

**Cons:** Synchronous — blocks the calling thread while Python runs. Requires `run_in_executor` on the asyncio side.

### Option B: Pull / polling pattern (rejected)

C++ exposes state and a "needs" query. Python polls `loop.state()` and provides data when needed.

**Pros:** Python controls async scheduling. No callback ownership complexity.

**Cons:** More API surface. Loop logic split across languages. Polling adds latency or complexity.

## Decision

**Callback registration** (Option A). The synchronous blocking is acceptable because:

1. Each turn is inherently sequential: Director → prompt → LLM → append.
2. The LLM HTTP call dominates latency — `run_in_executor` overhead is negligible.
3. The C++ FSM stays self-contained and testable without Python.

## Consequences

- The Python server uses `asyncio.run_in_executor()` when calling `loop.submit_input()`.
- The same callback pattern extends to Director — 2 callbacks — and MemorySystem — 7 callbacks for embed, store, query, and others.
- Future streaming support may require a different pattern — a streaming callback yielding chunks.

## Retrospective

This decision has held well through the addition of the Director and MemorySystem. The callback pattern scaled naturally — the MemorySystem alone registers 7 callbacks, all following the same `std::function` registration pattern through pybind11. The main cost is that all callbacks are synchronous, but since the LLM HTTP call is always the bottleneck, this has not been a practical issue.
