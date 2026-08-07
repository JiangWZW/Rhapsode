"""Collapse metrics for autonomous session runs (E0 instrument, revision 2).

Primary metrics are computed on TF-IDF vectors of narrator beats (fit on the
run's own beats), which — unlike raw bge sentence embeddings — actually
separate the known story eras of the reference run. Per sliding window of W
beats (stride 1):

  - participation ratio (PR) of the PCA spectrum,
  - motif monopoly share (fraction of beats within cosine threshold of the
    window centroid),
  - mean pairwise cosine similarity,
  - novelty rate: mean fraction of content bigrams per beat unseen in the
    previous 30 beats,
  - optional anchor mention rate (--anchors) on narrator beats and player
    inputs separately (diagnostic overlay; the primary E1 relaxation metric
    is the baseline-return z-score in relaxation.py).

The original bge-embedding metrics (BAAI/bge-base-en-v1.5, same model as
server/rhapsode/memory.py) are retained as an optional secondary backend
(--no-bge to skip); on this corpus they were validated as BLIND to the
collapse (era-centroid cosine 0.986 — style dominates the embedding).

Standalone: no imports from the harness or server modules.

Usage:
    python collapse_metrics.py <run_dir> [--window 20] [--tfidf-threshold 0.35]
        [--threshold 0.8] [--anchors "term1,term2"] [--no-bge] [--out DIR]
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import re
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

EMBEDDING_MODEL = "BAAI/bge-base-en-v1.5"
NOVELTY_LOOKBACK = 30
EARLY_ERA = (1, 40)
LATE_ERA = (200, 300)

_TOKEN_RE = re.compile(r"[a-z']+")

# Minimal fallback if sklearn's ENGLISH_STOP_WORDS is unavailable.
_FALLBACK_STOPWORDS = frozenset(
    """a about above after again all am an and any are as at be because been
    before being below between both but by could did do does doing down during
    each few for from further had has have having he her here hers herself him
    himself his how i if in into is it its itself just me more most my myself
    no nor not now of off on once only or other our ours ourselves out over
    own s same she should so some such t than that the their theirs them
    themselves then there these they this those through to too under until up
    very was we were what when where which while who whom why will with you
    your yours yourself yourselves""".split()
)


def get_stopwords() -> frozenset[str]:
    try:
        from sklearn.feature_extraction.text import ENGLISH_STOP_WORDS  # noqa: PLC0415

        return frozenset(ENGLISH_STOP_WORDS)
    except ImportError:
        return _FALLBACK_STOPWORDS


# --------------------------------------------------------------------------
# Parsing
# --------------------------------------------------------------------------

@dataclass
class Beat:
    turn: int
    text: str


def parse_turns(turns_path: Path) -> tuple[list[Beat], list[Beat], list[int]]:
    """Return (narrator_beats, player_beats, empty_narrator_turns)."""
    narrator: list[Beat] = []
    player: list[Beat] = []
    empty_turns: list[int] = []
    with turns_path.open(encoding="utf-8") as fh:
        for line in fh:
            line = line.strip()
            if not line:
                continue
            rec = json.loads(line)
            turn = int(rec["turn"])
            prose = "\n\n".join(
                m.get("content", "")
                for m in rec.get("messages", [])
                if m.get("type") == "scene_message"
                and m.get("scene_kind") == "narrator"
                and m.get("content")
            ).strip()
            if prose:
                narrator.append(Beat(turn, prose))
            else:
                empty_turns.append(turn)
            inp = (rec.get("input") or "").strip()
            player.append(Beat(turn, inp if inp else "(empty input)"))
    narrator.sort(key=lambda b: b.turn)
    player.sort(key=lambda b: b.turn)
    return narrator, player, empty_turns


# --------------------------------------------------------------------------
# Vectorization backends
# --------------------------------------------------------------------------

def tfidf_vectorize(texts: list[str]) -> tuple[np.ndarray, str]:
    """TF-IDF matrix (rows L2-normalized), fit on the given texts."""
    try:
        from sklearn.feature_extraction.text import TfidfVectorizer  # noqa: PLC0415

        vec = TfidfVectorizer(
            max_features=5000, sublinear_tf=True, stop_words="english")
        mat = np.asarray(vec.fit_transform(texts).todense(), dtype=np.float64)
        return l2_normalize(mat), "tfidf:sklearn"
    except ImportError:
        return l2_normalize(_tfidf_pure(texts)), "tfidf:pure-python"


def _tfidf_pure(texts: list[str], max_features: int = 5000) -> np.ndarray:
    """Hand-rolled TF-IDF: sublinear TF, smoothed IDF, stopwords removed."""
    stop = get_stopwords()
    docs = [[t for t in _TOKEN_RE.findall(x.lower()) if t not in stop]
            for x in texts]
    df: dict[str, int] = {}
    for doc in docs:
        for tok in set(doc):
            df[tok] = df.get(tok, 0) + 1
    top = sorted(df, key=lambda t: (-df[t], t))[:max_features]
    vocab = {tok: i for i, tok in enumerate(top)}
    n = len(docs)
    mat = np.zeros((n, len(vocab)), dtype=np.float64)
    for row, doc in enumerate(docs):
        counts: dict[str, int] = {}
        for tok in doc:
            if tok in vocab:
                counts[tok] = counts.get(tok, 0) + 1
        for tok, cnt in counts.items():
            idf = math.log((1 + n) / (1 + df[tok])) + 1.0
            mat[row, vocab[tok]] = (1.0 + math.log(cnt)) * idf
    return mat


def embed_bge(texts: list[str], cache_path: Path) -> np.ndarray | None:
    """bge embeddings with an .npz cache; returns None if the model fails."""
    digest = hashlib.sha256("\x00".join(texts).encode("utf-8")).hexdigest()
    if cache_path.exists():
        with np.load(cache_path, allow_pickle=False) as z:
            if str(z["digest"]) == digest:
                return np.asarray(z["emb"], dtype=np.float64)
    try:
        from sentence_transformers import SentenceTransformer  # noqa: PLC0415

        try:
            model = SentenceTransformer(EMBEDDING_MODEL, local_files_only=True)
        except Exception:
            model = SentenceTransformer(EMBEDDING_MODEL)
        emb = np.asarray(
            model.encode(texts, batch_size=32, show_progress_bar=False),
            dtype=np.float64)
    except Exception as exc:
        print(f"bge backend unavailable ({exc!r}); skipping embedding metrics.",
              file=sys.stderr)
        return None
    np.savez_compressed(cache_path, emb=emb, digest=digest,
                        backend=f"sentence-transformers:{EMBEDDING_MODEL}")
    return emb


def l2_normalize(x: np.ndarray) -> np.ndarray:
    norms = np.linalg.norm(x, axis=1, keepdims=True)
    norms[norms == 0.0] = 1.0
    return x / norms


# --------------------------------------------------------------------------
# Per-window metrics
# --------------------------------------------------------------------------

def participation_ratio(window: np.ndarray) -> float:
    """PR = (sum lambda)^2 / sum lambda^2 over covariance eigenvalues."""
    centered = window - window.mean(axis=0, keepdims=True)
    svals = np.linalg.svd(centered, compute_uv=False)
    lam = (svals ** 2) / max(window.shape[0] - 1, 1)
    total = lam.sum()
    if total <= 0.0:
        return float("nan")
    return float(total ** 2 / np.sum(lam ** 2))


def monopoly_share(window: np.ndarray, threshold: float) -> float:
    centroid = window.mean(axis=0)
    norm = np.linalg.norm(centroid)
    if norm == 0.0:
        return float("nan")
    cos = window @ (centroid / norm)
    return float(np.mean(cos > threshold))


def mean_pairwise_cos(window: np.ndarray) -> float:
    n = window.shape[0]
    if n < 2:
        return float("nan")
    gram = window @ window.T
    return float((gram.sum() - np.trace(gram)) / (n * (n - 1)))


def window_metric_series(
    emb: np.ndarray, w: int, threshold: float
) -> tuple[list[float], list[float], list[float]]:
    """(PR, monopoly, mean_cos) per sliding window, stride 1."""
    prs, mons, coss = [], [], []
    for i in range(emb.shape[0] - w + 1):
        win = emb[i : i + w]
        prs.append(participation_ratio(win))
        mons.append(monopoly_share(win, threshold))
        coss.append(mean_pairwise_cos(win))
    return prs, mons, coss


def windowed_mean(values: list[float], w: int) -> list[float]:
    arr = np.asarray(values, dtype=np.float64)
    return [float(np.nanmean(arr[i : i + w])) for i in range(len(arr) - w + 1)]


# --------------------------------------------------------------------------
# Novelty rate
# --------------------------------------------------------------------------

def content_bigrams(text: str, stopwords: frozenset[str]) -> set[tuple[str, str]]:
    toks = [t for t in _TOKEN_RE.findall(text.lower())
            if t not in stopwords and len(t) > 2]
    return set(zip(toks, toks[1:]))


def novelty_rates(
    texts: list[str], stopwords: frozenset[str], lookback: int = NOVELTY_LOOKBACK
) -> list[float]:
    """Per beat: fraction of its content bigrams unseen in the previous
    `lookback` beats. NaN for the first beat and for beats with no bigrams."""
    grams = [content_bigrams(t, stopwords) for t in texts]
    rates: list[float] = []
    for i, g in enumerate(grams):
        if i == 0 or not g:
            rates.append(float("nan"))
            continue
        seen: set[tuple[str, str]] = set()
        for prev in grams[max(0, i - lookback) : i]:
            seen |= prev
        rates.append(len(g - seen) / len(g))
    return rates


# --------------------------------------------------------------------------
# Anchor tracking
# --------------------------------------------------------------------------

def anchor_flags(texts: list[str], anchors: list[str]) -> list[float]:
    """1.0 if the beat mentions any anchor term (case-insensitive word
    match, plural 's' tolerated), else 0.0."""
    pattern = re.compile(
        r"\b(?:" + "|".join(re.escape(a.strip().lower()) for a in anchors if a.strip())
        + r")s?\b")
    return [1.0 if pattern.search(t.lower()) else 0.0 for t in texts]


# --------------------------------------------------------------------------
# Era separation and summary endpoints
# --------------------------------------------------------------------------

def era_centroid_cos(
    emb: np.ndarray, turns: np.ndarray,
    era_a: tuple[int, int], era_b: tuple[int, int],
) -> float:
    a = emb[(turns >= era_a[0]) & (turns <= era_a[1])].mean(axis=0)
    b = emb[(turns >= era_b[0]) & (turns <= era_b[1])].mean(axis=0)
    return float(a @ b / (np.linalg.norm(a) * np.linalg.norm(b)))


def detect_onset(
    centers: list[int], series: list[float], baseline: float, sustain: int = 3
) -> int | None:
    """First center turn where the series < 50% of baseline for >= sustain
    consecutive windows."""
    cut = 0.5 * baseline
    run = 0
    for idx, v in enumerate(series):
        if not math.isnan(v) and v < cut:
            run += 1
            if run >= sustain:
                return centers[idx - sustain + 1]
        else:
            run = 0
    return None


def block_mean(centers: list[int], series: list[float], lo: int, hi: int) -> float:
    vals = [v for c, v in zip(centers, series) if lo <= c <= hi]
    return float(np.nanmean(vals)) if vals else float("nan")


# --------------------------------------------------------------------------
# Output
# --------------------------------------------------------------------------

def write_csv(path: Path, columns: dict[str, list]) -> None:
    names = list(columns)
    n = len(columns[names[0]])
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        writer.writerow(names)
        for i in range(n):
            row = []
            for name in names:
                v = columns[name][i]
                if isinstance(v, float):
                    row.append("" if math.isnan(v) else f"{v:.4f}")
                else:
                    row.append(v)
            writer.writerow(row)


def make_plot(
    path: Path, run_name: str, w: int, columns: dict[str, list],
    tfidf_threshold: float, anchors: list[str] | None, has_bge: bool,
) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    turns = columns["center_turn"]
    n_panels = 3 + (1 if anchors else 0)
    fig, axes = plt.subplots(n_panels, 1, figsize=(11, 2.7 * n_panels),
                             sharex=True)

    ax = axes[0]
    ax.plot(turns, columns["pr_tfidf"], color="#1f4e79", lw=1.6,
            label="PR, TF-IDF (narrator)")
    if has_bge:
        ax.plot(turns, columns["pr_bge"], color="#8a8a8a", lw=1.0, ls="--",
                label="PR, bge (narrator; validated blind)")
    ax.set_ylabel("Participation ratio")
    ax.legend(loc="upper right", fontsize=8)

    ax = axes[1]
    ax.plot(turns, columns["monopoly_tfidf"], color="#7a1f1f", lw=1.6,
            label=f"Monopoly share, TF-IDF (cos > {tfidf_threshold})")
    ax.plot(turns, columns["mean_cos_tfidf"], color="#4a4a4a", lw=1.0, ls=":",
            label="Mean pairwise cosine, TF-IDF")
    ax.set_ylabel("Share / cosine")
    ax.set_ylim(bottom=0.0)
    ax.legend(loc="upper right", fontsize=8)

    ax = axes[2]
    ax.plot(turns, columns["novelty"], color="#1f6e3a", lw=1.6,
            label=f"Novelty rate (bigrams unseen in prev {NOVELTY_LOOKBACK} beats)")
    ax.set_ylabel("Novelty rate")
    ax.set_ylim(0.0, 1.0)
    ax.legend(loc="upper right", fontsize=8)

    if anchors:
        ax = axes[3]
        ax.plot(turns, columns["anchor_rate_narrator"], color="#5a2d82",
                lw=1.6, label="Anchor rate (narrator)")
        ax.plot(turns, columns["anchor_rate_player"], color="#b05a20",
                lw=1.2, ls="--", label="Anchor rate (player)")
        ax.set_ylabel("Mention rate")
        ax.set_ylim(0.0, 1.05)
        ax.legend(loc="upper right", fontsize=8)
        ax.set_title(f"Anchors: {', '.join(anchors)}", fontsize=9)

    for ax in axes:
        ax.grid(alpha=0.3)
        ax.axvspan(50, 100, color="#c9a227", alpha=0.15, lw=0)
    axes[-1].set_xlabel("Turn (window center)")
    axes[0].annotate("predicted collapse (turns 50\u2013100)",
                     xy=(75, axes[0].get_ylim()[1]),
                     xytext=(75, axes[0].get_ylim()[1] * 0.98),
                     ha="center", va="top", fontsize=8, color="#7a6210")
    fig.suptitle(
        f"Collapse metrics \u2014 {run_name} (window={w} beats, stride 1)")
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("run_dir", type=Path, help="run directory containing turns.jsonl")
    ap.add_argument("--window", type=int, default=20,
                    help="sliding window size (beats)")
    ap.add_argument("--tfidf-threshold", type=float, default=0.35,
                    help="cosine threshold for TF-IDF monopoly share")
    ap.add_argument("--threshold", type=float, default=0.8,
                    help="cosine threshold for bge monopoly share")
    ap.add_argument("--anchors", type=str, default=None,
                    help="comma-separated anchor terms to track")
    ap.add_argument("--no-bge", action="store_true",
                    help="skip the (secondary) bge embedding metrics")
    ap.add_argument("--out", type=Path, default=None,
                    help="output directory (default: <run_dir>/analysis)")
    args = ap.parse_args(argv)

    run_dir: Path = args.run_dir
    turns_path = run_dir / "turns.jsonl"
    if not turns_path.exists():
        ap.error(f"not found: {turns_path}")
    out_dir: Path = args.out if args.out is not None else run_dir / "analysis"
    out_dir.mkdir(parents=True, exist_ok=True)
    anchors = ([a.strip() for a in args.anchors.split(",") if a.strip()]
               if args.anchors else None)

    narrator, player, empty_turns = parse_turns(turns_path)
    print(f"Parsed {len(narrator)} narrator beats, {len(player)} player inputs.")
    if empty_turns:
        print(f"Skipped {len(empty_turns)} turns with empty narrator content: "
              f"{empty_turns}")

    w = args.window
    if len(narrator) < w or len(player) < w:
        print(f"error: fewer beats than window size {w}", file=sys.stderr)
        return 1

    nar_texts = [b.text for b in narrator]
    nar_turns = np.array([b.turn for b in narrator])
    centers = [narrator[i + w // 2].turn
               for i in range(len(narrator) - w + 1)]
    stopwords = get_stopwords()

    # --- primary: TF-IDF ---
    tfidf, tfidf_backend = tfidf_vectorize(nar_texts)
    print(f"Primary backend: {tfidf_backend} "
          f"({tfidf.shape[1]} features, fit on this run)")
    pr_t, mon_t, cos_t = window_metric_series(tfidf, w, args.tfidf_threshold)
    era_t = era_centroid_cos(tfidf, nar_turns, EARLY_ERA, LATE_ERA)

    # --- novelty ---
    nov_beat = novelty_rates(nar_texts, stopwords)
    nov = windowed_mean(nov_beat, w)

    columns: dict[str, list] = {
        "center_turn": centers,
        "pr_tfidf": pr_t,
        "monopoly_tfidf": mon_t,
        "mean_cos_tfidf": cos_t,
        "novelty": nov,
    }

    # --- anchors ---
    if anchors:
        flags_n = anchor_flags(nar_texts, anchors)
        flags_p = anchor_flags([b.text for b in player], anchors)
        rate_p_by_center = {
            player[i + w // 2].turn: float(np.mean(flags_p[i : i + w]))
            for i in range(len(player) - w + 1)
        }
        columns["anchor_rate_narrator"] = windowed_mean(flags_n, w)
        columns["anchor_rate_player"] = [
            rate_p_by_center.get(
                c, rate_p_by_center[min(rate_p_by_center,
                                        key=lambda t: abs(t - c))])
            for c in centers
        ]

    # --- secondary: bge embeddings ---
    era_b: float | None = None
    has_bge = False
    if not args.no_bge:
        nar_emb = embed_bge(nar_texts, out_dir / "narrator_embeddings.npz")
        ply_emb = embed_bge([b.text for b in player],
                            out_dir / "player_embeddings.npz")
        if nar_emb is not None:
            nar_emb = l2_normalize(nar_emb)
            pr_b, mon_b, cos_b = window_metric_series(nar_emb, w, args.threshold)
            columns["pr_bge"] = pr_b
            columns["monopoly_bge"] = mon_b
            columns["mean_cos_bge"] = cos_b
            era_b = era_centroid_cos(nar_emb, nar_turns, EARLY_ERA, LATE_ERA)
            has_bge = True
        if ply_emb is not None:
            ply_emb = l2_normalize(ply_emb)
            pr_by_center = {
                player[i + w // 2].turn: participation_ratio(ply_emb[i : i + w])
                for i in range(len(player) - w + 1)
            }
            columns["pr_player_bge"] = [
                pr_by_center.get(
                    c, pr_by_center[min(pr_by_center,
                                        key=lambda t: abs(t - c))])
                for c in centers
            ]

    csv_path = out_dir / "collapse_metrics.csv"
    png_path = out_dir / "collapse_metrics.png"
    write_csv(csv_path, columns)
    try:
        make_plot(png_path, run_dir.name, w, columns, args.tfidf_threshold,
                  anchors, has_bge)
    except ModuleNotFoundError as err:
        print(f"Plot skipped ({err}); CSV is complete. "
              f"Install matplotlib in this interpreter to get plots.")
        png_path = None

    # --- summary ---
    early_pr = block_mean(centers, pr_t, 1, 40)
    print("\n=== Summary (primary = TF-IDF) ===")
    print(f"Era-centroid cosine, turns {EARLY_ERA[0]}-{EARLY_ERA[1]} vs "
          f"{LATE_ERA[0]}-{LATE_ERA[1]}: TF-IDF={era_t:.3f}"
          + (f", bge={era_b:.3f}" if era_b is not None else ""))
    print(f"PR TF-IDF: early mean (centers <= 40) {early_pr:.3f}, "
          f"late mean (centers >= 120) {block_mean(centers, pr_t, 120, 10**9):.3f}")
    onset = detect_onset(centers, pr_t, early_pr)
    print(f"Onset t* (TF-IDF PR < 50% of early baseline, 3+ windows): "
          f"{onset if onset is not None else 'not reached'}")
    print(f"Monopoly TF-IDF (cos > {args.tfidf_threshold}): "
          f"early {block_mean(centers, mon_t, 1, 40):.3f}, "
          f"turns 60-100 {block_mean(centers, mon_t, 60, 100):.3f}, "
          f"late (>= 120) {block_mean(centers, mon_t, 120, 10**9):.3f}, "
          f"final {mon_t[-1]:.3f}")
    print(f"Novelty rate: early (centers <= 40) "
          f"{block_mean(centers, nov, 1, 40):.3f}, "
          f"late (>= 120) {block_mean(centers, nov, 120, 10**9):.3f}")
    if anchors:
        rate_n = columns["anchor_rate_narrator"]
        print(f"Anchor rate (narrator, {','.join(anchors)}): "
              f"turns 1-50 {block_mean(centers, rate_n, 1, 50):.3f}, "
              f"turns 251-300 {block_mean(centers, rate_n, 251, 300):.3f}")
    print(f"\nCSV:  {csv_path}")
    print(f"Plot: {png_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
