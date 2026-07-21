import gc
import json
import weakref

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
    story.set_scheduler_callback(make_scheduler_callback(story))
    story.set_narrator_llm_callback(make_narrator_callback(story))
    story_ref = weakref.ref(story)
    del story
    gc.collect()
    assert story_ref() is None


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
    story.fork_scene("root", "ridge", [], "Watch the ridge")
    annotator = Annotator(story.world())
    assert story.conclude_scene("root", "The root storyline closes")
    spans = annotator.annotate("Scout waits.")
    assert [(span.text, span.category) for span in spans] == [("Scout", "character")]
