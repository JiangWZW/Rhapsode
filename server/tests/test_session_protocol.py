from types import SimpleNamespace

from rhapsode.session import _player_text, _scene_ws_payload


def test_player_text_accepts_only_nonempty_player_messages():
    assert _player_text({"type": "player_message", "content": "  go north  "}) == "go north"
    assert _player_text({"type": "player_message", "content": "  "}) is None
    assert _player_text({"type": "status", "content": "go north"}) is None


def test_scene_payload_defaults_invalid_metadata_to_narrator():
    msg = SimpleNamespace(content="A door opens.", metadata="not-json")
    assert _scene_ws_payload(msg) == {
        "type": "scene_message",
        "content": "A door opens.",
        "scene_kind": "narrator",
    }


def test_scene_payload_preserves_character_metadata():
    msg = SimpleNamespace(
        content="Welcome.",
        metadata='{"scene_kind":"character","speaker":"Maren"}',
    )
    assert _scene_ws_payload(msg) == {
        "type": "scene_message",
        "content": "Welcome.",
        "scene_kind": "character",
        "speaker": "Maren",
    }
