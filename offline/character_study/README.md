# Offline character study

Walk a novel and write a living character study. This is **not** the game runtime, **not** session eval, and **not** the autoplay runner. It does not import the game server or C++ bindings. Studies are not written into `server/scenarios/` or narrator prompts.

## Setup

```
cd offline/character_study
python -m venv .venv
.venv\Scripts\pip install -r requirements.txt
copy .env.example .env
```

Put `DEEPSEEK_API_KEY` in this package's `.env`. Set `DEEPSEEK_API_BASE` too when the configured model is served by an OpenAI-compatible gateway. The scripts do not read `server/.env`.

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

`extract.py` opens 80 lines on each side of every configured name hit (clipped to the volume). Overlapping windows merge into one section. `critic.py` writes the Pass 1 study one section at a time. Pass 2 is `reader_experiment.py`. Pass 3 is `study_experiment.py`. The narrator A/B is `narrator_experiment.py`: it does not import the server or session eval; it builds temp Konosuba copies and subprocess-launches `run.py`.

### Blind-reader baseline

`reader_experiment.py` tests whether separate sampling alone produces meaningfully different readings. It sends the exact same first-pass study and neutral prompt to two fresh DeepSeek sessions at temperature 0.9. The requests use different explicit sampling seeds so an exact-request cache cannot turn the second session into a replay. Neither reader sees another output or a provisional portrait.

```
.venv\Scripts\python reader_experiment.py --dry-run
.venv\Scripts\python reader_experiment.py
```

Outputs and a manifest containing the input and prompt hashes go under the ignored `checkpoints/darkness/reader-baseline-*` folder. Compare the scenes, relationships, unresolved questions, and organizing perceptions—not merely wording.

### Reader-authored study experiment

`study_experiment.py` continues each saved reader separately. Each model receives the original first-pass prompt and only its own letter as conversation history, then writes a candidate character study. The character name is read from `config.yaml` and inserted at runtime; no name or gender is hardcoded in the instruction. The candidates are not merged.

```
.venv\Scripts\python study_experiment.py --dry-run
.venv\Scripts\python study_experiment.py
```

The script validates the first-pass hash and defaults to the latest complete two-reader checkpoint. Outputs go into a new ignored `checkpoints/darkness/study-candidates-*` folder. It does not overwrite Pass 1 or the letters.

### Narrator A/B (session eval)

`narrator_experiment.py` tests the tracked studies in `study/` against the game's current Darkness and Megumin text. It writes two temp `konosuba.json` copies. Default `--mode none` leaves the narrator prompt alone. `--mode short` adds the same two tiny scene notes on both sides. `--mode full` is the first pair (whole pages pasted onto the narrator). Default is five turns. Kazuma is played by the player model; no scripted Kazuma sentences. The narrator tool-round cap is 24. The old-character-text run forbids `query_character_core`; the studies run enables it. It uses `server/.venv` and `server/.env`, not this package's venv. It does not write into `server/scenarios/` or change runtime prompts.

```
..\..\server\.venv\Scripts\python.exe narrator_experiment.py --dry-run
..\..\server\.venv\Scripts\python.exe narrator_experiment.py --mode none
```

Fixtures and `plain.md` go under ignored `checkpoints/narrator-ab-*`. Eval output is `experiments/session_pipeline/runs/study-ab-{old,new}-<mode>-<stamp>/`. One pair at a time.

Unit tests always run. The real-extract count skips if the novel or `books/konosuba` is missing.

Tracked: `study/` (written studies). Not tracked: `books/`, `sections/`, `checkpoints/`, `.venv/`, `.env`.
