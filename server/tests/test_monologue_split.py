from rhapsode.llm_tools import (
    MONOLOGUE_USER_SENTINEL,
    PERCEPTION_USER_SENTINEL,
    make_perception_callback,
    make_reflection_callback,
    split_monologue_prompt,
)


def test_split_monologue_prompt_cuts_at_sentinel():
    blob = (
        "craft and schema\n"
        f"{MONOLOGUE_USER_SENTINEL}\n"
        "You are Aqua.\nWho you are:\nbible\n\nThe chair's legs bark.\n"
    )
    system, user = split_monologue_prompt(blob)
    assert system == "craft and schema"
    assert user.startswith("You are Aqua.")
    assert "The chair's legs bark." in user
    assert "What just happened" not in user
    assert MONOLOGUE_USER_SENTINEL not in system
    assert MONOLOGUE_USER_SENTINEL not in user


def test_split_monologue_prompt_missing_sentinel_keeps_blob():
    blob = "old single user blob"
    system, user = split_monologue_prompt(blob)
    assert system is None
    assert user == blob


def test_reflection_callback_sends_system_and_user(monkeypatch):
    captured = {}

    def fake_complete(messages, **kwargs):
        captured["messages"] = messages
        captured["kwargs"] = kwargs
        return "{}"

    monkeypatch.setattr("rhapsode.llm_tools.complete", fake_complete)
    prompt = f"shared craft\n{MONOLOGUE_USER_SENTINEL}\nYou are Aqua.\n"
    make_reflection_callback()(prompt)

    assert [m["role"] for m in captured["messages"]] == ["system", "user"]
    assert captured["messages"][0]["parts"][0]["text"] == "shared craft"
    assert captured["messages"][1]["parts"][0]["text"] == "You are Aqua.\n"
    assert captured["kwargs"]["stage"] == "monologue"
    assert captured["kwargs"]["thinking"] is True
    assert "model" in captured["kwargs"]


def test_reflection_callback_missing_sentinel_is_one_user_blob(monkeypatch):
    captured = {}

    def fake_complete(messages, **kwargs):
        captured["messages"] = messages
        captured["kwargs"] = kwargs
        return "{}"

    monkeypatch.setattr("rhapsode.llm_tools.complete", fake_complete)
    make_reflection_callback()("old blob without sentinel")

    assert len(captured["messages"]) == 1
    assert captured["messages"][0]["role"] == "user"
    assert captured["messages"][0]["parts"][0]["text"] == "old blob without sentinel"
    assert captured["kwargs"]["stage"] == "monologue"
    assert captured["kwargs"]["thinking"] is True


def test_perception_callback_sends_system_and_user(monkeypatch):
    captured = {}

    def fake_complete(messages, **kwargs):
        captured["messages"] = messages
        captured["kwargs"] = kwargs
        return "{}"

    monkeypatch.setattr("rhapsode.llm_tools.complete", fake_complete)
    prompt = f"first-person craft\n{PERCEPTION_USER_SENTINEL}\nThe bell rings.\n"
    make_perception_callback()(prompt)

    assert [m["role"] for m in captured["messages"]] == ["system", "user"]
    assert captured["messages"][0]["parts"][0]["text"] == "first-person craft"
    assert captured["messages"][1]["parts"][0]["text"] == "The bell rings.\n"
    assert captured["kwargs"]["stage"] == "perception"
    assert captured["kwargs"]["thinking"] is True
    assert "model" in captured["kwargs"]
