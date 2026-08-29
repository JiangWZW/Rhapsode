"""One-off forensic dump for player-tools-300turn (149 turns)."""
from __future__ import annotations

import json
import re
from collections import Counter, defaultdict
from pathlib import Path

RUN = Path(r"D:\cursor-workspace\Rhapsode\experiments\session_pipeline\runs\player-tools-300turn")

META_RE = re.compile(
    r"(fork|merge|off-stage|on-stage|storyline|mechanism test|my brief|"
    r"list_scenes|query_graph|query_mind|### Situation|player_present|"
    r"konosuba_f|inactive thread|the brief)",
    re.I,
)
FIRST_PERSON_RE = re.compile(r"\b(I|I'll|I'm|I've|I'd|me|my|myself)\b")
PLANNING_RE = re.compile(
    r"(I need to|Let me re-read|Per my brief|The brief|mechanism|"
    r"That's my fork|Change of plans|the other thread|"
    r"no off-stage|everyone who matters)",
    re.I,
)


def load_jsonl(path: Path):
    rows = []
    with path.open(encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    return rows


def main() -> None:
    turns = load_jsonl(RUN / "turns.jsonl")
    hops = load_jsonl(RUN / "llm_profile.jsonl")
    console = (RUN / "console.log").read_text(encoding="utf-8", errors="replace")
    story = (RUN / "story.txt").read_text(encoding="utf-8", errors="replace")
    report = json.loads((RUN / "report.json").read_text(encoding="utf-8"))
    manifest = json.loads((RUN / "manifest.json").read_text(encoding="utf-8"))

    player_hops = [h for h in hops if h.get("stage") == "player"]
    other_hops = [h for h in hops if h.get("stage") != "player"]

    # --- player tools ---
    by_turn_tools = []
    # Group player hops by gaps: each player() call is a cluster of tools_round
    clusters = []
    cur = []
    for h in player_hops:
        if h.get("kind") != "tools_round":
            continue
        if h.get("tool_round") == 1 and cur:
            clusters.append(cur)
            cur = [h]
        else:
            cur.append(h)
    if cur:
        clusters.append(cur)

    tool_counter = Counter()
    turns_with_tools = 0
    rounds_per_cluster = []
    for cl in clusters:
        names = []
        for h in cl:
            tools = (h.get("tools") or "").split(",")
            names.extend(t for t in tools if t)
        if names:
            turns_with_tools += 1
        for n in names:
            tool_counter[n] += 1
        rounds_per_cluster.append(len(cl))

    # --- player actions ---
    actions = []
    empty_narrator = []
    for t in turns:
        text = t.get("input") or ""
        n_lines = len([ln for ln in text.splitlines() if ln.strip()])
        words = len(text.split())
        actions.append({
            "turn": t["turn"],
            "chars": len(text),
            "words": words,
            "lines": n_lines,
            "meta": bool(META_RE.search(text)),
            "planning": bool(PLANNING_RE.search(text)),
            "first_person": bool(FIRST_PERSON_RE.search(text)),
            "starts_quote": text.lstrip().startswith('"'),
            "preview": text.replace("\n", " ")[:160],
        })
        msgs = t.get("messages") or []
        narr = [m for m in msgs if m.get("scene_kind") == "narrator"]
        if narr and not (narr[0].get("content") or "").strip():
            empty_narrator.append(t["turn"])

    # --- latency ---
    ready = [t["ready_ms"] for t in turns if t.get("ready_ms") is not None]
    idle = [t["idle_ms"] for t in turns if t.get("idle_ms") is not None]

    def pct(xs, p):
        if not xs:
            return None
        s = sorted(xs)
        i = min(len(s) - 1, max(0, int(round((p / 100) * (len(s) - 1)))))
        return s[i]

    # --- console events ---
    forks = re.findall(r"story: fork from=(\S+) to=(\S+) cast=\[(.*?)\] intention=(.*)", console)
    merges = re.findall(r"story: merge .*", console)
    concludes = re.findall(r"story: conclude .*", console)
    fork_lines = [ln for ln in console.splitlines() if "story: fork" in ln]
    merge_lines = [ln for ln in console.splitlines() if "story: merge" in ln or " merge from=" in ln]
    conclude_lines = [ln for ln in console.splitlines() if "story: conclude" in ln or "conclude_scene" in ln]
    errors = [ln for ln in console.splitlines() if "WARN" in ln or "ERROR" in ln or "Traceback" in ln or "Insufficient" in ln]
    turn_ends = re.findall(r"turn: end scene=(\S+) id=(\d+)", console)

    # --- story structure ---
    scene_headers = re.findall(r"^## .+$", story, re.M)
    live = re.search(r"Live storylines: (\d+).*Concluded: (\d+).*Merged forks: (\d+)", story)

    # --- saves ---
    story_save = json.loads((RUN / "saves" / "story.json").read_text(encoding="utf-8"))
    world = json.loads((RUN / "saves" / "world.json").read_text(encoding="utf-8"))
    scenes = story_save.get("scenes") or story_save.get("storylines") or []
    if isinstance(story_save, dict) and "scene_ids" in story_save:
        scene_ids = story_save.get("scene_ids")
    else:
        scene_ids = list(story_save.keys()) if isinstance(story_save, dict) else []

    graph = world.get("world_graph") or world.get("graph") or {}
    nodes = graph.get("nodes") or []
    if isinstance(nodes, dict):
        node_n = len(nodes)
    else:
        node_n = len(nodes)

    chars = world.get("characters") or []

    # hop stages
    stage_counts = Counter(h.get("stage", "?") for h in hops)
    stage_ms = defaultdict(int)
    for h in hops:
        stage_ms[h.get("stage", "?")] += int(h.get("wall_ms") or 0)

    player_tool_turns = []
    for i, cl in enumerate(clusters, 1):
        names = []
        for h in cl:
            tools = (h.get("tools") or "").split(",")
            names.extend(t for t in tools if t)
        player_tool_turns.append({
            "cluster": i,
            "rounds": len(cl),
            "tools": names,
            "used": bool(names),
            "final_chars": cl[-1].get("content_len"),
        })

    # buckets of 10 for tool use and action chars
    buckets = []
    for start in range(0, len(actions), 10):
        chunk = actions[start:start + 10]
        cl_chunk = clusters[start:start + 10] if start < len(clusters) else []
        used = sum(1 for cl in cl_chunk if any(
            (h.get("tools") or "") for h in cl
        ))
        buckets.append({
            "range": f"{chunk[0]['turn']}-{chunk[-1]['turn']}",
            "n": len(chunk),
            "tool_turns": used,
            "mean_chars": int(sum(a["chars"] for a in chunk) / len(chunk)),
            "meta": sum(1 for a in chunk if a["meta"]),
            "planning": sum(1 for a in chunk if a["planning"]),
            "mean_ready_s": round(sum(turns[a["turn"] - 1]["ready_ms"] for a in chunk) / len(chunk) / 1000, 1),
            "mean_idle_s": round(sum(turns[a["turn"] - 1]["idle_ms"] for a in chunk) / len(chunk) / 1000, 1),
        })

    out = {
        "n_turns": len(turns),
        "manifest": manifest,
        "narrative": report.get("narrative"),
        "player_clusters": len(clusters),
        "player_turns_with_tools": turns_with_tools,
        "player_tool_rate": round(turns_with_tools / max(len(clusters), 1), 3),
        "player_tool_counts": dict(tool_counter),
        "player_rounds_mean": round(sum(rounds_per_cluster) / max(len(rounds_per_cluster), 1), 2),
        "player_rounds_max": max(rounds_per_cluster) if rounds_per_cluster else 0,
        "action_mean_chars": int(sum(a["chars"] for a in actions) / len(actions)),
        "action_median_chars": sorted(a["chars"] for a in actions)[len(actions) // 2],
        "action_mean_lines": round(sum(a["lines"] for a in actions) / len(actions), 2),
        "action_one_line": sum(1 for a in actions if a["lines"] == 1),
        "action_meta": sum(1 for a in actions if a["meta"]),
        "action_planning": sum(1 for a in actions if a["planning"]),
        "action_first_person": sum(1 for a in actions if a["first_person"]),
        "empty_narrator_turns": empty_narrator,
        "ready_mean_s": round(sum(ready) / len(ready) / 1000, 1),
        "ready_p50_s": round(pct(ready, 50) / 1000, 1),
        "ready_p95_s": round(pct(ready, 95) / 1000, 1),
        "idle_mean_s": round(sum(idle) / len(idle) / 1000, 1),
        "idle_p50_s": round(pct(idle, 50) / 1000, 1),
        "idle_p95_s": round(pct(idle, 95) / 1000, 1),
        "elapsed_h": round(38585179 / 3600000, 2),
        "forks_parsed": forks,
        "fork_n": len(fork_lines),
        "merge_n": len(merge_lines),
        "conclude_n": len(conclude_lines),
        "fork_lines": fork_lines[:40],
        "merge_lines": merge_lines[:20],
        "conclude_lines": conclude_lines[:20],
        "warn_sample": errors[:30],
        "turn_end_last": turn_ends[-5:] if turn_ends else [],
        "scene_headers": scene_headers,
        "live_line": live.groups() if live else None,
        "story_save_keys": list(story_save.keys())[:40] if isinstance(story_save, dict) else type(story_save).__name__,
        "scene_ids": scene_ids,
        "n_chars": len(chars) if isinstance(chars, list) else None,
        "n_nodes": node_n,
        "stage_counts": dict(stage_counts),
        "stage_ms": dict(stage_ms),
        "buckets": buckets,
        "meta_turns": [a["turn"] for a in actions if a["meta"]],
        "planning_turns": [a["turn"] for a in actions if a["planning"]],
        "longest_actions": sorted(actions, key=lambda a: -a["chars"])[:8],
        "first_8": actions[:8],
        "last_8": actions[-8:],
    }
    dest = Path(r"D:\cursor-workspace\Rhapsode\tmp\player_tools_149_stats.json")
    dest.write_text(json.dumps(out, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps({k: out[k] for k in [
        "n_turns", "player_clusters", "player_turns_with_tools", "player_tool_rate",
        "player_tool_counts", "action_mean_chars", "action_median_chars",
        "action_mean_lines", "action_one_line", "action_meta", "action_planning",
        "action_first_person", "empty_narrator_turns", "ready_mean_s", "idle_mean_s",
        "fork_n", "merge_n", "conclude_n", "live_line", "n_nodes",
        "scene_headers",
    ]}, indent=2))
    print("wrote", dest)


if __name__ == "__main__":
    main()
