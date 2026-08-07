"""Baseline-return (distance-to-attractor) relaxation metric for E1.

Primary E1 relaxation measure, replacing hand-picked anchor terms (too
probe-specific, blind to paraphrase/coreference). Idea: model the collapsed
attractor as the TF-IDF centroid of the last N narrator beats of a baseline
run, score every beat of the perturbation run by how far it sits from that
centroid relative to the baseline's own spread (a one-sided z-score), and
measure how long after each injected probe the story takes to fall back
in-band.

Method (lexical on purpose — bge embeddings were validated style-blind on
this corpus, see README):
  - TF-IDF (sublinear, English stopwords, 5000 features) fit on the union of
    baseline-last-N beats and all perturbation-run beats, rows L2-normalized.
  - Baseline model: centroid c of the baseline beats; mu/sigma of the
    baseline beats' own cosines to c, both series smoothed with a trailing
    --smooth-beat mean (default 3) — per-beat cosines were validated too
    noisy (positive control marginal at N=60 without smoothing).
  - Per run beat: z = (smoothed cos(beat, c) - mu) / sigma; in-band iff
    z >= -2 (one-sided: probes push AWAY from the attractor).
  - Relaxation time for an injection at turn T: smallest r such that the
    k (default 3) consecutive beats starting at turn T+r are all in-band.
  - Secondary robustness check: Jensen-Shannon divergence between the
    unigram distribution of a trailing 5-beat window and the baseline
    unigram distribution, with an in-band ceiling calibrated on the
    baseline's own rolling windows (leave-window-out).

Usage:
    python relaxation.py --run RUN_DIR --baseline RUN_DIR_OR_TURNS_JSONL
        [--baseline-last 60] [--injections FILE] [--k 3] [--z-min -2]
        [--smooth 3] [--anchors "a,b"] [--out DIR]
    python relaxation.py --validate REF_RUN_DIR   # controls + smoke test
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
import tomllib
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from collapse_metrics import (
    _TOKEN_RE,
    Beat,
    anchor_flags,
    get_stopwords,
    parse_turns,
    tfidf_vectorize,
)

Z_MIN_DEFAULT = -2.0
SMOOTH_DEFAULT = 3
JS_WINDOW = 5
JS_SIGMA = 2.0


# --------------------------------------------------------------------------
# Injection file (same {turn, text, label} format as injection.py)
# --------------------------------------------------------------------------

@dataclass(frozen=True)
class Injection:
    turn: int
    label: str


def load_injections(path: Path) -> list[Injection]:
    """Rows of {turn, text, label} from TOML ([[injection]]/[[injections]])
    or JSON (top-level list). Only turn and label are needed here."""
    if path.suffix.lower() == ".json":
        rows = json.loads(path.read_text(encoding="utf-8"))
    else:
        with path.open("rb") as fh:
            data = tomllib.load(fh)
        rows = data.get("injection", data.get("injections", []))
    out = [Injection(int(r["turn"]), str(r.get("label", f"turn{r['turn']}")))
           for r in rows]
    return sorted(out, key=lambda i: i.turn)


# --------------------------------------------------------------------------
# Baseline (attractor) model
# --------------------------------------------------------------------------

@dataclass
class BaselineModel:
    centroid: np.ndarray  # unit-norm
    mu: float
    sigma: float


def trailing_mean(x: np.ndarray, w: int) -> np.ndarray:
    """Causal trailing mean: out[i] = mean(x[max(0, i-w+1) .. i])."""
    if w <= 1:
        return x.astype(np.float64)
    return np.array([float(np.mean(x[max(0, i - w + 1) : i + 1]))
                     for i in range(len(x))])


def fit_baseline(base_vecs: np.ndarray, smooth: int) -> BaselineModel:
    centroid = base_vecs.mean(axis=0)
    centroid = centroid / np.linalg.norm(centroid)
    cos = base_vecs @ centroid
    # Band from FULL trailing windows only, so partial-window edge values
    # don't widen sigma.
    smoothed = (np.convolve(cos, np.ones(smooth) / smooth, mode="valid")
                if smooth > 1 else cos)
    sigma = float(np.std(smoothed))
    return BaselineModel(centroid, float(np.mean(smoothed)), max(sigma, 1e-9))


def z_scores(
    base_texts: list[str], run_texts: list[str], smooth: int = SMOOTH_DEFAULT
) -> tuple[np.ndarray, BaselineModel]:
    """TF-IDF fit on union(base, run); one-sided z-score of each run beat's
    trailing-`smooth`-beat mean cosine vs the baseline attractor band."""
    vecs, _ = tfidf_vectorize(base_texts + run_texts)
    base_vecs, run_vecs = vecs[: len(base_texts)], vecs[len(base_texts):]
    model = fit_baseline(base_vecs, smooth)
    run_cos = trailing_mean(run_vecs @ model.centroid, smooth)
    z = (run_cos - model.mu) / model.sigma
    return z, model


# --------------------------------------------------------------------------
# Jensen-Shannon secondary check (unigram distributions)
# --------------------------------------------------------------------------

def unigram_counts(text: str, stopwords: frozenset[str]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for tok in _TOKEN_RE.findall(text.lower()):
        if tok not in stopwords and len(tok) > 2:
            counts[tok] = counts.get(tok, 0) + 1
    return counts


def merge_counts(dicts: list[dict[str, int]]) -> dict[str, int]:
    out: dict[str, int] = {}
    for d in dicts:
        for k, v in d.items():
            out[k] = out.get(k, 0) + v
    return out


def js_divergence(p_counts: dict[str, int], q_counts: dict[str, int]) -> float:
    """JSD (log2, in [0, 1]) between two unigram count distributions."""
    if not p_counts or not q_counts:
        return float("nan")
    keys = list(p_counts.keys() | q_counts.keys())
    p = np.array([p_counts.get(k, 0) for k in keys], dtype=np.float64)
    q = np.array([q_counts.get(k, 0) for k in keys], dtype=np.float64)
    p /= p.sum()
    q /= q.sum()
    m = 0.5 * (p + q)

    def kl(a: np.ndarray, b: np.ndarray) -> float:
        mask = a > 0
        return float(np.sum(a[mask] * np.log2(a[mask] / b[mask])))

    return 0.5 * kl(p, m) + 0.5 * kl(q, m)


def js_series(
    run_counts: list[dict[str, int]], base_total: dict[str, int], w: int = JS_WINDOW
) -> list[float]:
    """Trailing-window JSD per beat (NaN for the first w-1 beats)."""
    out: list[float] = [float("nan")] * len(run_counts)
    for i in range(w - 1, len(run_counts)):
        window = merge_counts(run_counts[i - w + 1 : i + 1])
        out[i] = js_divergence(window, base_total)
    return out


def js_band_ceiling(
    base_counts: list[dict[str, int]], w: int = JS_WINDOW, n_sigma: float = JS_SIGMA
) -> float:
    """In-band JSD ceiling from the baseline's own rolling windows,
    leave-window-out: JSD(window, baseline minus window)."""
    total = merge_counts(base_counts)
    jsds: list[float] = []
    for i in range(len(base_counts) - w + 1):
        window = merge_counts(base_counts[i : i + w])
        rest = {k: v - window.get(k, 0) for k, v in total.items()}
        rest = {k: v for k, v in rest.items() if v > 0}
        jsds.append(js_divergence(window, rest))
    arr = np.array(jsds, dtype=np.float64)
    return float(np.mean(arr) + n_sigma * np.std(arr))


# --------------------------------------------------------------------------
# Relaxation
# --------------------------------------------------------------------------

def relaxation_turn(
    beats: list[Beat], in_band: list[bool], inject_turn: int, k: int
) -> tuple[int, int] | None:
    """First turn at/after inject_turn starting k consecutive in-band beats.

    Returns (relaxed_turn, r) with r = relaxed_turn - inject_turn, or None
    if the run never relaxes."""
    start = next((j for j, b in enumerate(beats) if b.turn >= inject_turn), None)
    if start is None:
        return None
    for j in range(start, len(beats) - k + 1):
        if all(in_band[j : j + k]):
            return beats[j].turn, beats[j].turn - inject_turn
    return None


# --------------------------------------------------------------------------
# Core analysis (shared by main run mode and the smoke test)
# --------------------------------------------------------------------------

@dataclass
class Analysis:
    beats: list[Beat]
    z: np.ndarray
    in_band_z: list[bool]
    js: list[float]
    js_ceiling: float
    in_band_js: list[bool]
    model: BaselineModel


def analyze(
    base_beats: list[Beat], run_beats: list[Beat], z_min: float,
    smooth: int = SMOOTH_DEFAULT,
) -> Analysis:
    base_texts = [b.text for b in base_beats]
    run_texts = [b.text for b in run_beats]
    z, model = z_scores(base_texts, run_texts, smooth)
    in_band_z = [bool(v >= z_min) for v in z]

    stop = get_stopwords()
    base_counts = [unigram_counts(t, stop) for t in base_texts]
    run_counts = [unigram_counts(t, stop) for t in run_texts]
    js = js_series(run_counts, merge_counts(base_counts))
    ceiling = js_band_ceiling(base_counts)
    in_band_js = [bool(not math.isnan(v) and v <= ceiling) for v in js]
    return Analysis(run_beats, z, in_band_z, js, ceiling, in_band_js, model)


def report_probes(
    ana: Analysis, injections: list[Injection], k: int
) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for inj in injections:
        rz = relaxation_turn(ana.beats, ana.in_band_z, inj.turn, k)
        rj = relaxation_turn(ana.beats, ana.in_band_js, inj.turn, k)
        rows.append({
            "label": inj.label,
            "turn": inj.turn,
            "relaxed_turn_z": rz[0] if rz else None,
            "r_z": rz[1] if rz else None,
            "relaxed_turn_js": rj[0] if rj else None,
            "r_js": rj[1] if rj else None,
        })
    return rows


def method_agreement(ana: Analysis) -> float:
    pairs = [(a, b) for a, b, v in zip(ana.in_band_z, ana.in_band_js, ana.js)
             if not math.isnan(v)]
    if not pairs:
        return float("nan")
    return float(np.mean([a == b for a, b in pairs]))


# --------------------------------------------------------------------------
# Output
# --------------------------------------------------------------------------

def write_csv(
    path: Path, ana: Analysis, anchor: list[float] | None
) -> None:
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh)
        header = ["turn", "z_score", "in_band_z", "js_div", "in_band_js"]
        if anchor is not None:
            header.append("anchor_flag")
        writer.writerow(header)
        for i, b in enumerate(ana.beats):
            row: list[object] = [
                b.turn, f"{ana.z[i]:.4f}", int(ana.in_band_z[i]),
                "" if math.isnan(ana.js[i]) else f"{ana.js[i]:.4f}",
                int(ana.in_band_js[i]),
            ]
            if anchor is not None:
                row.append(int(anchor[i]))
            writer.writerow(row)


def make_plot(
    path: Path, title: str, ana: Analysis, injections: list[Injection],
    probe_rows: list[dict[str, object]], z_min: float,
    anchor: list[float] | None,
) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    turns = [b.turn for b in ana.beats]
    n_panels = 2 + (1 if anchor is not None else 0)
    fig, axes = plt.subplots(n_panels, 1, figsize=(11, 3.0 * n_panels),
                             sharex=True)

    ax = axes[0]
    ax.plot(turns, ana.z, color="#1f4e79", lw=1.4, label="z(beat vs attractor)")
    lo = min(float(np.min(ana.z)) - 1.0, z_min - 2.0)
    ax.axhspan(lo, z_min, color="#a33", alpha=0.12, lw=0)
    ax.axhline(z_min, color="#a33", lw=1.0, ls="--",
               label=f"in-band floor (z = {z_min:g})")
    for row in probe_rows:
        if row["relaxed_turn_z"] is not None:
            idx = turns.index(row["relaxed_turn_z"])
            ax.plot(turns[idx], ana.z[idx], "o", color="#1f6e3a", ms=7,
                    zorder=5)
    ax.set_ylabel("z-score (TF-IDF cos to attractor)")
    ax.legend(loc="lower right", fontsize=8)

    ax = axes[1]
    ax.plot(turns, ana.js, color="#5a2d82", lw=1.4,
            label=f"JSD (trailing {JS_WINDOW}-beat window vs baseline)")
    ax.axhline(ana.js_ceiling, color="#a33", lw=1.0, ls="--",
               label=f"in-band ceiling ({ana.js_ceiling:.3f})")
    ax.set_ylabel("Jensen-Shannon divergence")
    ax.legend(loc="upper right", fontsize=8)

    if anchor is not None:
        ax = axes[2]
        roll = [float(np.mean(anchor[max(0, i - 4) : i + 1]))
                for i in range(len(anchor))]
        ax.plot(turns, roll, color="#b05a20", lw=1.4,
                label="Anchor rate, trailing 5 beats (diagnostic only)")
        ax.set_ylabel("Anchor rate")
        ax.set_ylim(0.0, 1.05)
        ax.legend(loc="upper right", fontsize=8)

    for ax in axes:
        ax.grid(alpha=0.3)
        for inj in injections:
            ax.axvline(inj.turn, color="#c9a227", lw=1.2, ls=":")
    for inj in injections:
        axes[0].annotate(inj.label, xy=(inj.turn, axes[0].get_ylim()[1]),
                         ha="left", va="top", rotation=90, fontsize=7,
                         color="#7a6210")
    axes[-1].set_xlabel("Turn")
    fig.suptitle(title)
    fig.tight_layout()
    fig.savefig(path, dpi=150)
    plt.close(fig)


# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------

def resolve_turns_path(p: Path) -> Path:
    path = p if p.suffix == ".jsonl" else p / "turns.jsonl"
    if not path.exists():
        raise FileNotFoundError(path)
    return path


def load_narrator_beats(p: Path) -> list[Beat]:
    narrator, _, empty = parse_turns(resolve_turns_path(p))
    if empty:
        print(f"note: {p.name}: skipped {len(empty)} empty narrator turns "
              f"{empty}", file=sys.stderr)
    return narrator


# --------------------------------------------------------------------------
# Validation suite (controls on the reference run)
# --------------------------------------------------------------------------

def run_validation(ref_run: Path, z_min: float, k: int, smooth: int) -> None:
    beats = load_narrator_beats(ref_run)
    print(f"Validation on {ref_run.name}: {len(beats)} narrator beats "
          f"(smooth={smooth}, raw = per-beat shown for comparison).\n")

    hdr = (f"{'N':>4} | {'pos median z':>12} {'pos %out':>8} | "
           f"{'neg %in':>7} {'neg median z':>12} | {'raw pos z':>9}")
    print(hdr)
    for n in (40, 60, 80):
        # Positive control: early quest era vs attractor of last N beats.
        base = [b.text for b in beats[-n:]]
        early = [b.text for b in beats if b.turn <= 30]
        z_pos, _ = z_scores(base, early, smooth)
        z_pos_raw, _ = z_scores(base, early, 1)
        # Negative control: hold out last 10; baseline = the N before them.
        held = [b.text for b in beats[-10:]]
        base_neg = [b.text for b in beats[-(n + 10) : -10]]
        z_neg, _ = z_scores(base_neg, held, smooth)
        print(f"{n:>4} | {float(np.median(z_pos)):>12.2f} "
              f"{float(np.mean(z_pos < z_min)):>8.0%} | "
              f"{float(np.mean(z_neg >= z_min)):>7.0%} "
              f"{float(np.median(z_neg)):>12.2f} | "
              f"{float(np.median(z_pos_raw)):>9.2f}")

    # Smoke test of the per-probe reporting path: pretend an injection at
    # turn 250, baseline = last 60 beats of the same run (trivially in-band).
    print("\nSmoke test (mechanical only): injection at turn 250, "
          "baseline = last 60 beats of the same run")
    ana = analyze(beats[-60:], beats, z_min, smooth)
    probes = report_probes(ana, [Injection(250, "smoke")], k)
    for row in probes:
        print(f"  probe {row['label']!r} at turn {row['turn']}: "
              f"z-method relaxed at turn {row['relaxed_turn_z']} "
              f"(r={row['r_z']}), js-method relaxed at turn "
              f"{row['relaxed_turn_js']} (r={row['r_js']})")
    print(f"  method agreement over scored beats: {method_agreement(ana):.1%}")


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--run", type=Path, help="perturbation run dir")
    ap.add_argument("--baseline", type=Path,
                    help="baseline run dir or turns.jsonl")
    ap.add_argument("--baseline-last", type=int, default=60,
                    help="use the last N narrator beats of the baseline")
    ap.add_argument("--injections", type=Path, default=None,
                    help="TOML/JSON injection table ({turn, text, label})")
    ap.add_argument("--k", type=int, default=3,
                    help="consecutive in-band beats required for relaxation")
    ap.add_argument("--z-min", type=float, default=Z_MIN_DEFAULT,
                    help="one-sided in-band floor for the z-score")
    ap.add_argument("--smooth", type=int, default=SMOOTH_DEFAULT,
                    help="trailing-mean window (beats) for the cosine score")
    ap.add_argument("--anchors", type=str, default=None,
                    help="diagnostic overlay: comma-separated anchor terms")
    ap.add_argument("--out", type=Path, default=None,
                    help="output dir (default: <run>/analysis/relaxation)")
    ap.add_argument("--validate", type=Path, default=None, metavar="REF_RUN",
                    help="run positive/negative controls + smoke test on a "
                         "reference run instead of scoring a perturbation run")
    args = ap.parse_args(argv)

    if args.validate is not None:
        run_validation(args.validate, args.z_min, args.k, args.smooth)
        return 0
    if args.run is None or args.baseline is None:
        ap.error("--run and --baseline are required (or use --validate)")

    run_beats = load_narrator_beats(args.run)
    base_all = load_narrator_beats(args.baseline)
    base_beats = base_all[-args.baseline_last:]
    print(f"Run: {len(run_beats)} beats; baseline: last {len(base_beats)} of "
          f"{len(base_all)} beats from {args.baseline}")

    injections = load_injections(args.injections) if args.injections else []
    anchors = ([a.strip() for a in args.anchors.split(",") if a.strip()]
               if args.anchors else None)

    ana = analyze(base_beats, run_beats, args.z_min, args.smooth)
    probes = report_probes(ana, injections, args.k)
    anchor = (anchor_flags([b.text for b in run_beats], anchors)
              if anchors else None)

    out_dir = args.out if args.out is not None else args.run / "analysis" / "relaxation"
    out_dir.mkdir(parents=True, exist_ok=True)
    csv_path = out_dir / "relaxation.csv"
    png_path: Path | None = out_dir / "relaxation.png"
    write_csv(csv_path, ana, anchor)
    try:
        make_plot(png_path, f"Baseline-return relaxation \u2014 {args.run.name} "
                  f"(attractor: {args.baseline.name}, last "
                  f"{args.baseline_last} beats)",
                  ana, injections, probes, args.z_min, anchor)
    except ModuleNotFoundError as err:
        print(f"Plot skipped ({err}); CSV is complete.")
        png_path = None

    print(f"\nBaseline attractor: mu={ana.model.mu:.3f}, "
          f"sigma={ana.model.sigma:.3f}; in-band floor z >= {args.z_min:g}; "
          f"JSD ceiling {ana.js_ceiling:.3f}")
    frac = float(np.mean(ana.in_band_z))
    print(f"Run beats in-band (z-method): {frac:.1%}; "
          f"method agreement: {method_agreement(ana):.1%}")
    for row in probes:
        rz = (f"relaxed at turn {row['relaxed_turn_z']} (r={row['r_z']})"
              if row["relaxed_turn_z"] is not None else "NOT relaxed by end of run")
        rj = (f"turn {row['relaxed_turn_js']} (r={row['r_js']})"
              if row["relaxed_turn_js"] is not None else "not relaxed")
        print(f"Probe {row['label']!r} at turn {row['turn']}: {rz} "
              f"[JSD check: {rj}]")
    print(f"\nCSV:  {csv_path}")
    print(f"Plot: {png_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
