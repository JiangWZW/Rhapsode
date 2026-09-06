---
title: Offline character study
date: 2026-08-30
tags: [architecture, offline, character-study]
---

# Offline character study

A novel-walk toolchain that writes a living character study. It lives at `offline/character_study/` and is **not** part of a turn.

It does not import `rhapsode`, `_core`, or `session_pipeline`. It does not write into `server/scenarios/` or narrator prompts. It is not [session eval](session-eval.md).

## Flow

1. `extract.py` + `critic.py` — Pass 1: name-hit sections, then a living study one section at a time.
2. `reader_experiment.py` — Pass 2: two independent reader letters from the same first-pass study.
3. `study_experiment.py` — Pass 3: each letter continues into its own candidate study. Not merged.
4. Manual revision into `study/`.
5. `narrator_experiment.py` — temp Konosuba copies + parallel [session eval](session-eval.md) A/B. Subprocess only; still not imported by the server.

## How to run

See `offline/character_study/README.md`. Own `.venv`, own `.env` (`DEEPSEEK_API_KEY`). Novel path is `CHARACTER_STUDY_NOVEL` or `paths.novel` in `config.yaml`. Book map is local `books/<book>/` (not tracked). Written studies in `study/` are tracked.

The narrator A/B uses `server/.venv` and `server/.env`. It subprocess-launches [session eval](session-eval.md); it does not import it. Default `--mode none` leaves the narrator prompt alone and runs five turns. Kazuma is played by the player model; no scripted Kazuma sentences. Both sides set `RHAPSODE_NARRATOR_MAX_ROUNDS=24`. The old-character-text run sets `RHAPSODE_QUERY_CHARACTER_CORE=0`; the studies run sets `1` so the beat narrator can pull the study page. Do not paste studies onto Scene style to judge the pages. The first pair (`checkpoints/narrator-ab-20260905-235107/`) pasted the full studies into scene style; short tries are under `checkpoints/narrator-ab-20260906-s1` through `-s3`; five-line `--mode none` pairs are `20260906-ab` and `20260906-ab2`. Notes: `checkpoints/short-tries.md`.

Do not start a critic walk unless asked.
