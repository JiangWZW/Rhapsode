import json

import pytest

from rhapsode import _core


def test_public_binding_surface_matches_owned_runtime_design():
    expected = {
        "Annotator", "Character", "CharacterMemory",
        "EdgeData", "EdgeInfo", "EndReason",
        "EntitySpan", "ExpiryOp", "GraphAnalysis", "MemorySystem",
        "NarrativeMetrics", "Node",
        "NodeState", "ReliabilityMetrics", "Role", "SceneData",
        "SceneMessage", "SessionEvalConfig", "SessionEvalRunner", "SessionReport",
        "Story", "WeaveOp", "WeaveResult",
        "World", "WorldGraph", "analyze_graph",
    }
    assert {name for name in dir(_core) if not name.startswith("_")} == expected
    assert not hasattr(_core, "Scene")
    assert not hasattr(_core, "SceneLoop")
    assert not hasattr(_core, "Director")
    assert not hasattr(_core, "DirectorOutput")
    assert not hasattr(_core, "Rejection")
    assert not hasattr(_core, "Weaver")


def test_story_is_the_production_composition_surface():
    expected = {
        "active_scene", "active_scene_id", "advance_player", "beat_clock",
        "turn_clock",
        "complete_turn",
        "conclude_scene", "delete_save", "dispatch_tool", "display_timeline",
        "fork_scene", "from_scenario_json_str", "get_scene", "has_save",
        "load_save", "load_scenario", "merge_scene", "note_advanced",
        "render_transcript", "revert_active_turns", "save", "scene_count",
        "scene_ids",
        "set_downsampler_callback", "set_history_window",
        "set_lifecycle_callback", "set_llm_callback",
        "set_memory", "set_observation_llm_callback",
        "set_reflection_llm_callback",
        "set_narrator_llm_callback", "set_saves_dir",
        "set_scheduler_callback", "set_weaver_interval",
        "set_weaver_llm_callback",
        "to_scenario_json_str", "tool_list_scenes", "weave_scene", "world",
    }
    assert {name for name in dir(_core.Story) if not name.startswith("_")} == expected
    assert "bind_runtime" not in expected
    assert "set_weaver_local_llm_callback" not in expected


def test_scene_data_has_only_per_storyline_state():
    expected = {
        "charge", "dialogue", "driving_intention", "history",
        "intention_node_id", "intention_owner", "last_advanced",
        "scene_id", "system_prompt", "title", "turn_index",
    }
    assert {name for name in dir(_core.SceneData) if not name.startswith("_")} == expected
    for forbidden in (
        "world", "world_graph", "characters", "character_memories",
        "enter_character", "fork", "save", "load_save", "revert_turns",
    ):
        assert not hasattr(_core.SceneData, forbidden)


@pytest.mark.parametrize(
    ("attribute", "replacement"),
    [
        ("scene_id", "other"),
        ("title", "Other"),
        ("system_prompt", "Other"),
        ("history", []),
        ("dialogue", []),
        ("turn_index", 9),
        ("driving_intention", "Other"),
        ("charge", 2.0),
        ("last_advanced", 9),
        ("intention_owner", "Scout"),
        ("intention_node_id", 3),
    ],
)
def test_scene_data_cannot_be_rewritten_from_python(attribute, replacement):
    story = _core.Story.from_scenario_json_str(
        '{"title":"Root","system_prompt":"Narrate."}', "root"
    )
    with pytest.raises(AttributeError):
        setattr(story.active_scene(), attribute, replacement)


def test_active_scene_rejects_unknown_ids():
    story = _core.Story.from_scenario_json_str(
        '{"title":"Root","system_prompt":"Narrate."}', "root"
    )
    with pytest.raises(ValueError, match="Unknown scene"):
        story.active_scene_id = "missing"
    assert story.active_scene_id == "root"


def test_world_exposes_read_state_but_not_lifecycle_staging():
    expected = {
        "character_memories", "characters", "find_character",
        "state_version", "world_graph",
    }
    assert {name for name in dir(_core.World) if not name.startswith("_")} == expected
    for removed in (
        "stage_fork", "stage_conclude", "stage_merge", "stage_exit",
        "clear_pending_ops", "save", "load_save",
    ):
        assert not hasattr(_core.World, removed)


@pytest.mark.parametrize(
    ("attribute", "replacement"),
    [
        ("world_graph", _core.WorldGraph()),
        ("characters", []),
        ("character_memories", {}),
    ],
)
def test_world_containers_cannot_be_replaced(attribute, replacement):
    world = _core.World()
    assert getattr(world, attribute) is not None
    with pytest.raises(AttributeError):
        setattr(world, attribute, replacement)


def test_story_world_views_are_detached_snapshots():
    story = _core.Story.from_scenario_json_str(
        json.dumps({
            "characters": [{
                "name": "Scout",
                "description": "Careful",
                "on_stage": True,
                "initial_memory": {
                    "beliefs": [{"content": "The gate is shut", "about": ["Gate"]}],
                },
            }],
        }),
        "root",
    )
    world = story.world()

    graph_snapshot = world.world_graph
    node = _core.Node()
    node.fact = "Injected"
    node.entities = ["Gate"]
    graph_snapshot.add_node(node)
    assert graph_snapshot.size() == 1
    assert world.world_graph.size() == 0

    character_snapshot = world.find_character("Scout")
    character_snapshot.name = "Changed"
    character_snapshot.is_player = True
    assert world.find_character("Scout").name == "Scout"
    assert world.find_character("Changed") is None
    assert not world.find_character("Scout").is_player

    memory_snapshot = world.character_memories["Scout"]
    belief_snapshot = memory_snapshot.beliefs
    belief_count = belief_snapshot.size()
    belief_snapshot.add_node(node)
    assert belief_snapshot.size() == belief_count + 1
    assert world.character_memories["Scout"].beliefs.size() == belief_count


def test_story_weave_command_mutates_owned_graph():
    story = _core.Story.from_scenario_json_str(
        json.dumps({
            "nodes": [
                {"fact": "The gate is shut", "entities": ["Gate"]},
                {"fact": "The key is nearby", "entities": ["Key"]},
            ],
        }),
        "root",
    )
    story.set_weaver_llm_callback(
        lambda _prompt: json.dumps({
            "connect": [{
                "from": 1, "to": 2, "weight": 0.7, "reason": "key",
            }],
            "disconnect": [],
            "reweight": [],
        })
    )

    result = story.weave_scene("root")

    assert len(result.connected) == 1
    assert len(story.world().world_graph.all_edges()) == 1


def test_python_cannot_mutate_membership_or_death_directly():
    character = _core.Character("Scout", "Careful", False)
    assert not hasattr(character, "join_scene")
    assert not hasattr(character, "leave_scene")
    with pytest.raises(AttributeError):
        character.scene_ids = ["root"]
    with pytest.raises(AttributeError):
        character.dead = True
