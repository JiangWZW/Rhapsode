---
title: Python server
last_updated: 2026-07-21
confidence: verified
tier: semantic
sources:
  - server/rhapsode/app.py
  - server/rhapsode/session.py
  - server/rhapsode/llm.py
  - server/rhapsode/llm_tools.py
  - server/rhapsode/scheduler.py
  - server/rhapsode/lifecycle.py
  - server/rhapsode/memory.py
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/cpp-data-model]]"
  - "[[architecture/memory-system]]"
  - "[[decisions/ownership-split]]"
tags:
  - python-server
---

# Python server

The FastAPI layer is a composition and transport adapter around the native Story API. It does not
own turn sequencing or reconstruct native execution objects after errors.

## Session composition

`server/rhapsode/session.py` constructs one `WsSession` containing:

- Story, the native aggregate;
- MemorySystem, the Chroma/embedding adapter retained by Story through shared ownership;
- Annotator, a roster-aware read service;
- a resume flag.

The session configures narrator, scheduler, lifecycle, Weaver, reflection, downsampling, and memory
callbacks on Story. Director, Weaver, TurnExecutor, World, and SceneData are constructed and owned
inside C++.

## WebSocket flow

1. Load the scenario and optional save.
2. Configure callbacks and synchronize graph nodes to external memory.
3. Receive player text; set status `processing`.
4. Run `Story.advance_player()` on a worker thread; stream those SceneMessages.
5. In `finally`, run `Story.complete_turn()` on a worker (post-turn, lifecycle, off-stage, save).
6. Stream any merge/off-stage outputs; then set status `idle`.
7. Report exceptions without rebuilding the native runtime; TurnExecutor rollback keeps it usable.
   If `advance_player` succeeds, `complete_turn` must still run so a pending beat cannot leak.

## LLM adapters

`llm.py` implements provider/tool-loop mechanics. `llm_tools.py`, `scheduler.py`, and `lifecycle.py`
adapt those mechanics to native callbacks.

Narrator and scheduler factories take no Story argument and retain no Story reference. Each native
call supplies a temporary `read_tool(name, args_json)` function.

The adapter may use it during the tool loop but must not retain it. Native code invalidates it on
callback return.

Lifecycle receives a self-contained beat summary and returns JSON; it has no tool callback.

## Ownership boundary

Python owns integration objects and cloud/local model clients. C++ owns runtime state and ordering.
SceneData is exposed as a read-only view. Story-owned World graph, roster, character, and memory
properties return detached snapshots for UI/diagnostic code, so mutating a returned Python object
cannot alter the running story. Mutations that affect native state go through explicit Story
commands. The manual `/weave` diagnostic, for example, configures the Story-owned Weaver and calls
`Story.weave_scene()` rather than constructing a Python Weaver over a live graph reference.

## Persistence

Story writes `world.json`, one `<scene_id>.json` per live SceneData, and `story.json` for aggregate
identity/scheduling. Python supplies the saves directory but does not serialize native fields.
