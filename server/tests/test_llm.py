import logging

import pytest

from rhapsode import llm


class Counter:
    def __init__(self, behavior):
        self.behavior = behavior
        self.calls = 0

    def __call__(self):
        attempt = self.calls
        self.calls += 1
        return self.behavior(attempt)


def test_retry_returns_immediate_result(monkeypatch):
    monkeypatch.setattr(llm.time, "sleep", lambda _seconds: None)
    call = Counter(lambda _attempt: "hello")
    assert llm._retry_complete(call) == "hello"
    assert call.calls == 1


def test_retry_recovers_from_empty(monkeypatch):
    monkeypatch.setattr(llm.time, "sleep", lambda _seconds: None)
    call = Counter(lambda attempt: "" if attempt == 0 else "recovered")
    assert llm._retry_complete(call) == "recovered"
    assert call.calls == 2


def test_retry_preserves_persistent_empty_contract(monkeypatch, caplog):
    monkeypatch.setattr(llm.time, "sleep", lambda _seconds: None)
    call = Counter(lambda _attempt: "")
    with caplog.at_level(logging.WARNING, logger=llm.log.name):
        assert llm._retry_complete(call) == ""
    assert call.calls == llm._MAX_RETRIES
    assert "empty" in caplog.text.lower()


def test_retry_raises_final_exception(monkeypatch):
    monkeypatch.setattr(llm.time, "sleep", lambda _seconds: None)

    def fail(_attempt):
        raise RuntimeError("api down")

    call = Counter(fail)
    with pytest.raises(RuntimeError, match="api down"):
        llm._retry_complete(call)
    assert call.calls == llm._MAX_RETRIES


def test_message_conversions_are_pure():
    messages = [
        {"role": "system", "parts": [{"text": "rule one"}, {"text": "rule two"}]},
        {"role": "user", "parts": [{"text": "hello"}]},
        {"role": "model", "parts": [{"text": "hi"}]},
    ]
    system, contents = llm._split_gemini_messages(messages)
    assert system == ["rule one", "rule two"]
    assert contents == messages[1:]
    assert llm._to_openai_messages(contents) == [
        {"role": "user", "content": "hello"},
        {"role": "model", "content": "hi"},
    ]
