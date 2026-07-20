from rhapsode import _core


def test_public_binding_surface_is_stable():
    expected = {
        "Annotator",
        "Character",
        "CharacterMemory",
        "DeathCandidate",
        "Director",
        "DirectorOutput",
        "EdgeData",
        "EdgeInfo",
        "EntitySpan",
        "ExpiryOp",
        "GraphAnalysis",
        "History",
        "LoopState",
        "MemorySystem",
        "Node",
        "NodeState",
        "Rejection",
        "Role",
        "Scene",
        "SceneLoop",
        "SceneMessage",
        "Snippet",
        "Story",
        "TextDownsampler",
        "WeaveOp",
        "WeaveResult",
        "Weaver",
        "World",
        "WorldGraph",
        "analyze_graph",
    }

    assert {name for name in dir(_core) if not name.startswith("_")} == expected
