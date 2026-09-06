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


def test_bump_max_tokens_doubles_until_cap():
    assert llm._bump_max_tokens(65536) == 131072
    assert llm._bump_max_tokens(131072) is None
    assert llm._bump_max_tokens(80000) == 131072


def test_deepseek_extra_body_defaults_enabled(monkeypatch):
    monkeypatch.delenv("RHAPSODE_DEEPSEEK_THINKING", raising=False)
    assert llm._deepseek_extra_body() == {"thinking": {"type": "enabled"}}
    monkeypatch.setenv("RHAPSODE_DEEPSEEK_THINKING", "disabled")
    assert llm._deepseek_extra_body() == {"thinking": {"type": "disabled"}}


def test_is_kimi_model():
    assert llm._is_kimi_model("kimi-k3")
    assert llm._is_kimi_model("Kimi-K3")
    assert not llm._is_kimi_model("deepseek-v4-pro")
    assert not llm._is_kimi_model("")
    assert not llm._is_kimi_model(None)


def test_kimi_kwargs_have_no_thinking_or_temperature(monkeypatch):
    monkeypatch.delenv("RHAPSODE_NARRATOR_REASONING_EFFORT", raising=False)
    kw = llm._openai_completion_kwargs(
        model="kimi-k3",
        messages=[{"role": "user", "content": "hi"}],
        thinking=True,
        default_extra_body={"thinking": {"type": "enabled"}},
        max_tokens=65536,
    )
    assert kw["reasoning_effort"] == "max"
    assert kw["max_completion_tokens"] == 65536
    assert "extra_body" not in kw
    assert "temperature" not in kw
    assert "max_tokens" not in kw
    assert "thinking" not in kw


def test_kimi_kwargs_honor_effort_override(monkeypatch):
    monkeypatch.setenv("RHAPSODE_NARRATOR_REASONING_EFFORT", "high")
    kw = llm._openai_completion_kwargs(
        model="kimi-k3",
        messages=[],
        thinking=None,
        default_extra_body={"thinking": {"type": "enabled"}},
        max_tokens=100,
        tools=[{"type": "function", "function": {"name": "x"}}],
    )
    assert kw["reasoning_effort"] == "high"
    assert "tools" in kw


def test_kimi_effort_rejects_unknown(monkeypatch):
    monkeypatch.setenv("RHAPSODE_NARRATOR_REASONING_EFFORT", "medium")
    with pytest.raises(ValueError, match="low, high, or max"):
        llm._kimi_reasoning_effort()


def test_deepseek_kwargs_keep_thinking_body():
    kw = llm._openai_completion_kwargs(
        model="deepseek-v4-pro",
        messages=[],
        thinking=True,
        default_extra_body={"thinking": {"type": "enabled"}},
        max_tokens=100,
    )
    assert kw["extra_body"] == {"thinking": {"type": "enabled"}}
    assert kw["reasoning_effort"] == "high"
    assert kw["max_tokens"] == 100
    assert "temperature" not in kw
    assert "max_completion_tokens" not in kw


def test_require_kimi_config_fails_without_host_or_key(monkeypatch):
    monkeypatch.delenv("RHAPSODE_NARRATOR_API_BASE", raising=False)
    monkeypatch.delenv("RHAPSODE_NARRATOR_API_KEY", raising=False)
    monkeypatch.delenv("MOONSHOT_API_KEY", raising=False)
    with pytest.raises(ValueError, match="Moonshot"):
        llm._require_kimi_config("kimi-k3")
    llm._require_kimi_config("deepseek-v4-pro")


def test_kimi_provider_fails_fast_without_key(monkeypatch):
    monkeypatch.setenv("DEEPSEEK_API_KEY", "sk-test")
    monkeypatch.setenv("RHAPSODE_NARRATOR_MODEL", "kimi-k3")
    monkeypatch.setenv("RHAPSODE_NARRATOR_API_BASE", "https://api.moonshot.cn/v1")
    monkeypatch.delenv("RHAPSODE_NARRATOR_API_KEY", raising=False)
    monkeypatch.delenv("MOONSHOT_API_KEY", raising=False)
    with pytest.raises(ValueError, match="MOONSHOT_API_KEY"):
        llm._DeepSeekProvider()


def test_kimi_provider_starts_with_moonshot_key(monkeypatch):
    monkeypatch.setenv("DEEPSEEK_API_KEY", "sk-test")
    monkeypatch.setenv("RHAPSODE_NARRATOR_MODEL", "kimi-k3")
    monkeypatch.setenv("RHAPSODE_NARRATOR_API_BASE", "https://api.moonshot.cn/v1")
    monkeypatch.setenv("MOONSHOT_API_KEY", "sk-moon")
    monkeypatch.delenv("RHAPSODE_NARRATOR_API_KEY", raising=False)
    provider = llm._DeepSeekProvider()
    assert provider.kimi_client is not None
    assert provider.kimi_client.api_key == "sk-moon"
    assert provider.kimi_client.base_url == "https://api.moonshot.cn/v1/"


def test_assistant_history_kimi_keeps_reasoning():
    class Msg:
        def model_dump(self, exclude_none=True):
            return {
                "role": "assistant",
                "content": "",
                "reasoning_content": "think",
                "tool_calls": [{
                    "id": "1",
                    "type": "function",
                    "function": {"name": "x", "arguments": "{}"},
                }],
            }

    entry = llm._assistant_history_entry(Msg(), kimi=True)
    assert entry["reasoning_content"] == "think"
    assert entry["tool_calls"][0]["id"] == "1"


def test_narrator_callback_passes_pro_model(monkeypatch):
    from rhapsode import llm_tools

    seen = {}

    def fake_complete_with_tools(*_args, **kwargs):
        seen.update(kwargs)
        return "ok"

    monkeypatch.setenv("RHAPSODE_NARRATOR_MODEL", "kimi-k3")
    monkeypatch.setattr(llm_tools, "complete_with_tools", fake_complete_with_tools)
    llm_tools.make_narrator_callback()("konosuba", "how-to", "state", lambda *_: "{}")
    assert seen["model"] == "kimi-k3"
