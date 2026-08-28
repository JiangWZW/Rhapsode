---
title: Python server
last_updated: 2026-08-27
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
- a resume flag;
- a shared `ThreadPoolExecutor` for perception and monologue HTTP.

The session configures narrator, scheduler, lifecycle, Weaver, downsampling, memory, and
`PromptJobs` ready/submit callbacks on Story. Blocking perception/reflection setters stay unset so
`complete_turn` polls.

C++ owns `StoryData`, `TurnServices`, World, SceneData, and the Weaver service.
Turn execution and graph-plan application are free functions, not owned executor/director objects.

Eval spawn-wait uses HTTP `GET /health`, not `/ws`. Opening `/ws` constructs a full session
(Story + Chroma); a throwaway probe would race the real connection on the Chroma client.

## WebSocket flow

1. Load the scenario and optional save.
2. Configure callbacks and synchronize graph nodes to external memory.
3. Receive player text; set status `processing`.
4. Run `Story.advance_player()` on a worker thread; stream those SceneMessages.
5. Set status `ready` so the UI can accept the next action while post-turn work runs.
6. In `finally`, run `Story.complete_turn()` on a worker (deferred graph extraction, post-turn,
   lifecycle, off-stage, save).
7. Stream any merge/off-stage extras; then set status `idle` (eval waits on this).
8. Report exceptions without rebuilding the native runtime; candidate-turn rollback keeps it usable.
   If `advance_player` succeeds, `complete_turn` must still run so a pending turn cannot leak.
   A player_message sent after `ready` stays in the WebSocket buffer until the loop receives
   again; C++ still rejects overlapping `advance_player` calls.
9. While `idle`, `Story.poll_minds` runs every 0.25s: harvest newest ready perception/monologue, then catch-up-submit monologue if perception is ahead. It does not increment ring heads. `process_post_turn` harvests and catch-up-sends before weave, then submits this beat’s perception and increments heads.
10. On disconnect, wait in-flight perception/monologue HTTP, `apply_ready_minds` (harvest only,
   no new dispatch), then save. Unfinished work is not saved; next load redispatches from
   `perception_turn_` / `monologue_turn_`.

## LLM adapters

`llm.py` implements provider/tool-loop mechanics. `llm_tools.py`, `scheduler.py`, and `lifecycle.py`
adapt those mechanics to native callbacks.

Narrator and scheduler factories take no Story argument and retain no Story reference. Each native
call supplies a temporary `read_tool(name, args_json)` function.

The adapter may use it during the tool loop but must not retain it. Native code invalidates it on
callback return.

Lifecycle receives a self-contained turn summary and returns JSON; it has no tool callback.

## Ownership boundary

Python owns integration objects and model clients while C++ owns runtime state and ordering.
SceneData, observation, roster, character, and memory properties return detached values that cannot
alter the running story.

Only explicit Story commands mutate native state. The manual `/weave` diagnostic, for example,
configures the Story-owned Weaver and calls
`Story.weave_scene()` rather than constructing a Python Weaver over a live graph reference.

## Limitations

- `status: ready` means the response is delivered and graph extraction has moved off the response
  path. Another socket message can be buffered, but the session does not start that next turn until
  the synchronous `complete_turn()` call returns.
- `PromptJobs` generations prevent an obsolete ring result from applying to a reused slot, but
  replacing a `(handle, slot)` entry does not cancel the older `Future`. Superseded HTTP work may
  continue consuming the shared eight-worker pool and provider capacity.
- Provider calls have no foreground/background admission controller. The narrator bypasses the mind
  thread pool but can still contend with it for shared provider quotas and queues.
- `_stream_outputs()` sends complete messages after `advance_player()` returns; provider-token
  streaming is not implemented.

## Persistence

Story writes `world.json`, one `<scene_id>.json` per live SceneData, and `story.json` for aggregate
identity/scheduling. Python supplies the saves directory but does not serialize native fields.
