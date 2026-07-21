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

    def narrator(self, _scene_id: str, _instructions: str, _turn_state: str) -> str:
        return (
            "The moment turns and the storyline moves on.\n"
            "<<<RHAPSODE_JSON>>>\n"
            '{"transitions":[],"new_nodes":[],"speech_turns":[],'
            '"new_characters":[],"active_cast":[]}'
        )

    def scheduler(self, _instructions: str, _user: str) -> str:
        rows = json.loads(self.story.tool_list_scenes())
        return next(
            (row["scene_id"] for row in rows if not row["player_present"]),
            "",
        )

    def lifecycle(self, _instructions: str, user: str) -> str:
        context = json.loads(user[user.find("{"):])
        scene_id = context["scene_id"]
        verdict = {
            "fork": None,
            "merge_into": None,
            "conclude": None,
            "exited": [],
        }
        if self.mode == "fork" and scene_id == self.root_id:
            verdict["fork"] = {
                "cast": ["Sergeant Maren"],
                "driving_intention": INTENTION,
            }
            self.mode = "idle"
        elif self.mode == "conclude" and scene_id != self.root_id:
            verdict["conclude"] = "the flanking route paid off"
            self.mode = "idle"
        elif self.mode == "merge" and scene_id != self.root_id:
            verdict["merge_into"] = self.root_id
            self.mode = "idle"
        return json.dumps(verdict)


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
    story.advance_scene("Send Maren through the tunnels.")
    children = [scene_id for scene_id in story.scene_ids()
                if scene_id != script.root_id]
    check("fork creates an off-stage storyline", len(children) == 1)
    child = children[0]
    maren = story.world().find_character("Sergeant Maren")
    check("fork moves its cast", maren is not None and maren.in_scene(child))

    script.mode = "conclude"
    story.advance_scene("Hold the line.")
    check("off-stage lifecycle can conclude itself", story.scene_count() == 1)


def verify_merge() -> None:
    story, script = build_engine()
    script.mode = "fork"
    story.advance_scene("Send Maren through the tunnels.")
    child = next(scene_id for scene_id in story.scene_ids()
                 if scene_id != script.root_id)
    script.mode = "merge"
    story.advance_scene("Wait for Maren's signal.")
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
