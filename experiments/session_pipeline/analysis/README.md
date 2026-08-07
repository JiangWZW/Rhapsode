# Session collapse metrics (E0/E1 instruments)

Offline analysis tools for diagnosing motif collapse in autonomous runs.
Background and spec: `wiki/research/motif-collapse-default-guide-300.md` (§6).

- `relaxation.py` — **primary E1 relaxation metric** (baseline-return /
  distance-to-attractor).
- `collapse_metrics.py` — E0 collapse-shape metrics over a whole run.

## relaxation.py — baseline-return (primary E1 metric)

Measures how long a perturbed session takes to fall back into the collapsed
attractor. Replaces hand-picked anchor terms as the primary measure (anchors
are probe-specific and blind to paraphrase/coreference — e.g. "Gerhardt"
continuing as "the old farmer").

```
python relaxation.py --run RUN_DIR --baseline RUN_DIR_OR_TURNS_JSONL
    [--baseline-last 60] [--injections FILE] [--k 3] [--z-min -2]
    [--smooth 3] [--anchors "a,b"] [--out DIR]
python relaxation.py --validate REF_RUN_DIR    # controls + smoke test
```

Method (lexical on purpose — bge embeddings are style-blind on this corpus,
see below): TF-IDF (same recipe as `collapse_metrics.py`) fit on the union
of the baseline's last N narrator beats and all run beats; the attractor is
the baseline centroid, with an in-band z-band calibrated from the baseline
beats' own cosines (trailing 3-beat means; per-beat scores were validated
too noisy). A run beat is in-band iff its one-sided z ≥ −2. Relaxation time
per injection at turn T = smallest r such that the k=3 consecutive beats
from turn T+r are all in-band. Secondary robustness check: unigram
Jensen-Shannon divergence of a trailing 5-beat window vs the baseline
distribution, ceiling calibrated leave-window-out on the baseline. Outputs
CSV + PNG into `<run>/analysis/relaxation/`. `--injections` accepts the
E1 TOML (`[[injection]]` rows) or a JSON list of `{turn, text, label}`.

### Validation on `default-guide-300` (2026-08-06)

Positive control = quest-era beats (turns 1–30) scored against the last-N
attractor; negative control = last 10 beats held out, baseline from the N
beats before them. Smoothed (default, trailing 3) and raw per-beat medians:

| N | pos median z | pos % out-of-band | neg % in-band | neg median z | raw pos median z |
|---|---|---|---|---|---|
| 40 | −4.87 | 100% | 90% | −1.67 | −2.91 |
| **60 (default)** | **−2.76** | **83%** | **100%** | **−1.02** | −1.81 |
| 80 | −2.16 | 53% | 100% | −0.74 | −1.57 |

Notes: (1) smoothing is load-bearing — raw per-beat scores fail the positive
control at N≥60; (2) **do not overextend the baseline window**: N=80 reaches
back into the frost-bench/card-game excursions and dilutes the attractor
(only 53% of quest beats flagged); N=40–60 is the validated range. (3) The
JSD secondary is under-sensitive out-of-band (flags only ~35% of quest-era
beats; 95% agreement with the z-method on settled in-band beats) — treat it
as an in-band corroborator, not a detector. The z-method is the decision
metric.

## collapse_metrics.py

Usage (standalone; no harness imports):

```
python collapse_metrics.py <run_dir> [--window 20] [--tfidf-threshold 0.35]
    [--threshold 0.8] [--anchors "term1,term2"] [--no-bge] [--out DIR]
```

`<run_dir>` must contain `turns.jsonl`. Outputs go to `<run_dir>/analysis/`
(or `--out`): `collapse_metrics.csv`, `collapse_metrics.png`, plus bge
embedding caches (`*_embeddings.npz`) if that backend runs. Turns with empty
narrator content are skipped and reported.

### Primary metrics (TF-IDF)

Narrator beats are vectorized with TF-IDF fit on the run's own beats
(sklearn if available, else a small built-in implementation; English
stopwords removed, sublinear TF, max 5000 features, rows L2-normalized).
Per sliding window of W beats (stride 1):

- **Participation ratio** of the PCA spectrum, `PR = (Σλ)² / Σλ²`.
- **Motif monopoly share** — fraction of beats with cosine similarity to the
  window centroid above `--tfidf-threshold` (default 0.35; TF-IDF cosines are
  much lower than dense-embedding cosines, so this is a separate knob).
  **Validated weak on `default-guide-300`:** the metric is threshold-brittle
  (0.25 saturates at 1.0, 0.35 is flat ~0.6, 0.45 is near 0) because late-run
  beats are lexically diverse paraphrases of a fixed theme — window-level
  lexical tightness does not capture thematic collapse. Use the anchor rate
  and era-centroid cosine as primary signals; treat monopoly/PR/novelty as
  secondary context only.
- **Mean pairwise cosine similarity.**
- **Novelty rate** — per beat, the fraction of content bigrams
  (stopword-filtered) not seen in the previous 30 beats; windowed mean.
  Needs no fitting; drops as the run locks onto a motif monopoly.

### Anchor tracking (`--anchors`) — diagnostic overlay only

`--anchors "gerhardt,fence,toad"` tracks the per-window fraction of beats
mentioning any anchor term (case-insensitive word match, plural tolerated),
separately for narrator beats and player inputs. **Demoted from primary E1
metric** (2026-08-06): too probe-specific, misses paraphrase/coreference,
doesn't generalize across runs. Still useful as a human-readable overlay —
`relaxation.py --anchors` plots it alongside the baseline-return z-score.

### Secondary metrics (bge embeddings) — validated blind on this corpus

The same PR/monopoly/mean-cosine metrics over `BAAI/bge-base-en-v1.5`
sentence embeddings (the model `server/rhapsode/memory.py` uses) are kept as
an optional backend (`--no-bge` to skip). **Honest caveat:** on
`default-guide-300` this backend was validated as *blind* to the collapse.
The centroids of the quest era (turns 1–40) and the candle-covenant era
(turns 200–300) have bge cosine **0.986** — the embedding is dominated by the
shared narrator style, register, and setting, so its PR stays flat (~11–13)
and monopoly share saturates from turn 1 at any usable threshold. TF-IDF on
the same eras gives centroid cosine **0.699** and does separate them; hence
the lexical metrics are primary. Treat bge PR as a style-diversity signal,
not a motif-collapse signal.

### Summary endpoints printed per run

Era-centroid cosine (TF-IDF and bge), early/late TF-IDF PR, onset t\*
(first window where TF-IDF PR < 50% of the turns-1–40 baseline for ≥ 3
consecutive windows), monopoly share by block, early/late novelty rate, and
anchor-rate decay if anchors were given.
