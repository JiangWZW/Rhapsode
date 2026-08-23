from rhapsode._core import CharacterMemory


def facts(mem):
    return [node.fact for node in mem.beliefs.all_nodes()]


def entities(mem):
    return [entity for node in mem.beliefs.all_nodes() for entity in node.entities]


def test_exact_canonical_keys_do_not_merge_by_substring():
    mem = CharacterMemory("Father Aldric")
    mem.seed_belief("Ash is the keep's blacksmith", ["Ash"], 0)
    mem.seed_belief(
        "Ashenmoor's eastern wall is breached", ["Ashenmoor"], 1,
        type="perception")

    assert "blacksmith" in mem.view_of(["Ash"])
    assert "breached" not in mem.view_of(["Ash"])
    assert "Ash" in entities(mem)
    assert "Ashenmoor" in entities(mem)


def test_perception_nodes_keep_keys_but_await_reflection():
    mem = CharacterMemory("Sergeant Maren")
    mem.seed_belief("The captain is my oldest friend", ["Player"], 0)
    mem.seed_belief(
        "The captain rallied the garrison", ["Player"], 1, type="perception")

    assert "oldest friend" in mem.view_of(["Player"])
    assert "rallied the garrison" not in mem.view_of(["Player"])
    assert any("rallied the garrison" in fact for fact in facts(mem))
    assert all(entity == "Player" for entity in entities(mem))


def test_noncanonical_key_remains_isolated_until_reflection():
    mem = CharacterMemory("Warden Elara Voss")
    mem.seed_belief("The disgraced captain is a liability", ["Player"], 0)
    mem.seed_belief(
        "Someone rallied soldiers", ["the captain"], 1, type="perception")

    assert "liability" in mem.view_of(["Player"])
    assert "rallied soldiers" not in mem.view_of(["Player"])
    assert "the captain" in entities(mem)
