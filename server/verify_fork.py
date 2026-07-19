r"""Headless, network-free verification of the parallel-scene lifecycle.

Runs the REAL engine path end to end -- Story.advance_scene -> dispatch_tool ->
stage -> apply_pending_ops -> scheduler -> off-stage beat -- with the LLM calls
replaced by a scripted stub. No Gemini/DeepSeek, no llama.cpp, no ChromaDB.

The only seam we replace is the module-level `complete_with_tools`, the single
function both the narrator and scheduler adapters call to drive their tool-use
loops. Everything below it (the adapters, the C++ Story/SceneLoop/Director, the
staging/apply transaction, the scheduler read tools) is the production code.

Run:  server\.venv\Scripts\python.exe verify_fork.py
Exits non-zero if any check fails.
"""

import json
import os
import sys

import rhapsode.llm_tools as llm_tools
import rhapsode.scheduler as scheduler_mod
from rhapsode._core import Director, Scene, SceneLoop, Story

SCENARIO = os.path.join(os.path.dirname(__file__), "scenarios", "siege.json")

INTENT_A = "slip through the drainage tunnels and flank the siege line"
PROSE = "The moment turns; the storyline holds its breath and moves on."


# -- Scripted, network-free LLM ------------------------------------------------

class Script:
    """Mutable state that drives the stubbed narrator/scheduler across a run."""
    def __init__(self):
        self.story = None
        self.root_id = ""
        self.a_offstage_beats = 0
        self.want_merge = False
        self.staging_ok = None      # set by the fork beat: was the op staged, not applied?
        self.staging_detail = ""
        # Latches: a player's input lingers in the turn transcript, so a sentinel
        # would otherwise re-fire on every later beat. Each trigger fires once.
        self.forked = False
        self.concluded = False

    def reset_triggers(self):
        self.a_offstage_beats = 0
        self.want_merge = False
        self.staging_ok = None
        self.forked = False
        self.concluded = False


SCRIPT = Script()


def _user_text(messages):
    """The last user turn's text (the narrator's turn_state / the cue)."""
    for msg in reversed(messages):
        if msg.get("role") == "user":
            return "".join(p.get("text", "") for p in msg.get("parts", []))
    return ""


def _other_scene():
    """The single non-root live scene id, or None."""
    return next((i for i in SCRIPT.story.scene_ids() if i != SCRIPT.root_id), None)


def fake_narrator_ctw(messages, tools, dispatch):
    """Stubbed narrator tool-use loop.

    Decides lifecycle actions from the beat's turn_state: player beats carry a
    sentinel token in the player's input; off-stage beats carry the scene's
    driving intention in the autonomous cue. Every decision goes through the REAL
    dispatcher (-> Story.dispatch_tool -> World.stage_*), exactly as a live model
    would express it.
    """
    ts = _user_text(messages)
    off_stage = "[Off-stage beat:" in ts

    if not off_stage:
        if "FORK_A" in ts and not SCRIPT.forked:
            SCRIPT.forked = True
            before = SCRIPT.story.scene_count()
            dispatch("fork_scene",
                     {"driving_intention": INTENT_A, "cast": ["Sergeant Maren"]})
            after = SCRIPT.story.scene_count()
            SCRIPT.staging_ok = (after == before)
            SCRIPT.staging_detail = f"scene_count during beat: before={before} after={after}"
        elif "CONCLUDE_A" in ts and not SCRIPT.concluded:
            SCRIPT.concluded = True
            child = _other_scene()
            if child:
                dispatch("conclude_scene",
                         {"scene_id": child, "reason": "the flanking route paid off"})
    else:
        if INTENT_A in ts:
            SCRIPT.a_offstage_beats += 1
            if SCRIPT.want_merge and SCRIPT.a_offstage_beats >= 2:
                dispatch("merge_scene", {"into_scene_id": SCRIPT.root_id})
                SCRIPT.want_merge = False

    return PROSE


def fake_scheduler_ctw(messages, tools, dispatch):
    """Stubbed scheduler tool-use loop: read the live scenes via the REAL
    list_scenes tool, then advance the first off-stage storyline."""
    rows = json.loads(dispatch("list_scenes", {}))
    pick = ""
    for row in rows:
        if not row.get("player_present"):
            pick = row["scene_id"]
            break
    dispatch("advance_scene", {"scene_id": pick})
    return ""


# -- Engine wiring (offline) ---------------------------------------------------

def build_engine():
    """A Story bound to a real SceneLoop, wired with offline stubs.

    No weaver, no saves dir, no memory -> the background weave/persist/ChromaDB
    paths are all skipped, leaving a deterministic, network-free engine.
    """
    story = Story.from_scene(Scene.load_json(SCENARIO))
    root = story.active_scene()

    director = Director(root.world_graph)

    loop = SceneLoop()
    loop.set_director(director)
    loop.set_llm_callback(lambda _prompt: "")            # narrator fallback (unused)
    loop.set_narrator_llm_callback(llm_tools.make_narrator_callback(story))

    story.bind_runtime(loop)
    story.set_scheduler_callback(scheduler_mod.make_scheduler_callback(story))
    story.set_downsampler_callback(lambda s: s)          # identity

    SCRIPT.story = story
    SCRIPT.root_id = root.scene_id
    return story, loop, director


# -- Check harness -------------------------------------------------------------

class Checks:
    def __init__(self):
        self.results = []

    def __call__(self, label, ok, detail=""):
        self.results.append((label, bool(ok), detail))
        mark = "PASS" if ok else "FAIL"
        line = f"  [{mark}] {label}"
        if detail:
            line += f"  ({detail})"
        print(line)

    def failed(self):
        return [r for r in self.results if not r[1]]


def phase_a(check, story, loop):
    """Fork during a player beat, then verify the scheduler advances the child
    off-stage on the same and following turns, then conclude it."""
    print("\n=== Phase A: fork -> scheduled off-stage advance -> conclude ===")

    story.advance_scene("look around the war room")
    loop.join_background()
    check("T1: single storyline before any fork", story.scene_count() == 1,
          f"scene_count={story.scene_count()}")

    clock_before = story.beat_clock
    story.advance_scene("FORK_A: send Sergeant Maren to scout another way in")
    loop.join_background()
    clock_after = story.beat_clock

    child = _other_scene()
    check("T2: fork staged during the beat, applied only after",
          SCRIPT.staging_ok is True, SCRIPT.staging_detail)
    check("T2: fork produced a second storyline",
          story.scene_count() == 2 and child is not None,
          f"scenes={story.scene_ids()}")

    maren = story.world().find_character("Sergeant Maren")
    check("T2: fork moved the named cast onto the child",
          maren is not None and child is not None and maren.in_scene(child),
          f"Maren.scene_ids={list(maren.scene_ids) if maren else None}")

    # The scheduler's read tool sees the child as an off-stage, charged storyline.
    rows = json.loads(story.dispatch_tool("", "list_scenes", "{}"))
    child_row = next((r for r in rows if r["scene_id"] == child), None)
    check("T2: list_scenes exposes the child as off-stage with charge",
          child_row is not None and not child_row["player_present"]
          and child_row["charge"] > 0.0,
          f"row={child_row}")

    check("T2: scheduler advanced the child off-stage this turn",
          child is not None and story.get_scene(child).turn_index >= 1,
          f"child.turn_index={story.get_scene(child).turn_index if child else None}")
    check("T2: two beats ran (player + off-stage) -> beat_clock +2",
          clock_after - clock_before == 2,
          f"beat_clock {clock_before} -> {clock_after}")

    t_before = story.get_scene(child).turn_index
    story.advance_scene("press the assault on the gate")
    loop.join_background()
    t_after = story.get_scene(child).turn_index
    check("T3: scheduler advanced the child off-stage again",
          t_after > t_before, f"child.turn_index {t_before} -> {t_after}")

    story.advance_scene("CONCLUDE_A: recall Maren, the route is mapped")
    loop.join_background()
    check("T4: conclude retired the child storyline",
          story.scene_count() == 1 and child not in story.scene_ids(),
          f"scenes={story.scene_ids()}")


def phase_b(check):
    """Fork, let the child advance once, then have the child's own off-stage beat
    merge it back -- exercising merge through the same staged-op transaction."""
    print("\n=== Phase B: fork -> off-stage beat merges the child back ===")
    SCRIPT.reset_triggers()
    SCRIPT.want_merge = True

    story, loop, _director = build_engine()

    story.advance_scene("FORK_A: send Maren ahead through the tunnels")
    loop.join_background()
    child = _other_scene()
    check("T1: fork created the child storyline",
          story.scene_count() == 2 and child is not None,
          f"scenes={story.scene_ids()}")

    story.advance_scene("hold the line and wait for the signal")
    loop.join_background()
    check("T2: child's off-stage beat merged it back (source retired)",
          story.scene_count() == 1 and child not in story.scene_ids(),
          f"scenes={story.scene_ids()}")

    maren = story.world().find_character("Sergeant Maren")
    check("T2: merge moved cast back to the surviving scene",
          maren is not None and child is not None
          and maren.in_scene(SCRIPT.root_id) and not maren.in_scene(child),
          f"Maren.scene_ids={list(maren.scene_ids) if maren else None}")


def main():
    # Replace the single LLM seam in both adapter modules. Everything else is real.
    llm_tools.complete_with_tools = fake_narrator_ctw
    scheduler_mod.complete_with_tools = fake_scheduler_ctw

    check = Checks()

    story, loop, _director = build_engine()
    try:
        phase_a(check, story, loop)
    finally:
        loop.join_background()

    phase_b(check)

    failures = check.failed()
    total = len(check.results)
    print(f"\n{'-' * 60}")
    if failures:
        print(f"FAILED: {len(failures)}/{total} checks failed")
        for label, _ok, detail in failures:
            print(f"  - {label}  ({detail})")
        sys.exit(1)
    print(f"OK: all {total} checks passed")


if __name__ == "__main__":
    main()
