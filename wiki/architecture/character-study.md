---
title: Offline character study
date: 2026-08-30
tags: [architecture, offline, character-study]
---

# Offline character study

A novel-walk toolchain that writes a living character study. It lives at `offline/character_study/` and is **not** part of a turn.

It does not import `rhapsode`, `_core`, or `session_pipeline`. It does not write into `server/scenarios/` or narrator prompts. It is not [session eval](session-eval.md).

## Flow

1. `extract.py` — name hits become sections (window 80, overlapping windows merge).
2. `critic.py` — first pass, one section at a time, updates `study/<name>.md`.
3. `cite.py` — attach `(vN Lline)` after quotes that match the novel.
4. `refine.py` — one later pass over the cited study.

## How to run

See `offline/character_study/README.md`. Own `.venv`, own `.env` (`DEEPSEEK_API_KEY`). Novel path is `CHARACTER_STUDY_NOVEL` or `paths.novel` in `config.yaml`. Book map is local `books/<book>/` (not tracked). Written studies in `study/` are tracked.

Do not start a critic walk unless asked.
