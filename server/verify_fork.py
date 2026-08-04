r"""Network-free verification of Story-owned parallel-scene lifecycle.

Run: server\.venv\Scripts\python.exe server\verify_fork.py
"""

import json
import os
import sys

from rhapsode._core import Story


SCENARIO = os.path.join(os.path.dirname(__file__), "scenarios", "siege.json")
INTENTION = "slip through the drainage tunnels and flank the siege line"


class Script:
    def __init__(self, story: Story):
        self.story = story
        self.root_id = story.active_scene_id
        self.mode = "idle"

    def narrator(self, _scene_id: str, instructions: str, _turn_state: str,
                 _read_tool) -> str:
        if "fork_story_so_far" in instructions:
            return json.dumps({
                "fork_story_so_far": (
                    "Maren leaves the siege line for the drainage tunnels, "
                    "carrying the unresolved flanking plan."
                ),
            })
        if "merged_story_so_far" in instructions:
            return json.dumps({
                "merged_story_so_far": (
                    "Maren returns from the tunnels to the siege line with "
                    "the flanking thread reconciled into the main defense."
                ),
            })
        return (
            "The moment turns and the storyline moves on.\n"
            "<<<RHAPSODE_JSON>>>\n"
            '{"transitions":[],"new_nodes":[],"speech_turns":[],'
            '"new_characters":[],"active_cast":[]}'
        )

    def scheduler(self, _instructions: str, _user: str, read_tool) -> str:
        rows = json.loads(read_tool("list_scenes", "{}"))
        return next(
            (row["scene_id"] for row in rows if not row["player_present"]),
            "",
        )

    def lifecycle(self, _instructions: str, user: str, _read_tool) -> str:
        context = json.loads(user[user.find("{"):])
        advanced = context.get("advanced_scene_id") or context.get("scene_id")
        ops: list[dict] = []
        if self.mode == "fork" and advanced == self.root_id:
            ops.append({
                "op": "fork",
                "parent": self.root_id,
                "cast": ["Sergeant Maren"],
                "driving_intention": INTENTION,
            })
            self.mode = "idle"
        elif self.mode == "conclude" and advanced and advanced != self.root_id:
            ops.append({
                "op": "conclude",
                "scene_id": advanced,
                "reason": "the flanking route paid off",
            })
            self.mode = "idle"
        elif self.mode == "merge" and advanced and advanced != self.root_id:
            ops.append({
                "op": "merge",
                "from": advanced,
                "into": self.root_id,
                "reason": "co-presence with the player",
            })
            self.mode = "idle"
        return json.dumps({"ops": ops})


def build_engine() -> tuple[Story, Script]:
    story = Story.load_scenario(SCENARIO)
    script = Script(story)
    story.set_llm_callback(lambda _prompt: "")
    story.set_narrator_llm_callback(script.narrator)
    story.set_scheduler_callback(script.scheduler)
    story.set_lifecycle_callback(script.lifecycle)
    return story, script


def check(label: str, condition: bool) -> None:
    print(f"  [{'PASS' if condition else 'FAIL'}] {label}")
    if not condition:
        raise AssertionError(label)


def verify_conclude() -> None:
    story, script = build_engine()
    script.mode = "fork"
    story.advance_player("Send Maren through the tunnels.")
    story.complete_turn()
    children = [scene_id for scene_id in story.scene_ids()
                if scene_id != script.root_id]
    check("fork creates an off-stage storyline", len(children) == 1)
    child = children[0]
    maren = story.world().find_character("Sergeant Maren")
    check("fork moves its cast", maren is not None and maren.in_scene(child))

    script.mode = "conclude"
    story.advance_player("Hold the line.")
    story.complete_turn()
    check("off-stage lifecycle can conclude itself", story.scene_count() == 1)


def verify_merge() -> None:
    story, script = build_engine()
    script.mode = "fork"
    story.advance_player("Send Maren through the tunnels.")
    story.complete_turn()
    child = next(scene_id for scene_id in story.scene_ids()
                 if scene_id != script.root_id)
    script.mode = "merge"
    story.advance_player("Wait for Maren's signal.")
    story.complete_turn()
    maren = story.world().find_character("Sergeant Maren")
    check("merge retires the source", child not in story.scene_ids())
    check("merge returns cast to the survivor",
          maren is not None and maren.in_scene(script.root_id))


def main() -> None:
    try:
        verify_conclude()
        verify_merge()
    except AssertionError:
        sys.exit(1)
    print("OK: Story-owned lifecycle verification passed")


if __name__ == "__main__":
    main()
