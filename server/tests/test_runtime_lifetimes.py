import gc
import json
import weakref

import pytest

from rhapsode._core import (
    Annotator,
    Director,
    MemorySystem,
    Story,
    Weaver,
    WorldGraph,
)
from rhapsode.llm_tools import make_narrator_callback
from rhapsode.scheduler import make_scheduler_callback


def _story() -> Story:
    return Story.from_scenario_json_str(
        json.dumps({
            "title": "Root",
            "system_prompt": "Narrate.",
            "characters": [
                {
                    "name": "Player", "description": "The player",
                    "is_player": True, "on_stage": True,
                },
                {"name": "Scout", "description": "Careful", "on_stage": True},
            ],
        }),
        "root",
    )


def test_standalone_graph_utilities_keep_their_graph_alive():
    director_graph = WorldGraph()
    director_graph_ref = weakref.ref(director_graph)
    director = Director(director_graph)
    del director_graph
    gc.collect()
    assert director_graph_ref() is not None
    assert director.apply_planned_turn(0, "{}").new_nodes == []

    weaver_graph = WorldGraph()
    weaver_graph_ref = weakref.ref(weaver_graph)
    weaver = Weaver(weaver_graph)
    del weaver_graph
    gc.collect()
    assert weaver_graph_ref() is not None
    assert weaver.should_weave(0)


def test_story_callbacks_do_not_form_strong_reference_cycles():
    story = _story()
    story.set_scheduler_callback(make_scheduler_callback())
    story.set_narrator_llm_callback(make_narrator_callback())
    story_ref = weakref.ref(story)
    del story
    gc.collect()
    assert story_ref() is None


def test_read_tools_expire_when_the_narrator_call_returns():
    story = _story()
    retained = []

    def narrator(_scene_id, _instructions, _turn_state, read_tool):
        assert json.loads(read_tool("list_scenes", "{}"))[0]["scene_id"] == "root"
        retained.append(read_tool)
        return (
            "The scout waits.\n<<<RHAPSODE_JSON>>>\n"
            '{"transitions":[],"new_nodes":[],"speech_turns":[],'
            '"new_characters":[],"active_cast":["Scout"]}'
        )

    story.set_llm_callback(lambda _prompt: "")
    story.set_narrator_llm_callback(narrator)
    story.advance_scene("Wait.")

    with pytest.raises(RuntimeError, match="no longer active"):
        retained[0]("list_scenes", "{}")


def test_read_tools_expire_when_the_lifecycle_call_returns():
    story = _story()
    retained = []
    story.set_llm_callback(lambda _prompt: "")
    story.set_narrator_llm_callback(
        lambda _scene_id, _instructions, _turn_state, _read_tool: (
            "The scout waits.\n<<<RHAPSODE_JSON>>>\n"
            '{"transitions":[],"new_nodes":[],"speech_turns":[],'
            '"new_characters":[],"active_cast":["Scout"]}'
        )
    )

    def lifecycle(_instructions, _user, read_tool):
        retained.append(read_tool)
        assert json.loads(read_tool("list_scenes", "{}"))[0]["scene_id"] == "root"
        return json.dumps({
            "ops": [],
        })

    story.set_lifecycle_callback(lifecycle)
    story.advance_scene("Wait.")

    with pytest.raises(RuntimeError, match="no longer active"):
        retained[0]("list_scenes", "{}")


def test_complete_session_graph_is_released_together():
    story = _story()
    memory = MemorySystem("root")
    story.set_memory(memory)
    annotator = Annotator(story.world())
    story_ref = weakref.ref(story)
    memory_ref = weakref.ref(memory)
    annotator_ref = weakref.ref(annotator)

    del annotator, memory, story
    gc.collect()

    assert story_ref() is None
    assert memory_ref() is None
    assert annotator_ref() is None


def test_annotator_survives_scene_retirement_because_world_is_story_owned():
    story = _story()
    story.set_narrator_llm_callback(
        lambda _scene_id, _instructions, _turn_state, _read_tool: json.dumps({
            "fork_story_so_far": "Scout leaves the player to watch the ridge.",
        })
    )
    assert story.fork_scene("root", "ridge", ["Scout"], "Watch the ridge")
    retired_scene = story.get_scene("ridge")
    annotator = Annotator(story.world())
    assert story.conclude_scene("ridge", "The ridge is secure")
    assert retired_scene.scene_id == "ridge"
    spans = annotator.annotate("Scout waits.")
    assert [(span.text, span.category) for span in spans] == [("Scout", "character")]
