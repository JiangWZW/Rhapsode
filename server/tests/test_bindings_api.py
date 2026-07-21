import pytest

from rhapsode import _core


def test_public_binding_surface_matches_owned_runtime_design():
    expected = {
        "Annotator", "Character", "CharacterMemory", "DeathCandidate",
        "Director", "DirectorOutput", "EdgeData", "EdgeInfo", "EntitySpan",
        "ExpiryOp", "GraphAnalysis", "History", "MemorySystem", "Node",
        "NodeState", "Rejection", "Role", "SceneData", "SceneMessage",
        "Snippet", "Story", "TextDownsampler", "WeaveOp", "WeaveResult",
        "Weaver", "World", "WorldGraph", "analyze_graph",
    }
    assert {name for name in dir(_core) if not name.startswith("_")} == expected
    assert not hasattr(_core, "Scene")
    assert not hasattr(_core, "SceneLoop")


def test_story_is_the_production_composition_surface():
    expected = {
        "active_scene", "active_scene_id", "advance_scene", "beat_clock",
        "conclude_scene", "delete_save", "dispatch_tool", "display_timeline",
        "fork_scene", "from_scenario_json_str", "get_scene", "has_save",
        "load_save", "load_scenario", "merge_scene", "note_advanced",
        "revert_active_turns", "save", "scene_count", "scene_ids",
        "set_downsampler_callback", "set_history_window",
        "set_lifecycle_callback", "set_llm_callback",
        "set_memory", "set_reflection_llm_callback",
        "set_narrator_llm_callback", "set_resuming", "set_saves_dir",
        "set_scheduler_callback", "set_weaver_interval",
        "set_weaver_llm_callback", "set_weaver_local_llm_callback",
        "to_scenario_json_str", "tool_list_scenes", "world",
    }
    assert {name for name in dir(_core.Story) if not name.startswith("_")} == expected
    assert "bind_runtime" not in expected


def test_scene_data_has_only_per_storyline_state():
    expected = {
        "charge", "dialogue", "driving_intention", "history",
        "last_advanced", "scene_id", "system_prompt", "title", "turn_index",
    }
    assert {name for name in dir(_core.SceneData) if not name.startswith("_")} == expected
    for forbidden in (
        "world", "world_graph", "characters", "character_memories",
        "enter_character", "fork", "save", "load_save", "revert_turns",
    ):
        assert not hasattr(_core.SceneData, forbidden)


def test_world_exposes_read_state_but_not_lifecycle_staging():
    expected = {
        "character_memories", "characters", "find_character",
        "scan_death_candidates",
        "tool_query_graph", "tool_query_mind", "world_graph",
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


def test_python_cannot_mutate_membership_or_death_directly():
    character = _core.Character("Scout", "Careful", False)
    assert not hasattr(character, "join_scene")
    assert not hasattr(character, "leave_scene")
    with pytest.raises(AttributeError):
        character.scene_ids = ["root"]
    with pytest.raises(AttributeError):
        character.dead = True
