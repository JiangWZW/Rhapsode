---
sources:
  - "talemate:src/talemate/prompts/base.py"
  - "talemate:src/talemate/prompts/templates/conversation/dialogue.jinja2"
  - "talemate:src/talemate/util/__init__.py"
  - "talemate:src/talemate/agents/summarize/context_history.py"
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[talemate/summarization]]"
  - "[[talemate/retrieval-pipeline]]"
  - "[[talemate/reinforcements]]"
  - "[[talemate/comparison]]"
tags:
  - third-party-analysis
---

# Talemate — context assembly

Talemate assembles LLM prompts through token-budgeted section allocation with KV-cache-aware ordering. Each section has a defined role and position in the prompt, and a Jinja2 template architecture handles the composition per agent type.

## Token Counting

All token estimation uses a single shared encoder:

```python
# source: talemate:src/talemate/util/__init__.py:19
TIKTOKEN_ENCODING = tiktoken.encoding_for_model("gpt-4-turbo")

# source: talemate:src/talemate/util/__init__.py:22-39
def count_tokens(source) -> int:
    if isinstance(source, list):
        t = 0
        for s in source:
            t += count_tokens(s)
    elif isinstance(source, (str, SceneMessage)):
        txt = str(source)
        t = len(TIKTOKEN_ENCODING.encode(txt))
    else:
        t = 0
    return t
```

Lists are summed recursively; `SceneMessage` objects are converted to string before encoding. This is an intentional approximation — the actual model's tokenizer may differ, but `gpt-4-turbo`'s BPE provides a consistent baseline across all backends.

## Budget Calculation

The available token budget for `context_history` (the scene/history section) is computed directly in Jinja templates:

```jinja2
{# source: talemate:src/talemate/prompts/templates/conversation/dialogue.jinja2:47-51 #}
{% set rendered_context_tokens = count_tokens(rendered_context_text) + count_tokens(task_main_text) %}
{% set volatile_context_tokens = count_tokens(volatile_context_text) %}
{% set scene_history = scene.context_history(
    budget=max_tokens-200-rendered_context_tokens-volatile_context_tokens, ...) %}
```

Where:

- `max_tokens`: the LLM client's context window size
- `200`: fixed reserve for response generation overhead
- `rendered_context_tokens`: tokens consumed by static sections — `characters`, `additional information`
- `volatile_context_tokens`: tokens consumed by RAG/dynamic content

The same formula appears in `update-reinforcements.jinja2` (line 30).

## Section Ordering

The prompt is assembled in a fixed section sequence — verified from `dialogue.jinja2`:


| Position | Section                  | Content                                                             |
| -------- | ------------------------ | ------------------------------------------------------------------- |
| 1        | `CHARACTERS`             | Character sheets, physical descriptions, personality                |
| 2        | `SCENE DESCRIPTION`      | (optional) Scene setting and atmosphere                             |
| 3        | `ADDITIONAL INFORMATION` | Reinforcements (`extra-context-static.jinja2`), active context pins |
| 4        | `SCENE`                  | `context_history` output — budgeted history + summaries             |
| 5        | `VOLATILE`               | RAG results (`memory-context.jinja2`), dynamic instructions         |


Sections 1–3 are **stable** (change rarely during a scene). Section 4 grows with each turn. Section 5 changes every turn based on retrieval.

## Volatile Context Placement

The `VOLATILE` section can be positioned in two locations depending on the `optimize_prompt_caching` setting:

```python
# source: talemate:src/talemate/prompts/base.py:958-973
def volatile_context_placement(self):
    agent_ctx = active_agent.get()
    if not agent_ctx:
        return "before_history"
    agent = agent_ctx.agent
    agent_override = None
    for action in agent.actions.values():
        if action.config and "optimize_prompt_caching" in action.config:
            agent_override = action.config["optimize_prompt_caching"].value
            break
    if agent_override and agent_override != "auto":
        return "after_history" if agent_override == "on" else "before_history"
```

### `before_history` (default)

Volatile content appears between `ADDITIONAL INFORMATION` and `SCENE`. The stable character/info prefix changes rarely, enabling partial KV-cache reuse across turns for that prefix.

### `after_history` (cache-optimized)

Volatile content moves to after the `SCENE` section. This maximizes the stable prefix length — `characters` + info + growing history — giving longer KV-cache hits when the LLM provider supports prefix caching. The template handles this via:

```jinja2
{# source: talemate:src/talemate/prompts/templates/world_state/update-reinforcements.jinja2:55-63 #}
{% if volatile_placement != "after_history" %}
{{ volatile_context_text }}
{% endif %}
<|SECTION:SCENE|>
{{ scene_context_text }}
<|CLOSE_SECTION|>
{% if volatile_placement == "after_history" %}
{{ volatile_context_text }}
{% endif %}
```

## `limit_tokens` Helper

A utility for hard-truncating text to a token budget:

```python
# source: talemate:src/talemate/util/__init__.py:42-56
def limit_tokens(text: str, limit: int) -> str:
    lines = text.split("\n")
    while count_tokens(lines) > limit:
        lines.pop()  # drops last line
    return "\n".join(lines)
```

Removes lines from the **end** until under budget. Used for sections that must fit a fixed allocation without summarization.

## Jinja Template Architecture

Templates are resolved through a priority-ordered loader chain — from `Prompt.template_env`, line 333:

1. `prepended_template_dirs` (context variable)
2. Scene-specific template directories (via `active_scene`)
3. Config priority groups: custom groups → repo templates
4. Module templates
5. Package defaults: `src/talemate/prompts/templates/{agent_type}` and `.../common`

All template files use `.jinja2` extension.

### Template Global Functions

The `render()` method (line 495) injects numerous globals into the Jinja environment:

- `set_prepared_response(response)` — primes the LLM response prefix
- `set_data_response(initial_object)` — primes JSON/YAML structured output
- `query_memory(query, ...)` — synchronous memory query from within template
- `count_tokens(text)` — token counting
- `volatile_context_placement()` — returns placement strategy string
- `agent_action(name)` / `agent_config(name, key)` — access agent settings

### Template Organization

```
src/talemate/prompts/templates/
├── common/
│   ├── extra-context-static.jinja2    # Reinforcements + pins injection
│   ├── memory-context.jinja2          # RAG build integration
│   └── memory-query.jinja2            # Direct memory query
├── conversation/
│   └── dialogue.jinja2                # Main conversation prompt
├── world_state/
│   └── update-reinforcements.jinja2   # Reinforcement Q&A
└── per-agent-type directories: narrator, director, editor, etc.
```

### Response Priming

Templates use `set_prepared_response` to prime LLM outputs with expected prefixes:

```jinja2
{# source: update-reinforcements.jinja2:98 #}
{{ set_prepared_response("<ANSWER>") }}
```

This causes the client to prepend `<ANSWER>` to the generation, guiding the model to produce properly-tagged output that can be extracted by `AnchorExtractor`.

## Design Rationale

Token budgeting as the organizing principle — rather than fixed section sizes or message counts — adapts naturally to varying content lengths. Short character sheets leave more room for history; detailed world-building compresses the dialogue window.

The section ordering (stable → growing → volatile) is designed for KV-cache efficiency. Most LLM providers cache from the prompt prefix, so placing rarely-changing content first maximizes cache hits.

Jinja2 templating separates prompt structure from logic, allowing per-scene or per-agent customization without code changes. The priority chain means users can override any template at the scene level without forking the codebase.

## Limitations

- `tiktoken gpt-4-turbo` approximation can over- or under-count by 5-15% for non-GPT models.
- `limit_tokens` drops full lines, which can orphan markdown formatting or split mid-thought.
- The fixed 200-token reserve is a heuristic — some models need more, some less.

## See Also

- [[talemate/summarization]]
- [[talemate/retrieval-pipeline]]
- [[talemate/reinforcements]]
- [[talemate/comparison]]

