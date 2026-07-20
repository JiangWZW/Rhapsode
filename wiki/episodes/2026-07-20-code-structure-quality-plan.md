---
sources:
  - CMakeLists.txt
  - CMakePresets.json
  - build_ninja.bat
  - build_test_scene.bat
  - core/include/rhapsode/story.h
  - core/include/rhapsode/scene_loop.h
  - core/src/story.cpp
  - core/src/scene_loop.cpp
  - core/src/character_memory.cpp
  - core/src/weaver.cpp
  - core/src/world.cpp
  - core/src/world_graph.cpp
  - bindings/bind_rhapsode.cpp
  - server/rhapsode/session.py
  - server/rhapsode/llm.py
  - frontend/src/stores/websocket.ts
  - frontend/src/utils/sceneTextParser.ts
last_updated: 2026-07-20
confidence: verified
status: phase-2-in-progress
tier: episodic
related:
  - "[[decisions/coding-guidelines]]"
  - "[[architecture/system-overview]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/python-server]]"
  - "[[architecture/vue-frontend]]"
tags:
  - cross-layer
  - design
crystallized_from: "Clean-origin code structure and quality review"
---

# Code structure and quality plan

## Question

Which changes improve Rhapsode's maintainability without hiding behavioral changes inside a broad refactor?

## Verified baseline

- The clean `origin/main` tree builds, all 32 Catch2 tests pass, Python modules byte-compile, and the
  Vue production build passes.
- `build_test_scene.bat` builds under the existing Ninja cache but launches the Visual Studio-only
  path `build/core/Release/test_scene.exe`, so the documented test entry point reports failure.
- `build_ninja.bat` trusts `CMakeCache.txt` without checking for `build.ninja` or complete
  FetchContent checkouts. An interrupted build can leave a cache that Ninja cannot use.
- `test_llm_retry.py` passes. `test_alias_keying.py` has three assertions that expect raw
  perceptions to appear in `view_of`, while `CharacterMemory::route_fact` documents perceptions as
  awaiting `reflect_perceptions`.
- No CI workflow, Python test/lint configuration, frontend test/lint configuration, or C++ warning
  policy enforces the working baseline.
- `scene-loop.md` scores 2.9 in the wiki health check and describes removed APIs. `python-server.md`
  and `system-overview.md` also contain pre-`Story` architecture claims.

## Findings

### Correctness risks precede structural cleanup

`SceneLoop::dispatch_background` captures `this` and mutates the loaded scene's graph, character
memories, downsampler, and Weaver state. `Story::advance_scene` can call `save` immediately after a
beat returns. The single-scene path does not join background work before persistence. Tests do not
specify whether save and background mutation may overlap.

Narrator failure is not transactionally defined. Prompt construction increments `turn_index`, while
`submit_input` appends the user message before callbacks run. Retry rollback restores the world
graph and removes newly created roster entries, but its contract does not cover every related
container. The server rebuilds `SceneLoop` after an exception, which hides rather than specifies the
resulting scene state.

These paths need characterization tests and explicit invariants before files move.

### Large files combine stable responsibilities

The largest C++ translation units have natural, existing seams:

| File | Responsibilities to separate |
|---|---|
| `story.cpp` | lifecycle, narrator tools, scheduling/runtime, persistence |
| `character_memory.cpp` | belief mutation, thought rendering, reflection parsing/application |
| `weaver.cpp` | prompt assembly, response application, expiry queue |
| `world_graph.cpp` | graph mutation/traversal, JSON migration, DOT rendering |
| `world.cpp` | roster/lifecycle staging, query tools, persistence |
| `bind_rhapsode.cpp` | bindings for model, story/runtime, graph/memory, utilities |

The split can use multiple implementation files for the same classes. New interfaces or manager
classes are not required.

### Boundary code is difficult to test offline

`server/rhapsode/llm.py` combines provider construction, environment lookup, retries, message
conversion, network calls, and tool loops behind a module singleton. `session.py` constructs every
native and external dependency directly. Pure payload and provider-conversion logic exists, but it
is not collected by a test runner.

The frontend duplicates `EntitySpan`. Its custom Markdown inline rules emit raw `html_inline`
content from narrator text, despite `html: false`; parenthesized or bracketed model text therefore
needs explicit escaping and security tests. WebSocket messages are parsed as untyped JSON without a
malformed-message path.

## Implementation record (2026-07-20)

Phases 0 and 1 are complete.

- `verify.bat` now builds the native module, runs CTest generator-independently, byte-compiles and
  tests/lints Python, and tests/lints/builds the frontend. The same matrix runs in Windows CI.
- CMake uses explicit modern Python discovery, project targets have scoped warning flags, and build
  and test presets include `test_scene`. Interrupted Ninja trees are rebuilt cleanly.
- A failed player or autonomous submission is transactional across scene history, dialogue, turn
  state, downsampler state, the shared World (graph, roster, minds, and pending lifecycle ops), and
  emitted outputs. Narrator retries restore the same whole-World snapshot.
- Background work has an explicit `SceneLoop` shutdown contract. Scene switches, Story memory sync,
  lifecycle application, and persistence now join background mutation before crossing boundaries.
- Weaver's sampled subgraph now owns its nodes; testing this path exposed and removed dangling
  pointers into a temporary `WorldGraph::all_nodes()` result.
- Custom Markdown spans escape model text and sanitize entity CSS categories. Shared protocol types,
  malformed WebSocket handling, and duplicate-connect protection are covered by frontend tests.
- The two deterministic root Python scripts are collected pytest tests, with offline coverage for
  retry behavior, provider message conversion, player input parsing, and WebSocket payloads.

Verification result: 38 C++ tests, 11 Python tests, and 7 frontend tests pass; Ruff and ESLint are
clean, the frontend production build succeeds, and the production npm audit reports zero known
vulnerabilities.

Phase 2 began on 2026-07-20. The monolithic pybind11 translation unit is now a small module entry
point plus story, graph, runtime, and memory registration units. Registration order and the public
module surface remain unchanged; an exact public-symbol test raises the Python total to 12 tests.

## Plan

### Phase 0: establish one reliable verification command — complete

1. Repair both Windows build scripts and add CMake test presets. A partial Ninja tree must trigger a
   clean reconfigure; tests must run through CTest for either generator.
2. Make Python selection explicit. The current `Python3_EXECUTABLE` cache variable is reported as
   unused by the pybind11 configuration.
3. Add a CI workflow for the C++ build and CTest, Python byte-compilation and tests, and the frontend
   production build.
4. Add formatting/lint checks incrementally: `clang-format --dry-run` on touched C++ files, Ruff for
   Python correctness rules, and ESLint for TypeScript/Vue. Do not combine this with mass reformatting.

Exit criterion: one local command and CI produce the same deterministic result from a clean checkout.

### Phase 1: specify failure, persistence, and security behavior — complete

1. Add C++ tests for narrator exceptions, retry rollback, autonomous-turn failure, scene switching,
   background shutdown, and save-after-beat ordering.
2. Define the failed-turn transaction: which history entries, turn counters, graph changes, roster
   entries, character memories, and staged lifecycle operations survive an exception.
3. Ensure persistence cannot read structures while background work mutates them. Prefer an explicit
   join at the transaction boundary before adding locks or a task framework.
4. Give `SceneLoop` an explicit shutdown/lifetime contract and test destruction with active work.
5. Escape custom Markdown inline content and entity categories. Add browser-independent tests for
   model-supplied HTML, malformed brackets, annotations, and Unicode.
6. Resolve the alias-keying diagnostic against the documented perception/reflection contract, then
   convert it to a collected test.

Exit criterion: error recovery, save ordering, and rendered model text have deterministic tests.

### Phase 2: split C++ implementation files without changing APIs

Perform one extraction per commit, running the full verification command after each:

1. **Complete (2026-07-20):** Split `bind_rhapsode.cpp` into domain registration functions while
   retaining one `PYBIND11_MODULE` and the existing Python API.
2. Move `WorldGraph` JSON/legacy migration and DOT rendering into separate translation units.
3. Split `CharacterMemory` rendering from reflection parsing/application.
4. Split Weaver prompt/application logic from expiry-queue management.
5. Split Story into lifecycle/tools, runtime sequencing, and persistence files.
6. Split World and Scene query/persistence implementations only where the same separation is already
   visible in their public headers.

Exit criterion: public headers and serialized formats remain unchanged; each commit is mechanical and
passes all characterization tests.

### Phase 3: make Python and protocol boundaries testable

1. Convert deterministic `server/test_*.py` scripts into pytest tests with no execution at import time.
2. Separate provider construction from pure message/tool conversion in `llm.py`. Keep the existing
   functions; do not introduce a speculative provider class hierarchy.
3. Pass configuration and callback factories into session construction so socket tests can use fakes
   without ChromaDB, embedding models, or live LLMs.
4. Define WebSocket payloads with `TypedDict` or equivalent shared protocol types. Test player input,
   undo, output serialization, errors, and malformed messages offline.
5. Consolidate frontend protocol types and add store tests for duplicate connects, disconnects, bad
   JSON, error messages, and processing-state transitions.

Exit criterion: provider conversion and the WebSocket contract are covered without network services.

### Phase 4: narrow mutable APIs and ownership contracts

1. Audit pybind11 forwarding setters that replace complete World containers. Replace them with
   read-only properties or explicit mutation methods only after Python callers and tests are known.
2. Document and enforce the lifetimes of `Story::loop_`, `SceneLoop::scene_`, `director_`, `weaver_`,
   and `World::memory_`. Prefer small RAII changes over a new dependency-injection framework.
3. Raise project warnings in stages and fix findings locally. Avoid enabling warnings globally for
   fetched dependencies.

Exit criterion: native pointers and Python reference policies have an explicit, tested owner.

### Phase 5: documentation and repository hygiene

1. Rewrite `scene-loop.md` from source; its health score requires a rewrite rather than another note.
2. Rewrite the current-state sections of `system-overview.md` and `python-server.md` around
   `World`, `Story`, split server modules, and authored `speech_turns`.
3. Bring the wiki linter to zero structural errors before treating readability warnings as a gate.
4. Review `fix_and_apply.py`, `rw.txt`, and other root migration artifacts; remove them only after
   confirming they contain no unique history.

Exit criterion: the wiki describes current symbols and the repository root contains active tooling.

## Change discipline

- Tests precede behavioral changes; behavioral changes precede mechanical file moves.
- One responsibility moves per commit, with no opportunistic renaming.
- File length is a signal, not a target. A split is complete when each file has one main reason to
  change.
- Existing public APIs and serialized formats stay stable during structural phases.

## See Also

- [[decisions/coding-guidelines]]
- [[architecture/scene-loop]]
- [[architecture/python-server]]
- [[architecture/vue-frontend]]
