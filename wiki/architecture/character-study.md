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
4. Manual revision of a candidate (not a script). Narrator experiment is not built yet.

## How to run

See `offline/character_study/README.md`. Own `.venv`, own `.env` (`DEEPSEEK_API_KEY`). Novel path is `CHARACTER_STUDY_NOVEL` or `paths.novel` in `config.yaml`. Book map is local `books/<book>/` (not tracked). Written studies in `study/` are tracked.

Do not start a critic walk unless asked.
