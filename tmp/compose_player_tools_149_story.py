"""Compose a novel-format reading edition of the 149-turn player-tools run.

Public text from scene saves + closures. Perceptions from console.log
(overwrite stream). Monologues from world.json (retained lines).
Nothing private is invented.
"""
from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path

RUN = Path(r"D:\cursor-workspace\Rhapsode\experiments\session_pipeline\runs\player-tools-300turn")
OUT = Path(r"D:\cursor-workspace\Rhapsode\wiki\research\player-tools-149-story-with-minds.md")

TURN_END_RE = re.compile(r"turn: end scene=(\S+) id=(\d+)")
PERC_RE = re.compile(r"perception: (\S+) t=(\d+)(?: \(empty\)| (.*))$")
FORK_RE = re.compile(r"story: fork from=(\S+) to=(\S+)")
MERGE_RE = re.compile(r"story: merge from=(\S+) into=(\S+)")
CONCLUDE_RE = re.compile(r"story: conclude scene=(\S+):")
BRACKET_RE = re.compile(r"^\[([^\]]+)\]\s*$")

CHAPTERS = [
    (0, 8, "I. A goddess at the door"),
    (9, 27, "II. Ash under the yew"),
    (28, 76, "III. The bottle, and the frost"),
    (77, 123, "IV. The far gate"),
    (124, 200, "V. Every row"),
]


def load(p: Path):
    return json.loads(p.read_text(encoding="utf-8"))


def beats_from_scene(scene: dict) -> dict[int, list[tuple[int, str, str]]]:
    """turn -> [(ordinal, speaker, content), ...]"""
    buckets: dict[int, list[tuple[int, str, str]]] = defaultdict(list)
    for message in scene.get("history", []) + scene.get("dialogue", []):
        content = (message.get("content") or "").strip()
        if not content:
            continue
        meta = message.get("metadata") or {}
        kind = meta.get("scene_kind") or ""
        if kind in ("director_cue", "fork", "merge"):
            continue
        if message.get("role") == "system":
            continue
        turn = meta.get("turn")
        if turn is None:
            buckets[-1].append((0, "Narrator", content))
            continue
        speaker = meta.get("speaker") or ""
        if kind == "player":
            speaker = "Kazuma"
        elif kind == "narrator" or not speaker:
            speaker = "Narrator"
        ordinal = int(meta.get("turn_ordinal") or 0)
        buckets[int(turn)].append((ordinal, speaker, content))
    for items in buckets.values():
        items.sort(key=lambda x: x[0])
    return buckets


def parse_console(path: Path):
    perceptions: dict[tuple[str, int], list[tuple[str, str]]] = defaultdict(list)
    ops_after: dict[int, list[tuple[str, ...]]] = defaultdict(list)
    scene, turn = "konosuba", -1
    last_kono = -1
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if m := TURN_END_RE.search(raw):
            scene, turn = m.group(1), int(m.group(2))
            if scene == "konosuba":
                last_kono = turn
            continue
        if m := PERC_RE.search(raw):
            who, t, text = m.group(1), int(m.group(2)), m.group(3)
            if text:
                perceptions[(scene, t)].append((who, text.strip()))
            continue
        pin = last_kono if last_kono >= 0 else turn
        if m := FORK_RE.search(raw):
            ops_after[pin].append(("fork", m.group(1), m.group(2)))
        elif m := MERGE_RE.search(raw):
            ops_after[pin].append(("merge", m.group(1), m.group(2)))
        elif m := CONCLUDE_RE.search(raw):
            ops_after[pin].append(("conclude", m.group(1)))
    return perceptions, ops_after


def closure_map(story: dict) -> dict[str, dict]:
    return {c["scene_id"]: c for c in story.get("scene_closures", [])}


def rewrite_tagged(block: str) -> str:
    lines = []
    speaker = None
    buf: list[str] = []

    def flush():
        if speaker is None or not buf:
            return
        text = "\n\n".join(buf).strip()
        if speaker == "Narrator":
            lines.append(text)
        elif speaker == "Story so far":
            lines.append("*What had happened on that thread:*\n\n" + text)
        elif speaker == "Final narration":
            lines.append(text)
        else:
            lines.append(f"**{speaker}.** {text}")
        buf.clear()

    for line in block.splitlines():
        m = BRACKET_RE.match(line.strip())
        if m:
            flush()
            speaker = m.group(1)
            continue
        buf.append(line)
    flush()
    return "\n\n".join(lines)


def emit_public(out: list[str], items: list[tuple[int, str, str]]) -> None:
    for _, speaker, content in items:
        if speaker == "Narrator":
            out.append(content)
        elif speaker == "Stage":
            out.append(f"*{content}*")
        else:
            out.append(f"**{speaker}.** {content}")
        out.append("")


def emit_minds(
    out: list[str],
    perceptions: list[tuple[str, str]],
    monologues: list[tuple[str, str]],
) -> None:
    if not perceptions and not monologues:
        return
    seen: dict[str, int] = defaultdict(int)
    for who, text in perceptions:
        out.append(f"*{who} sees.* {text}")
        out.append("")
    for who, text in monologues:
        seen[who] += 1
        extra = " *(second line, same turn number — likely a fork scene)*" if seen[who] > 1 else ""
        out.append(f"*{who}, privately.*{extra} {text}")
        out.append("")


def chapter_for(turn: int) -> str | None:
    for a, b, title in CHAPTERS:
        if a <= turn <= b:
            return title
    return None


def main() -> None:
    world = load(RUN / "saves" / "world.json")
    story = load(RUN / "saves" / "story.json")
    main_scene = load(RUN / "saves" / "konosuba.json")
    live_fork = load(RUN / "saves" / "konosuba_f180_0.json")
    perceptions, ops_after = parse_console(RUN / "console.log")
    closures = closure_map(story)

    mono_by_turn: dict[int, list[tuple[str, str]]] = defaultdict(list)
    mono_counts = {}
    perc_log_n = sum(len(v) for v in perceptions.values())
    for name, mem in world["character_memories"].items():
        rows = mem.get("monologue") or []
        mono_counts[name] = len(rows)
        for row in rows:
            text = (row.get("text") or "").strip()
            if text:
                mono_by_turn[int(row.get("turn") or 0)].append((name, text))

    main_beats = beats_from_scene(main_scene)
    fork_beats = beats_from_scene(live_fork)

    used_closures: set[str] = set()
    out: list[str] = []
    out.append("---")
    out.append("title: Player-tools 149 — reading edition with minds")
    out.append("date: 2026-08-29")
    out.append("tags: [session-pipeline, story, monologue, perception]")
    out.append("---")
    out.append("")
    out.append("# The cemetery, again")
    out.append("")
    out.append(
        "A reading edition of `experiments/session_pipeline/runs/player-tools-300turn`. "
        "The public line is the saved transcript. Private sight and thought are "
        "whatever the run actually retained — every perception apply logged in "
        f"`console.log` ({perc_log_n} lines), and every monologue line still in "
        f"`world.json` (Aqua {mono_counts.get('Aqua', 0)}, Darkness "
        f"{mono_counts.get('Darkness', 0)}, Megumin {mono_counts.get('Megumin', 0)}, "
        f"Luna {mono_counts.get('Luna', 0)}). Perception is an overwrite: each "
        "*sees* block is the new string as of that scene turn, not a history of "
        "all prior sights. Monologue turns are the scene `turn_index` of the "
        "storyline that character was on; a fork can reuse a number. Those "
        "lines are printed after the main-scene beat with the same number, and "
        "are marked if more than one line shares a name and turn."
    )
    out.append("")
    out.append(
        "The run resumed a 3-turn guild save, then played 149 more eval turns "
        "under the fork_merge guide. It stopped on DeepSeek 402 after beat 151, "
        "with Aqua forked once more (`konosuba_f180_0`). No missing private "
        "prose was written in. Empty narrator beats are omitted. The engine "
        "roster name for the player is `Player`; here he is **Kazuma**, as the "
        "cast addresses him."
    )
    out.append("")
    out.append(
        "Companion analysis: "
        "[player-tools-149-narrative-analysis-2026-08-29.md]"
        "(player-tools-149-narrative-analysis-2026-08-29.md)."
    )
    out.append("")

    current_chapter = None
    turns = sorted(t for t in main_beats if t >= 0)

    if -1 in main_beats:
        out.append("## Prologue")
        out.append("")
        emit_public(out, main_beats[-1])
        out.append("")

    for turn in turns:
        title = chapter_for(turn)
        if title and title != current_chapter:
            current_chapter = title
            out.append(f"## {title}")
            out.append("")

        out.append(f"### {turn + 1}")
        out.append("")
        emit_public(out, main_beats[turn])
        out.append("")

        scene_perc = perceptions.get(("konosuba", turn), [])
        monos = list(mono_by_turn.get(turn, []))
        emit_minds(out, scene_perc, monos)
        if scene_perc or monos:
            out.append("")

        for op in ops_after.get(turn, []):
            if op[0] == "fork":
                _, parent, child = op
                if parent != "konosuba":
                    continue
                c = closures.get(child)
                intent = (c or {}).get("driving_intention") or ""
                cast = ", ".join((c or {}).get("cast") or [])
                out.append(
                    f"*A storyline leaves the hall — `{child}`"
                    + (f", {cast}" if cast else "")
                    + (f". They mean to: {intent}" if intent else "")
                    + ".*"
                )
                out.append("")
            elif op[0] == "merge":
                _, src, dest = op
                c = closures.get(src)
                if not c:
                    continue
                used_closures.add(src)
                out.append(f"### Interlude — `{src}` returns")
                out.append("")
                if c.get("driving_intention"):
                    out.append(f"*They had gone to: {c['driving_intention']}*")
                    out.append("")
                body = (c.get("story_so_far") or "") + "\n\n" + (c.get("final_narration") or "")
                rewritten = rewrite_tagged(body)
                if rewritten:
                    out.append(rewritten)
                    out.append("")
            elif op[0] == "conclude":
                _, src = op
                c = closures.get(src)
                if not c:
                    continue
                used_closures.add(src)
                out.append(f"### Closed — `{src}`")
                out.append("")
                if c.get("reason"):
                    out.append(f"*{c['reason']}*")
                    out.append("")
                body = ""
                if c.get("story_so_far"):
                    body += "[Story so far]\n" + c["story_so_far"] + "\n\n"
                if c.get("final_narration"):
                    body += "[Final narration]\n" + c["final_narration"]
                rewritten = rewrite_tagged(body)
                if rewritten:
                    out.append(rewritten)
                    out.append("")

    leftover = [cid for cid in closures if cid not in used_closures]
    if leftover:
        out.append("## Closures not pinned to a logged beat")
        out.append("")
        for cid in leftover:
            c = closures[cid]
            out.append(f"### `{cid}`")
            out.append("")
            if c.get("driving_intention"):
                out.append(f"*Intention:* {c['driving_intention']}")
                out.append("")
            if c.get("reason"):
                out.append(f"*{c['reason']}*")
                out.append("")
            body = (c.get("story_so_far") or "") + "\n\n" + (c.get("final_narration") or "")
            rewritten = rewrite_tagged(body)
            if rewritten:
                out.append(rewritten)
                out.append("")

    out.append("## Coda — Aqua, still walking")
    out.append("")
    out.append(
        "The run died as lifecycle opened `konosuba_f180_0`. What follows is "
        "the live fork save, not a reconstruction. Private lines for this "
        "thread share turn numbers with the main scene and were already "
        "printed above when the numbers collided."
    )
    out.append("")
    if live_fork.get("driving_intention"):
        out.append(f"*They mean to: {live_fork['driving_intention']}*")
        out.append("")
    for t in sorted(fork_beats):
        if t < 0:
            emit_public(out, fork_beats[t])
            out.append("")
            continue
        out.append(f"### Fork beat {t}")
        out.append("")
        emit_public(out, fork_beats[t])
        out.append("")
        emit_minds(out, perceptions.get(("konosuba_f180_0", t), []), [])
        if perceptions.get(("konosuba_f180_0", t)):
            out.append("")

    OUT.write_text("\n".join(out).rstrip() + "\n", encoding="utf-8")
    print("wrote", OUT, "chars", OUT.stat().st_size)


if __name__ == "__main__":
    main()
