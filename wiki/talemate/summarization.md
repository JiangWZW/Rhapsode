---
sources:
  - "talemate:src/talemate/agents/summarize/__init__.py"
  - "talemate:src/talemate/agents/summarize/context_history.py"
  - "talemate:src/talemate/agents/summarize/layered_history.py"
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[talemate/context-assembly]]"
  - "[[talemate/retrieval-pipeline]]"
  - "[[talemate/comparison]]"
tags:
  - third-party-analysis
---

# Talemate — summarization

Talemate uses a three-tier compression hierarchy to manage conversation history: live dialogue (full fidelity), archived summaries (per-segment), and layered meta-summaries (recursive). A budget-aware assembly function (`context_history`) selects the right mix of detail levels to fill the available token window.

## Archive Trigger

Archiving fires from `SummarizeAgent.on_push_history` (line 192), which calls `build_archive` after every history push:

```python
# source: talemate:src/talemate/agents/summarize/__init__.py:192-201
async def on_push_history(self, emission: HistoryEvent):
    ws = self.scene.writing_style
    go = GenerationOptions(writing_style=ws)
    scene = self.scene
    kw = dict(generation_options=go)
    await self.build_archive(scene, **kw)
```

**Trigger condition**: accumulated dialogue tokens in `scene.history[start:]` crossing `token_threshold`.

- Default threshold: **1536 tokens** — configurable via UI, range 512–64000, lines 104–111.
- Scan window: from last archive endpoint to ``len(history) - 1`` — excludes newest line so it remains regenerable.
- Skipped message types at start boundary: `DirectorMessage`, `ContextInvestigationMessage`, `ReinforcementMessage` (line 330-337).
- `TimePassageMessage` can either trim the start or terminate the segment early (line 339-356).

## `build_archive` Logic (line 262)

The summarizer walks history between the last archive end and the current threshold breach:

```python
# source: talemate:src/talemate/agents/summarize/__init__.py:311-362
tokens = 0
dialogue_entries = []
token_threshold = self.actions["archive"].config["threshold"].value

hi = len(scene.history) - 1
upper_end = max(start, hi)

skip_types = (
    DirectorMessage,
    ContextInvestigationMessage,
    ReinforcementMessage,
)

for i in range(start, upper_end):
    dialogue = scene.history[i]
    if isinstance(dialogue, skip_types):
        if i == start:
            start += 1
        continue
    if isinstance(dialogue, TimePassageMessage):
        # Time passages terminate the segment early
        if i == start:
            start += 1
            continue
        end = i - 1
        break
    tokens += util.count_tokens(dialogue)
    dialogue_entries.append(dialogue)
    if tokens > token_threshold:
        end = i
        break
```

**Process after trigger:**

1. Accumulate tokens walking forward.
2. When `tokens > token_threshold`, set `end = i` and break.
3. Optional AI "natural termination" detection — skipped when a time-passage message already terminated the segment.
4. Summarize the segment via LLM.
5. Push to `scene.archived_history`.

## Layered History

The `LayeredHistoryMixin` — in `layered_history.py`, line 52 — provides recursive summarization. Layer 0 summarizes archived history entries. Each subsequent layer summarizes the previous layer's entries when their combined tokens exceed the threshold:

| Layer | Source                   | Content                               |
| ----- | ------------------------ | ------------------------------------- |
| 0     | `scene.archived_history` | Summaries of archive entries          |
| 1     | Layer 0 chunks           | Meta-summaries of layer 0             |
| N     | Layer N-1 chunks         | Progressively more abstract summaries |

**Configuration** (line 59-124):

- `threshold`: tokens before summarizing a layer (default 1536)
- `max_layers`: maximum depth (default 3)
- `max_process_tokens`: maximum tokens per summarization call (default 768)
- `chunk_size`: characters per chunk for detail retention (default 1280)
- `analyze_chunks`: enable per-chunk analysis for quality (default True)
- `response_length`: max summarization response length (default "2048")

**Layering mechanism:**

1. Summarizes archives into layer 0 when they cross the threshold — minimum 2 entries per chunk.
2. Iteratively builds higher layers from lower ones via `_lh_update_layers(max_layers)`.
3. Each layer entry stores `start`/`end` indices referencing the source layer.
4. **Validation**: summaries longer than their source are rejected — see `SummaryLongerThanOriginalError`, line 43.

**Compilation** — `compile_layered_history`: walks from the highest layer downward, respects `next_layer_start` so higher layers skip spans already superseded by lower layers. With `include_base_layer=True`, merges `scene.archived_history` after layer 0 with correct slicing.

## `context_history` Assembly

The budget-aware function in `ContextHistoryMixin` — line 124 of `context_history.py` — selects how much of each tier to include.

Two modes of operation:

### Ratio Mode

- `dialogue_ratio` (default 50%): percentage of budget allocated to raw dialogue.
- `summary_detail_ratio` (default 50%): how remaining budget is split across summary layers.
- Assembly order: highest layer → layer 0 → archived → dialogue.
- `enforce_boundary`: when true, dialogue won't expand into already-summarized content.

### Best-Fit Mode — default; enable with `best_fit=True`

```python
# source: talemate:src/talemate/agents/summarize/context_history.py:162-189
"best_fit": AgentActionConfig(
    type="bool",
    label="Best Fit Mode",
    description="Automatically distribute budget across layers to cover "
                "the full timeline with a detail gradient.",
    value=True,
),
```

Skips fixed ratios entirely. The algorithm selects the best detail level for each time segment — compressed at the start, detailed at the end. Additional settings:

- `best_fit_min_dialogue`: minimum guaranteed dialogue messages (default 5)
- `best_fit_max_dialogue`: scan limit for performance (default 250)

### Budget and Configuration

```python
# source: talemate:src/talemate/agents/summarize/context_history.py:150-161
"max_budget": AgentActionConfig(
    type="number",
    label="Max. Budget",
    description="Cap the context budget for scene history (in tokens). "
                "Set to 0 to use the full available budget.",
    value=8192,
    min=0,
    max=262144,
    step=512,
),
```

## `recent_history` API

A simpler retrieval on Scene (line 1216 of `tale_mate.py`):

```python
def context_history(self, budget=8192, **kwargs):
    summ = get_agent("summarizer")
    return summ.context_history(self, budget, **kwargs)
```

Delegates to the summarizer agent which handles all budget allocation and tier selection.

## Design Rationale

The three-tier hierarchy addresses the fundamental tension between context window limits and information preservation. Live dialogue provides full fidelity for immediate context. Archives compress completed segments. Layered history enables arbitrarily long play sessions by recursively compressing older material.

The archive trigger uses token counting rather than message counting because message lengths vary dramatically. The 1536-token default produces segments that fit comfortably in a single LLM summarization call while remaining long enough to capture narrative beats.

Ratio-based budget splitting gives predictable behavior. Best-fit mode, the default, trades predictability for maximum information density by selecting the best detail level per time segment.

## Limitations

- Archive quality depends entirely on the summarization LLM. Hallucinated or omitted details propagate to all higher layers.
- Layered history validation catches expansion but not semantic drift.
- The fixed threshold may produce awkward segment boundaries mid-scene.

## See Also

- [[talemate/context-assembly]]
- [[talemate/retrieval-pipeline]]
- [[talemate/comparison]]
