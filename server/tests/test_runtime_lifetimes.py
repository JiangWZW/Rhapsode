import gc
import weakref

from rhapsode._core import (
    Annotator,
    Character,
    Director,
    Scene,
    SceneLoop,
    Story,
    Weaver,
    WorldGraph,
)
from rhapsode.llm_tools import make_narrator_callback
from rhapsode.scheduler import make_scheduler_callback


def test_graph_bound_services_keep_their_graph_alive():
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


def test_loop_and_annotator_keep_borrowed_runtime_objects_alive():
    scene = Scene()
    scene.scene_id = "root"
    director = Director(scene.world_graph)
    weaver = Weaver(scene.world_graph)
    scene_ref = weakref.ref(scene)
    director_ref = weakref.ref(director)
    weaver_ref = weakref.ref(weaver)

    loop = SceneLoop()
    loop.load_scene(scene)
    loop.set_director(director)
    loop.set_weaver(weaver)
    del scene, director, weaver
    gc.collect()

    assert scene_ref() is not None
    assert director_ref() is not None
    assert weaver_ref() is not None

    owner_scene = Scene()
    scout = Character("Scout", "A careful scout", False)
    owner_scene.enter_character(scout)
    world = owner_scene.world()
    world_ref = weakref.ref(world)
    annotator = Annotator(world)
    del owner_scene, world
    gc.collect()

    assert world_ref() is not None
    spans = annotator.annotate("Scout waits.")
    assert [(span.text, span.category) for span in spans] == [("Scout", "character")]


def test_story_callbacks_do_not_form_strong_reference_cycles():
    story = Story()
    loop = SceneLoop()
    story.bind_runtime(loop)
    story.set_scheduler_callback(make_scheduler_callback(story))
    loop.set_narrator_llm_callback(make_narrator_callback(story))
    story_ref = weakref.ref(story)
    loop_ref = weakref.ref(loop)

    del story, loop
    gc.collect()

    assert story_ref() is None
    assert loop_ref() is None


def test_annotator_survives_retirement_of_the_scene_used_for_setup():
    root = Scene()
    root.scene_id = "root"
    root.enter_character(Character("Scout", "A careful scout", False))
    story = Story.from_scene(root)
    story.fork_scene("root", "ridge", [], "Watch the ridge")
    annotator = Annotator(story.world())

    assert story.conclude_scene("root", "The root storyline closes")
    assert [(span.text, span.category) for span in annotator.annotate("Scout waits.")] == [
        ("Scout", "character")
    ]
