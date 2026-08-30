"""Name-hit windows. Overlapping windows merge into one section."""

from __future__ import annotations

import argparse
import json
import sys

from corpus import ROOT, hit_pattern, load_config, load_lines, load_volumes


def find_hits(lines: list[str], names: list[str]) -> list[int]:
    pat = hit_pattern(names)
    return [i for i, line in enumerate(lines) if pat.search(line)]


def volume_bounds(line_0: int, volumes: list[dict]) -> tuple[int, int, int]:
    """Volume id and 0-based inclusive bounds for a 0-based line."""
    line_1 = line_0 + 1
    for vol in volumes:
        if vol["start"] <= line_1 <= vol["end"]:
            return int(vol["id"]), vol["start"] - 1, vol["end"] - 1
    last = volumes[-1]
    return int(last["id"]), last["start"] - 1, last["end"] - 1


def window_spans(
    hits: list[int], volumes: list[dict], window: int
) -> list[tuple[int, int, int, int]]:
    """Each hit opens [hit-W, hit+W], clipped to its volume. Overlapping or
    touching windows in the same volume become one section.
    Returns (volume_id, lo_0, hi_0, hit_count).
    """
    raw: list[tuple[int, int, int]] = []
    for h in hits:
        vid, vlo, vhi = volume_bounds(h, volumes)
        raw.append((vid, max(vlo, h - window), min(vhi, h + window)))
    raw.sort()
    out: list[tuple[int, int, int, int]] = []
    for vid, lo, hi in raw:
        if out and out[-1][0] == vid and lo <= out[-1][2] + 1:
            pvid, plo, phi, n = out[-1]
            out[-1] = (pvid, plo, max(phi, hi), n + 1)
        else:
            out.append((vid, lo, hi, 1))
    return out


def build_sections(lines: list[str], cfg: dict) -> list[dict]:
    ext = cfg["extract"]
    volumes = load_volumes(cfg)
    hits = find_hits(lines, ext["names"])
    window = int(ext["window"])
    sections = []
    for i, (vid, a, b, n_hits) in enumerate(window_spans(hits, volumes, window), start=1):
        start_1 = a + 1
        end_1 = b + 1
        sections.append(
            {
                "id": i,
                "volume": vid,
                "line_start": start_1,
                "line_end": end_1,
                "hit_count": n_hits,
                "text": "\n".join(lines[a : b + 1]),
            }
        )
    return sections


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract Darkness sections from konosuba.txt")
    parser.add_argument("--out", default="", help="jsonl path (default: config)")
    args = parser.parse_args()
    cfg = load_config()
    lines = load_lines(cfg)
    sections = build_sections(lines, cfg)
    out = ROOT / (args.out or cfg["paths"]["sections"])
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8") as f:
        for row in sections:
            f.write(json.dumps(row, ensure_ascii=False) + "\n")
    print(f"wrote {len(sections)} sections to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
