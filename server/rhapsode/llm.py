"""LLM provider abstraction. Supports Gemini and DeepSeek, selectable via RHAPSODE_PROVIDER."""

import os
import time
import logging

log = logging.getLogger(__name__)

_provider = None

_MAX_RETRIES = 3

# Shared output-token ceiling for every provider.  Reasoning/"pro" models share
# this budget with hidden thinking; too low yields empty content +
# finish_reason=length.  Billing is by actual usage, so a higher ceiling only
# adds headroom.  Override via RHAPSODE_MAX_OUTPUT_TOKENS.
_MAX_OUTPUT_TOKENS = int(os.environ.get("RHAPSODE_MAX_OUTPUT_TOKENS", "65536"))
# Hard cap when length-retry doubles the budget after a thinking overflow.
_MAX_OUTPUT_TOKENS_CAP = 131072


def _bump_max_tokens(current: int) -> int | None:
    """Next max_tokens after a length overflow, or None if already at cap."""
    if current >= _MAX_OUTPUT_TOKENS_CAP:
        return None
    return min(max(current * 2, current + 1), _MAX_OUTPUT_TOKENS_CAP)


def _deepseek_extra_body() -> dict:
    """DeepSeek V4 thinking controls (reasoning_content + final content share max_tokens).

    RHAPSODE_DEEPSEEK_THINKING:
      enabled|1|on|true   — thinking mode (default; required for pro quality)
      disabled|0|off|false — no reasoning_content (faster, cheaper)
    """
    raw = (os.environ.get("RHAPSODE_DEEPSEEK_THINKING") or "enabled").strip().lower()
    if raw in ("0", "false", "off", "no", "disabled"):
        return {"thinking": {"type": "disabled"}}
    return {"thinking": {"type": "enabled"}}


def _resolve_thinking_body(
    default: dict, thinking: bool | None
) -> tuple[dict, bool]:
    """Return (extra_body, thinking_on) for a DeepSeek request."""
    if thinking is None:
        body = default
    elif thinking:
        body = {"thinking": {"type": "enabled"}}
    else:
        body = {"thinking": {"type": "disabled"}}
    on = body.get("thinking", {}).get("type") == "enabled"
    return body, on


def _is_kimi_model(model: str | None) -> bool:
    return bool(model) and str(model).strip().lower().startswith("kimi-")


def _narrator_api_base() -> str:
    return (os.environ.get("RHAPSODE_NARRATOR_API_BASE") or "").strip()


def _narrator_api_key() -> str:
    return (
        (os.environ.get("RHAPSODE_NARRATOR_API_KEY") or "").strip()
        or (os.environ.get("MOONSHOT_API_KEY") or "").strip()
    )


def _kimi_reasoning_effort() -> str:
    raw = (os.environ.get("RHAPSODE_NARRATOR_REASONING_EFFORT") or "max").strip().lower()
    if raw not in ("low", "high", "max"):
        raise ValueError(
            f"RHAPSODE_NARRATOR_REASONING_EFFORT={raw!r} is not low, high, or max."
        )
    return raw


def _require_kimi_config(model: str) -> None:
    """Refuse to start a Kimi call without a Moonshot host and key."""
    if not _is_kimi_model(model):
        return
    missing: list[str] = []
    if not _narrator_api_base():
        missing.append("RHAPSODE_NARRATOR_API_BASE")
    if not _narrator_api_key():
        missing.append("RHAPSODE_NARRATOR_API_KEY or MOONSHOT_API_KEY")
    if missing:
        raise ValueError(
            f"RHAPSODE_NARRATOR_MODEL={model!r} needs a Moonshot host and key "
            f"(missing {', '.join(missing)}). "
            "China keys use https://api.moonshot.cn/v1; "
            "international keys use https://api.moonshot.ai/v1."
        )


def _openai_completion_kwargs(
    *,
    model: str,
    messages: list,
    thinking: bool | None,
    default_extra_body: dict,
    max_tokens: int,
    tools: list | None = None,
) -> dict:
    """Chat Completions kwargs for the model on this call.

    Kimi K3: official shape only — reasoning_effort, max_completion_tokens,
    no thinking body, no temperature. DeepSeek keeps today's thinking body.
    """
    if _is_kimi_model(model):
        kwargs: dict = {
            "model": model,
            "messages": messages,
            "max_completion_tokens": max_tokens,
            "reasoning_effort": _kimi_reasoning_effort(),
        }
        if tools is not None:
            kwargs["tools"] = tools
        return kwargs
    extra_body, thinking_on = _resolve_thinking_body(default_extra_body, thinking)
    kwargs = {
        "model": model,
        "messages": messages,
        "max_tokens": max_tokens,
        "extra_body": extra_body,
    }
    if tools is not None:
        kwargs["tools"] = tools
    if thinking_on:
        kwargs["reasoning_effort"] = "high"
    else:
        kwargs["temperature"] = 1.0
    return kwargs


def _set_output_budget(kwargs: dict, max_tokens: int) -> None:
    if "max_completion_tokens" in kwargs:
        kwargs["max_completion_tokens"] = max_tokens
    else:
        kwargs["max_tokens"] = max_tokens


def _assistant_history_entry(msg, *, kimi: bool) -> dict:
    """Replay an assistant turn into the next request's messages.

    Kimi K3 requires the complete assistant message (including
    reasoning_content). DeepSeek keeps the existing rebuilt dict.
    """
    if kimi and hasattr(msg, "model_dump"):
        dumped = msg.model_dump(exclude_none=True)
        dumped.setdefault("role", "assistant")
        return dumped
    assistant_msg = {
        "role": "assistant",
        "content": msg.content or "",
        "tool_calls": [
            {
                "id": tc.id,
                "type": "function",
                "function": {
                    "name": tc.function.name,
                    "arguments": tc.function.arguments or "{}",
                },
            }
            for tc in (msg.tool_calls or [])
        ],
    }
    reasoning = getattr(msg, "reasoning_content", None)
    if reasoning:
        assistant_msg["reasoning_content"] = reasoning
    return assistant_msg


def _retry_complete(fn):
    """Retry an LLM call with exponential backoff.

    Retries on BOTH a raised exception and a falsy (empty) return -- an empty
    completion is not an exception but is usually a transient gateway/model
    hiccup worth retrying.  Contract preserved: a persistent exception still
    raises on the final attempt; a persistent empty still returns "" (callers
    tolerate the empty string and must not start crashing on it).
    """
    result = ""
    for attempt in range(_MAX_RETRIES):
        last = attempt == _MAX_RETRIES - 1
        try:
            result = fn()
        except Exception as e:
            if last:
                raise
            wait = 2 ** attempt
            log.warning("LLM call failed (attempt %d/%d): %s -- retrying in %ds",
                        attempt + 1, _MAX_RETRIES, e, wait)
            time.sleep(wait)
            continue
        if result:
            return result
        if last:
            return result  # persistent empty -- preserve the "" fallback contract
        wait = 2 ** attempt
        log.warning("LLM returned empty (attempt %d/%d) -- retrying in %ds",
                    attempt + 1, _MAX_RETRIES, wait)
        time.sleep(wait)
    return result


def _get_provider():
    global _provider
    if _provider is not None:
        return _provider

    name = os.environ.get("RHAPSODE_PROVIDER", "gemini").lower()

    if name == "gemini":
        _provider = _GeminiProvider()
    elif name == "deepseek":
        _provider = _DeepSeekProvider()
    else:
        raise ValueError(f"Unknown RHAPSODE_PROVIDER: {name!r}. Use 'gemini' or 'deepseek'.")

    tool_model = getattr(_provider, "tool_model", _provider.model)
    log.info("LLM provider: %s (model=%s, tool_model=%s)",
             name, _provider.model, tool_model)
    return _provider


def _split_gemini_messages(messages: list[dict]) -> tuple[list[str], list[dict]]:
    """Separate system-role messages (-> joined text parts) from the rest."""
    system_parts: list[str] = []
    contents: list[dict] = []
    for msg in messages:
        if msg.get("role") == "system":
            system_parts.extend(p.get("text", "") for p in msg.get("parts", []))
        else:
            contents.append(msg)
    return system_parts, contents


def _tool_schemas_to_gemini_declarations(tools: list[dict]) -> list:
    from google.genai.types import FunctionDeclaration, Schema, Type

    type_map = {
        "string": Type.STRING,
        "number": Type.NUMBER,
        "integer": Type.INTEGER,
        "boolean": Type.BOOLEAN,
        "array": Type.ARRAY,
        "object": Type.OBJECT,
    }
    declarations = []
    for t in tools:
        props = {}
        for prop_name, prop_schema in t.get("parameters", {}).get("properties", {}).items():
            props[prop_name] = Schema(
                type=type_map.get(prop_schema.get("type", "string"), Type.STRING),
                description=prop_schema.get("description", ""),
            )
        declarations.append(FunctionDeclaration(
            name=t["name"],
            description=t["description"],
            parameters=Schema(
                type=Type.OBJECT,
                properties=props,
                required=t.get("parameters", {}).get("required", []),
            ),
        ))
    return declarations


def _stage_tag(stage: str) -> str:
    return f"[{stage}] " if stage else ""


def _gemini_tool_use_loop(client, model, contents, config, tool_dispatcher,
                          *, stage: str = "", max_rounds: int = 10) -> str:
    import json as json_module
    tag = _stage_tag(stage)
    for round_i in range(max_rounds):
        log.info("Gemini %stools round=%d/%d model=%s (waiting on API…)",
                 tag, round_i + 1, max_rounds, model)
        response = client.models.generate_content(
            model=model, contents=contents, config=config,
        )

        candidate = (response.candidates or [None])[0]
        if not candidate or not candidate.content or not candidate.content.parts:
            return response.text or ""

        # Check for function calls
        has_fc = False
        fc_parts = []
        text_parts = []
        for part in candidate.content.parts:
            if hasattr(part, "function_call") and part.function_call:
                has_fc = True
                fc_parts.append(part.function_call)
            elif hasattr(part, "text") and part.text:
                text_parts.append(part.text)

        if not has_fc:
            log.info("Gemini %stools round=%d done tool_calls=0",
                     tag, round_i + 1)
            return " ".join(text_parts) if text_parts else (response.text or "")

        tool_names = ",".join(fc.name for fc in fc_parts)
        log.info("Gemini %stools round=%d done tool_calls=%d tools=%s",
                 tag, round_i + 1, len(fc_parts), tool_names)

        # Add the model's response (with function calls) to contents
        # Gemini uses "model" role for assistant
        contents.append({"role": "model", "parts": [
            {"function_call": {"name": fc.name, "args": dict(fc.args) if fc.args else {}}}
            for fc in fc_parts
        ]})

        # Execute each function call and add responses
        for fc in fc_parts:
            name = fc.name
            args = dict(fc.args) if fc.args else {}
            try:
                result = tool_dispatcher(name, args)
            except Exception as e:
                result = json_module.dumps({"error": str(e)})
            contents.append({"role": "user", "parts": [
                {"function_response": {"name": name, "response": {"result": result}}}
            ]})

    log.warning("Gemini %stool-use loop exceeded %d rounds", tag, max_rounds)
    return ""


class _GeminiProvider:
    def __init__(self):
        from google import genai
        from google.genai.types import HttpOptions
        base_url = os.environ.get("RHAPSODE_API_BASE")
        http_options = HttpOptions(baseUrl=base_url) if base_url else None
        self.client = genai.Client(
            api_key=os.environ["GOOGLE_API_KEY"],
            http_options=http_options,
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "gemini-2.0-flash")
        self.tool_model = os.environ.get("RHAPSODE_NARRATOR_MODEL") or self.model

    def complete(self, messages: list[dict], model: str | None = None,
                 *, thinking: bool | None = None, stage: str = "",
                 phase: str = "") -> str:
        from google.genai.types import GenerateContentConfig

        del thinking  # Gemini path has no DeepSeek thinking toggle.
        _ = phase
        system_parts, contents = _split_gemini_messages(messages)
        use_model = model or self.model
        tag = _stage_tag(stage)

        config = GenerateContentConfig(
            maxOutputTokens=_MAX_OUTPUT_TOKENS,
            temperature=1.0,
            systemInstruction="\n".join(system_parts) if system_parts else None,
        )
        kwargs = {"model": use_model, "contents": contents, "config": config}

        def _call():
            log.info("Gemini %scall start model=%s (waiting on API…)",
                     tag, use_model)
            response = self.client.models.generate_content(**kwargs)
            text = response.text or ""
            log.info("Gemini %scall done model=%s content_len=%d",
                     tag, use_model, len(text))
            if not text:
                try:
                    cand = (response.candidates or [None])[0]
                    log.warning(
                        "Gemini %sempty text: finish_reason=%s prompt_feedback=%s",
                        tag,
                        getattr(cand, "finish_reason", None),
                        getattr(response, "prompt_feedback", None),
                    )
                except Exception:  # noqa: BLE001 -- diagnostics must never throw
                    log.warning("Gemini %sempty text (no diagnostics available)",
                                tag)
            return text

        return _retry_complete(_call)

    def complete_with_tools(self, messages: list[dict], tools: list[dict],
                            tool_dispatcher, *, model: str | None = None,
                            thinking: bool | None = None,
                            stage: str = "", phase: str = "",
                            max_rounds: int | None = None) -> str:
        from google.genai.types import GenerateContentConfig, Tool

        del thinking  # Gemini path has no DeepSeek thinking toggle.
        _ = phase
        system_parts, contents = _split_gemini_messages(messages)
        declarations = _tool_schemas_to_gemini_declarations(tools)

        config = GenerateContentConfig(
            maxOutputTokens=_MAX_OUTPUT_TOKENS,
            temperature=1.0,
            systemInstruction="\n".join(system_parts) if system_parts else None,
            tools=[Tool(function_declarations=declarations)],
        )

        return _gemini_tool_use_loop(
            self.client, model or self.tool_model, contents, config,
            tool_dispatcher, stage=stage,
            max_rounds=10 if max_rounds is None else max_rounds)


class _DeepSeekProvider:
    def __init__(self):
        from openai import OpenAI
        self.client = OpenAI(
            api_key=os.environ["DEEPSEEK_API_KEY"],
            base_url=os.environ.get("DEEPSEEK_API_BASE", "https://api.deepseek.com"),
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "deepseek-chat")
        # Narrator + inner thoughts use this id when callers omit model=.
        # Scheduler / lifecycle pass RHAPSODE_MODEL (Flash) explicitly.
        self.tool_model = os.environ.get("RHAPSODE_NARRATOR_MODEL") or self.model
        self._extra_body = _deepseek_extra_body()
        self.kimi_client = None
        if _is_kimi_model(self.tool_model):
            _require_kimi_config(self.tool_model)
            self.kimi_client = OpenAI(
                api_key=_narrator_api_key(),
                base_url=_narrator_api_base(),
            )
            log.info(
                "Narrator client host=%s model=%s effort=%s",
                _narrator_api_base(), self.tool_model, _kimi_reasoning_effort(),
            )
        log.info(
            "DeepSeek thinking=%s",
            self._extra_body.get("thinking", {}).get("type", "?"),
        )

    def _client_for(self, model: str):
        if not _is_kimi_model(model):
            return self.client
        _require_kimi_config(model)
        if self.kimi_client is None:
            from openai import OpenAI
            self.kimi_client = OpenAI(
                api_key=_narrator_api_key(),
                base_url=_narrator_api_base(),
            )
        return self.kimi_client

    def complete(self, messages: list[dict], model: str | None = None,
                 *, thinking: bool | None = None, stage: str = "",
                 phase: str = "") -> str:
        from rhapsode.llm_profile import reasoning_tokens, record_api_hop

        converted = _to_openai_messages(messages)
        max_tokens = _MAX_OUTPUT_TOKENS
        use_model = model or self.model
        tag = _stage_tag(stage)
        kimi = _is_kimi_model(use_model)
        vendor = "Kimi" if kimi else "DeepSeek"
        thinking_on = True if kimi else _resolve_thinking_body(
            self._extra_body, thinking)[1]
        client = self._client_for(use_model)

        def _call():
            nonlocal max_tokens
            log.info(
                "%s %scall start model=%s thinking=%s max_tokens=%d "
                "(waiting on API…)",
                vendor, tag, use_model, thinking_on, max_tokens,
            )
            t0 = time.monotonic()
            kwargs = _openai_completion_kwargs(
                model=use_model,
                messages=converted,
                thinking=thinking,
                default_extra_body=self._extra_body,
                max_tokens=max_tokens,
            )
            response = client.chat.completions.create(**kwargs)
            elapsed_ms = int((time.monotonic() - t0) * 1000)
            choice = response.choices[0]
            content = choice.message.content or ""
            finish = getattr(choice, "finish_reason", None)
            usage = getattr(response, "usage", None)
            reasoning = getattr(choice.message, "reasoning_content", None)
            log.info(
                "%s %scall done phase=%s finish=%s elapsed_ms=%d "
                "content_len=%d reasoning_tokens=%s",
                vendor, tag, phase or "-", finish, elapsed_ms, len(content),
                reasoning_tokens(usage),
            )
            record_api_hop(
                kind="complete",
                stage=stage,
                phase=phase,
                model=use_model,
                thinking=thinking_on,
                wall_ms=elapsed_ms,
                finish=finish,
                content_len=len(content),
                usage=usage,
                reasoning=reasoning,
            )
            if not content:
                reasoning = reasoning or ""
                log.warning(
                    "%s %sempty content: finish_reason=%s reasoning_len=%d "
                    "max_tokens=%d usage=%s",
                    vendor,
                    tag,
                    finish,
                    len(reasoning),
                    max_tokens,
                    usage,
                )
                if finish == "length":
                    nxt = _bump_max_tokens(max_tokens)
                    if nxt is not None:
                        log.warning(
                            "%s %slength budget exhausted at max_tokens=%d; "
                            "bumping to %d for retry",
                            vendor, tag, max_tokens, nxt,
                        )
                        max_tokens = nxt
            return content

        return _retry_complete(_call)

    def complete_with_tools(self, messages: list[dict], tools: list[dict],
                            tool_dispatcher, *, model: str | None = None,
                            thinking: bool | None = None,
                            stage: str = "", phase: str = "",
                            max_rounds: int | None = None) -> str:
        import json as json_module

        from rhapsode.llm_profile import reasoning_tokens, record_api_hop

        openai_messages = _to_openai_messages(messages)
        openai_tools = [{"type": "function", "function": t} for t in tools]
        use_model = model or self.tool_model
        tag = _stage_tag(stage)
        kimi = _is_kimi_model(use_model)
        vendor = "Kimi" if kimi else "DeepSeek"
        thinking_on = True if kimi else _resolve_thinking_body(
            self._extra_body, thinking)[1]
        client = self._client_for(use_model)

        max_tokens = _MAX_OUTPUT_TOKENS
        if max_rounds is None:
            max_rounds = 10
        create_kwargs = _openai_completion_kwargs(
            model=use_model,
            messages=openai_messages,
            thinking=thinking,
            default_extra_body=self._extra_body,
            max_tokens=max_tokens,
            tools=openai_tools,
        )

        for round_i in range(max_rounds):
            while True:
                create_kwargs["messages"] = openai_messages
                _set_output_budget(create_kwargs, max_tokens)
                log.info(
                    "%s %stools round=%d/%d phase=%s model=%s thinking=%s "
                    "max_tokens=%d (waiting on API…)",
                    vendor, tag, round_i + 1, max_rounds, phase or "-",
                    use_model, thinking_on, max_tokens,
                )
                t0 = time.monotonic()
                response = client.chat.completions.create(**create_kwargs)
                elapsed_ms = int((time.monotonic() - t0) * 1000)
                choice = response.choices[0]
                msg = choice.message
                finish = getattr(choice, "finish_reason", None)
                usage = getattr(response, "usage", None)
                n_tools = len(msg.tool_calls or [])
                tool_names = ",".join(
                    tc.function.name for tc in (msg.tool_calls or [])
                )
                log.info(
                    "%s %stools round=%d done phase=%s finish=%s "
                    "elapsed_ms=%d tool_calls=%d%s content_len=%d "
                    "reasoning_tokens=%s",
                    vendor, tag, round_i + 1, phase or "-", finish, elapsed_ms,
                    n_tools,
                    f" tools={tool_names}" if tool_names else "",
                    len(msg.content or ""), reasoning_tokens(usage),
                )
                record_api_hop(
                    kind="tools_round",
                    stage=stage,
                    phase=phase,
                    model=use_model,
                    thinking=thinking_on,
                    wall_ms=elapsed_ms,
                    finish=finish,
                    content_len=len(msg.content or ""),
                    tool_round=round_i + 1,
                    tool_calls=n_tools,
                    tools=tool_names,
                    usage=usage,
                    reasoning=getattr(msg, "reasoning_content", None),
                )

                if msg.tool_calls:
                    break

                content = msg.content or ""
                if content:
                    return content

                reasoning = getattr(msg, "reasoning_content", "") or ""
                log.warning(
                    "%s %sempty content: finish_reason=%s reasoning_len=%d "
                    "max_tokens=%d usage=%s",
                    vendor,
                    tag,
                    finish,
                    len(reasoning),
                    max_tokens,
                    getattr(response, "usage", None),
                )
                if finish == "length":
                    nxt = _bump_max_tokens(max_tokens)
                    if nxt is not None:
                        log.warning(
                            "%s %slength budget exhausted at max_tokens=%d; "
                            "bumping to %d for retry",
                            vendor, tag, max_tokens, nxt,
                        )
                        max_tokens = nxt
                        continue
                return ""

            # Replay the assistant turn so the next round can continue.
            # Kimi K3: whole message as-is (Preserved Thinking).
            # DeepSeek: rebuilt dict with reasoning_content.
            # reasoning_content is API-only history — never returned as prose.
            openai_messages.append(_assistant_history_entry(msg, kimi=kimi))

            for tc in msg.tool_calls:
                args = (
                    json_module.loads(tc.function.arguments)
                    if tc.function.arguments else {}
                )
                try:
                    result = tool_dispatcher(tc.function.name, args)
                except Exception as e:
                    result = json_module.dumps({"error": str(e)})
                openai_messages.append({
                    "role": "tool",
                    "tool_call_id": tc.id,
                    "content": result,
                })

        log.warning("%s %stool-use loop exceeded %d rounds", vendor, tag, max_rounds)
        return ""


def _to_openai_messages(gemini_messages: list[dict]) -> list[dict]:
    """Convert Gemini-style messages to OpenAI-style messages."""
    result = []
    for msg in gemini_messages:
        role = msg.get("role", "user")
        parts = msg.get("parts", [])
        text = " ".join(p.get("text", "") for p in parts if "text" in p)
        result.append({"role": role, "content": text})
    return result


def complete(messages: list[dict], model: str | None = None,
             *, thinking: bool | None = None, stage: str = "",
             phase: str = "") -> str:
    """Call the configured LLM provider (optionally on a specific model)."""
    return _get_provider().complete(
        messages, model, thinking=thinking, stage=stage, phase=phase)


def complete_with_tools(messages: list[dict], tools: list[dict],
                        tool_dispatcher, *, model: str | None = None,
                        thinking: bool | None = None,
                        stage: str = "", phase: str = "",
                        max_rounds: int | None = None) -> str:
    """Run a tool-use conversation loop. Returns the final text response.

    When model is omitted, providers use their tool/narrator model (pro).
    thinking=None keeps RHAPSODE_DEEPSEEK_THINKING; True/False overrides per call.
    stage/phase label logs and optional RHAPSODE_LLM_PROFILE JSONL hops.
    max_rounds=None keeps the default 10-round loop.
    """
    return _get_provider().complete_with_tools(
        messages, tools, tool_dispatcher, model=model, thinking=thinking,
        stage=stage, phase=phase, max_rounds=max_rounds)
