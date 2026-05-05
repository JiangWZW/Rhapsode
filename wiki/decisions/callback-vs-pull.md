# Decision: callback vs pull pattern for SceneLoop-to-Python

**Status:** accepted  
**Date:** 2026-05-05  

## Context

The `SceneLoop` in C++ needs to invoke Python code at two points during each turn:

1. **Build prompt** — assemble the messages list from history + scenario metadata.
2. **Run LLM** — send the prompt to an LLM API and get the assistant response.

Two patterns were considered for this cross-language invocation.

## Options

### Option A: Callback registration (chosen)

Python registers `std::function` callbacks via pybind11. The C++ loop calls them synchronously at the appropriate stages.

```
Python                           C++
  |                               |
  |-- set_prompt_callback(fn) --->|
  |-- set_llm_callback(fn) ----->|
  |                               |
  |-- submit_input(text) -------->|
  |                               |-- appends user msg
  |                               |-- calls prompt_callback
  |<-- prompt_callback(hist) -----|
  |-- returns prompt ------------>|
  |                               |-- calls llm_callback
  |<-- llm_callback(prompt) ------|
  |-- returns response ---------->|
  |                               |-- appends assistant msg
  |                               |-- calls turn_complete_callback
  |<-- turn_complete(msg) --------|
```

**Pros:**
- C++ controls the flow — the FSM is self-contained and testable.
- No polling or busy-waiting.
- Natural fit for pybind11's `std::function` wrapping.
- Easy to unit-test in C++ with mock callbacks.

**Cons:**
- Callbacks are synchronous — blocks the calling thread while Python runs.
- Requires `run_in_executor` on the Python side to avoid blocking the asyncio event loop.

### Option B: Pull / polling pattern (rejected)

C++ exposes state and a "needs" query. Python polls `loop.state()` and provides data when the loop is in a waiting state.

```
Python                           C++
  |                               |
  |-- submit_input(text) -------->|
  |                               |-- appends user msg
  |                               |-- state = NeedPrompt
  |-- poll: state()? ------------>|
  |<-- NeedPrompt ----------------|
  |-- provide_prompt(prompt) ---->|
  |                               |-- state = NeedLLM
  |-- poll: state()? ------------>|
  |<-- NeedLLM -------------------|
  |-- provide_llm_result(text) -->|
  |                               |-- appends assistant msg
```

**Pros:**
- Python has full control over async scheduling.
- No callback ownership complexity.

**Cons:**
- More API surface: multiple "provide" methods + polling.
- The loop is split across two languages — harder to reason about and test.
- Polling adds latency or complexity (wait conditions, events).

## Decision

**Callback registration** (Option A). The synchronous blocking is acceptable for MVP because:

1. Each turn is inherently sequential (prompt -> LLM -> append).
2. The LLM HTTP call dominates latency — the overhead of `run_in_executor` is negligible.
3. The C++ FSM stays self-contained and testable without Python.

## Consequences

- The Python server must use `asyncio.run_in_executor()` when calling `loop.submit_input()` to avoid blocking the FastAPI event loop.
- Future streaming support may require a different pattern (e.g. a streaming callback that yields chunks). This is out of scope for MVP.
- If the loop needs to become async-native (e.g. for parallel LLM calls in multi-character scenes), this decision should be revisited.
