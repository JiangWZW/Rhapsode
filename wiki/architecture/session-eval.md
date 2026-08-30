---
title: Session eval (autoplay)
last_updated: 2026-08-29
confidence: verified
tier: semantic
related:
  - "[[architecture/python-server]]"
tags:
  - session-eval
  - autoplay
---

# Session eval (autoplay)

Offline harness. Not a frontend mode.

`autoplay.bat` → [`experiments/session_pipeline/run.py`](../../experiments/session_pipeline/run.py) → C++ `SessionEvalRunner` spawns one uvicorn, drives one `/ws`, writes `experiments/session_pipeline/runs/<name>/`.

## Parallel dispatch

Each spawned `run.py` owns its own backend. **Unique `--out-dir` per run.** Same out-dir means the same `live/saves` and `live/chroma`.

Leave `--port 8080` on every process. `run.py` claims the next free port with a process-held lock under `runs/.port-locks/`. A listen-check alone is not enough: uvicorn binds only after embedding warmup (~50s), so simultaneous starts all see 8080 free unless the lock is held from claim until process exit.

`--attach` shares whoever is already on that host:port. Do not use it for a second eval. `autoplay.bat` does not kill listeners.

Human play and `reset.bat` stay on `server/saves` and `server/chroma`. Spawned eval sets `RHAPSODE_SAVES_DIR` and `RHAPSODE_CHROMA_DIR` to `<out_dir>/live/`.

If the spawned uvicorn dies or `GET /health` never answers, eval aborts. It must not open `/ws` on another run's server.

```bat
set NO_PROXY=127.0.0.1,localhost
cd experiments\session_pipeline
start "a" ..\..\server\.venv\Scripts\python.exe run.py --turns 2 --out-dir runs\iso-a
start "b" ..\..\server\.venv\Scripts\python.exe run.py --turns 2 --out-dir runs\iso-b
```

Or two `autoplay.bat` windows with different output folder names. Confirm `port=` and `saves_dir=` in each window.

They share the DeepSeek key (long overlap can 402). Each uvicorn loads the embedding model.

## Do not

- `--attach` to launch a second eval
- Reuse one `--out-dir` for two live runs
- Point `--saves-dir` at `server/saves` for a parallel eval
- Kill whatever is on 8080 to "make room"
