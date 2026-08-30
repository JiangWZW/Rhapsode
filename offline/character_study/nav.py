"""Navigation helpers the critic tools call. Caps live here so tests do not need an LLM."""

from __future__ import annotations

from corpus import load_volumes, read_brief


def clip_range(start: int, end: int, n_lines: int, cap: int) -> tuple[int, int]:
    """1-based inclusive. If the span is wider than cap, keep the start and trim the end."""
    if start < 1:
        start = 1
    if end < start:
        end = start
    if end > n_lines:
        end = n_lines
    if end - start + 1 > cap:
        end = start + cap - 1
    return start, end


def format_lines(lines: list[str], start: int, end: int, cap: int) -> str:
    a, b = clip_range(start, end, len(lines), cap)
    body = "\n".join(f"{i}|{lines[i - 1]}" for i in range(a, b + 1))
    note = ""
    if (end - start + 1) > cap:
        note = f"\n(trimmed to {cap} lines; asked {start}-{end})\n"
    return f"lines {a}-{b}\n{body}{note}"


def format_sources(
    volumes: list[dict],
    sections: list[dict],
    current_id: int,
    nearby: int = 3,
) -> str:
    vol_rows = [f"vol {v['id']}: {v['title']} lines {v['start']}-{v['end']}" for v in volumes]
    by_id = {s["id"]: s for s in sections}
    cur = by_id.get(current_id)
    near = []
    if cur:
        for s in sections:
            if abs(s["id"] - current_id) <= nearby:
                near.append(
                    f"section {s['id']} vol={s['volume']} lines {s['line_start']}-{s['line_end']} hits={s['hit_count']}"
                )
    return "volumes:\n" + "\n".join(vol_rows) + "\n\nnearby sections:\n" + "\n".join(near)


def format_brief(volume: int) -> str:
    titles = {v["id"]: v["title"] for v in load_volumes()}
    title = titles.get(volume, "")
    return f"volume {volume} {title}\n\n{read_brief(volume)}"
