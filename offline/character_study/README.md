# Offline character study

Walk a novel and write a living character study. This is **not** the game runtime, **not** session eval, and **not** the autoplay runner. It does not import the game server or C++ bindings. Studies are not written into `server/scenarios/` or narrator prompts.

## Setup

```
cd offline/character_study
python -m venv .venv
.venv\Scripts\pip install -r requirements.txt
copy .env.example .env
```

Put `DEEPSEEK_API_KEY` in this package's `.env`. The scripts do not read `server/.env`.

### Novel (not in git)

Set `CHARACTER_STUDY_NOVEL`, or leave `paths.novel` in `config.yaml`. On this machine the default is `D:/cursor-workspace/Konosuba/konosuba-data/konosuba.txt`. Do not copy the novel into this repo.

### Book map (local only, not in git)

Copy `volumes.json` and `briefs/` into `books/konosuba/` from `D:/cursor-workspace/Konosuba/study` (or rebuild the map). Without that folder, extract cannot assign volumes.

## Run

```
.venv\Scripts\python extract.py
.venv\Scripts\python test_extract.py
.venv\Scripts\python critic.py --max-sections 1
```

`extract.py` opens 80 lines on each side of every configured name hit (clipped to the volume). Overlapping windows merge into one section. `critic.py` updates `study/darkness.md` one section at a time. `cite.py` and `refine.py` are later passes.

Unit tests always run. The real-extract count skips if the novel or `books/konosuba` is missing.

Tracked: `study/` (written studies). Not tracked: `books/`, `sections/`, `checkpoints/`, `.venv/`, `.env`.
