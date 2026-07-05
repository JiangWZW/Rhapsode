"""LLM provider abstraction. Supports Gemini and DeepSeek, selectable via RHAPSODE_PROVIDER."""

import os
import time
import logging

log = logging.getLogger(__name__)

_provider = None

_MAX_RETRIES = 3

# Shared output-token ceiling for every provider.  Raised from the old hard-coded
# 4096 -- a reasoning/"pro" model can spend that on hidden reasoning and return
# empty content (finish_reason=length).  Billing is by actual usage, so a higher
# ceiling only adds headroom.  Override via env if a model still truncates.
_MAX_OUTPUT_TOKENS = int(os.environ.get("RHAPSODE_MAX_OUTPUT_TOKENS", "8192"))


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

    log.info("LLM provider: %s (model=%s)", name, _provider.model)
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


def _gemini_tool_use_loop(client, model, contents, config, tool_dispatcher) -> str:
    import json as json_module

    max_rounds = 10
    for _ in range(max_rounds):
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
            return " ".join(text_parts) if text_parts else (response.text or "")

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

    log.warning("Gemini tool-use loop exceeded %d rounds", max_rounds)
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

    def complete(self, messages: list[dict]) -> str:
        from google.genai.types import GenerateContentConfig

        system_parts, contents = _split_gemini_messages(messages)

        config = GenerateContentConfig(
            maxOutputTokens=_MAX_OUTPUT_TOKENS,
            temperature=1.0,
            systemInstruction="\n".join(system_parts) if system_parts else None,
        )
        kwargs = {"model": self.model, "contents": contents, "config": config}

        def _call():
            response = self.client.models.generate_content(**kwargs)
            text = response.text or ""
            if not text:
                try:
                    cand = (response.candidates or [None])[0]
                    log.warning(
                        "Gemini empty text: finish_reason=%s prompt_feedback=%s",
                        getattr(cand, "finish_reason", None),
                        getattr(response, "prompt_feedback", None),
                    )
                except Exception:  # noqa: BLE001 -- diagnostics must never throw
                    log.warning("Gemini empty text (no diagnostics available)")
            return text

        return _retry_complete(_call)

    def complete_with_tools(self, messages: list[dict], tools: list[dict],
                            tool_dispatcher) -> str:
        from google.genai.types import GenerateContentConfig, Tool

        system_parts, contents = _split_gemini_messages(messages)
        declarations = _tool_schemas_to_gemini_declarations(tools)

        config = GenerateContentConfig(
            maxOutputTokens=_MAX_OUTPUT_TOKENS,
            temperature=1.0,
            systemInstruction="\n".join(system_parts) if system_parts else None,
            tools=[Tool(function_declarations=declarations)],
        )

        return _gemini_tool_use_loop(
            self.client, self.model, contents, config, tool_dispatcher)


class _DeepSeekProvider:
    def __init__(self):
        from openai import OpenAI
        self.client = OpenAI(
            api_key=os.environ["DEEPSEEK_API_KEY"],
            base_url=os.environ.get("DEEPSEEK_API_BASE", "https://api.deepseek.com"),
        )
        self.model = os.environ.get("RHAPSODE_MODEL", "deepseek-chat")

    def complete(self, messages: list[dict]) -> str:
        converted = _to_openai_messages(messages)

        def _call():
            response = self.client.chat.completions.create(
                model=self.model,
                messages=converted,
                temperature=1.0,
                max_tokens=_MAX_OUTPUT_TOKENS,
            )
            choice = response.choices[0]
            content = choice.message.content or ""
            if not content:
                reasoning = getattr(choice.message, "reasoning_content", "") or ""
                log.warning(
                    "DeepSeek empty content: finish_reason=%s reasoning_len=%d usage=%s",
                    getattr(choice, "finish_reason", None),
                    len(reasoning),
                    getattr(response, "usage", None),
                )
            return content

        return _retry_complete(_call)

    def complete_with_tools(self, messages: list[dict], tools: list[dict],
                            tool_dispatcher) -> str:
        import json as json_module

        # Convert initial messages to OpenAI format
        openai_messages = _to_openai_messages(messages)

        # Convert tool schemas to OpenAI tools format
        openai_tools = [{"type": "function", "function": t} for t in tools]

        max_rounds = 10
        for _ in range(max_rounds):
            response = self.client.chat.completions.create(
                model=self.model,
                messages=openai_messages,
                tools=openai_tools,
                temperature=1.0,
                max_tokens=_MAX_OUTPUT_TOKENS,
            )
            msg = response.choices[0].message

            if not msg.tool_calls:
                return msg.content or ""

            # Add assistant message with tool_calls.
            # Discard intermediate text (msg.content) — it's preamble/commentary
            # the model produces alongside tool calls, not the final prose.
            # Keeping it would let the model see and repeat its own preamble.
            openai_messages.append({
                "role": "assistant",
                "content": None,
                "tool_calls": [
                    {
                        "id": tc.id,
                        "type": "function",
                        "function": {
                            "name": tc.function.name,
                            "arguments": tc.function.arguments or "{}",
                        },
                    }
                    for tc in msg.tool_calls
                ],
            })

            # Execute each tool call and add results
            for tc in msg.tool_calls:
                args = json_module.loads(tc.function.arguments) if tc.function.arguments else {}
                try:
                    result = tool_dispatcher(tc.function.name, args)
                except Exception as e:
                    result = json_module.dumps({"error": str(e)})
                openai_messages.append({
                    "role": "tool",
                    "tool_call_id": tc.id,
                    "content": result,
                })

        log.warning("DeepSeek tool-use loop exceeded %d rounds", max_rounds)
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


def complete(messages: list[dict]) -> str:
    """Call the configured LLM provider."""
    return _get_provider().complete(messages)


def complete_with_tools(messages: list[dict], tools: list[dict],
                        tool_dispatcher) -> str:
    """Run a tool-use conversation loop. Returns the final text response."""
    return _get_provider().complete_with_tools(messages, tools, tool_dispatcher)
