---
sources:
  - "talemate:src/talemate/agents/memory/__init__.py"
  - "talemate:src/talemate/agents/memory/rag.py"
  - "talemate:src/talemate/prompts/base.py"
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[talemate/memory-architecture]]"
  - "[[talemate/context-assembly]]"
  - "[[talemate/comparison]]"
tags:
  - third-party-analysis
---

# Talemate — retrieval pipeline

Talemate retrieves memories through a multi-query pipeline with round-robin interleaving, token-capped assembly, and optional AI-assisted sub-question generation. The system supports three retrieval modes of increasing sophistication: direct semantic search, LLM-generated sub-queries, and LLM-compiled Q&A answers.

## `multi_query` Algorithm

The core retrieval function (line 531) accepts multiple query strings and interleaves their results with deduplication and a global token budget:

```python
# source: talemate:src/talemate/agents/memory/__init__.py:531-610
async def multi_query(
    self,
    queries: list[str],
    iterate: int = 1,
    max_tokens: int = 1000,
    filter: Callable = lambda x: True,
    formatter: Callable = lambda x: x,
    limit: int = 10,
    return_docs: bool = False,
    **where,
):
```

**Algorithm steps:**

1. **Per-query retrieval**: For each non-empty query, call `await self.get(formatter(query), limit=limit, **where)` — vector similarity search with distance gating.

2. **Per-query filtering**: Walk results, apply `filter(memory)`. Keep at most `iterate` accepted strings per query → builds `per_query_results: list[list[str]]`.

3. **Round-robin interleave**: Starting at index 0, take the `idx`-th item from each query's result list in order. Skip duplicates already in `memory_context`.

4. **Token cap**: After each append, check `util.count_tokens(memory_context) >= max_tokens`. If exceeded, return immediately.

5. **Exhaustion**: Stop when a full pass adds nothing new.

The `iterate` parameter is a **per-query cap** on how many hits enter the interleaved pool, not the global result count. With `iterate=5` and 4 queries, up to 20 results enter the round-robin, but the token budget typically truncates well before that.

## `MemoryRAGMixin.semantic_context`

The base retrieval path (line 285 of `rag.py`):

```python
# source: talemate:src/talemate/agents/memory/rag.py:285-342
async def semantic_context(
    self,
    num_messages: int = 3,
    min_query_length: int = 100,
    max_response_tokens: int = 1024,
):
```

**Steps:**

1. Collect recent messages from scene history via `scene.collect_messages` — `max_messages=num_messages`, `typ=["character", "narrator", "director"]`.
2. Split messages into sentences using `compile_text_to_sentences`.
3. Merge short sentences to meet `min_query_length` via `compile_sentences_to_length`.
4. Build queries = raw messages + derived sentence chunks.
5. Call `memory.multi_query` with `queries`, `max_tokens=max_response_tokens`, `iterate=5`.
6. Return the assembled memory context list.

## `MemoryRAGMixin.rag_build`

The full RAG orchestrator (line 175 of `rag.py`):

```python
# source: talemate:src/talemate/agents/memory/rag.py:175-283
async def rag_build(
    self,
    character: "Character | None" = None,
    prompt: str = "",
    sub_instruction: str = "",
    retrieval_method: str | None = None,
    include_raw_semantic: bool = True,
) -> list[str]:
```

**Flow:**

1. Check cache (fingerprint-keyed). If hit and no `retrieval_method` override, return cached.
2. Call `self.semantic_context` with `num_messages=self.long_term_memory_num_messages` → raw semantic results.
3. If `retrieval_method == "direct"`: return semantic context directly.
4. Otherwise build prompt from `scene.context_history`; set `budget` to the integer truncation of ``client.max_token_length * 0.75``.
5. Dispatch to WorldState agent for AI-assisted retrieval.
6. Merge: wrap `semantic_context + memory_context` with `set`, then `list(...)` — when `include_raw_semantic=True`.
7. Cache results (unless method was overridden).

## Three Retrieval Modes

Each mode adds a layer of LLM assistance on top of semantic search:

### `direct`

Pure semantic retrieval. Builds queries from recent messages → `semantic_context(...)` → returns results. No LLM calls during retrieval itself.

### `queries`

1. Computes `semantic_context` (same as direct mode).
2. Calls `world_state.analyze_text_and_extract_context_via_queries` — passes `prompt`, `sub_instruction`, `include_character_context=True`, `num_queries`, `response_length`, `extra_context=semantic_context`.
3. The LLM generates targeted sub-queries based on conversation context.
4. Merges LLM-retrieved results with semantic results.

### `questions`

1. Computes `semantic_context`.
2. Calls `world_state.analyze_text_and_extract_context` — passes `prompt`, `sub_instruction`, `include_character_context=True`, `num_queries`, `response_length`, `extra_context=semantic_context`.
3. The LLM compiles answers to questions about the scene, split by newlines.
4. Same merge/dedupe with optional raw semantic inclusion.

The `questions` mode produces pre-synthesized answers rather than raw memory excerpts — useful when the downstream prompt benefits from coherent summaries rather than fragments.

## Result Caching

The RAG mixin caches results per scene to avoid redundant vector searches within a turn:

- **Cache key**: based on `long_term_memory_cache_key` property combining retrieval method, number of queries, and answer length (line 139-145 of `rag.py`).
- **Fingerprint**: `scene.history[-1].fingerprint` — invalidates when new history arrives.
- **Storage**: `scene.rag_cache` dictionary, reset on agent connect (line 151).
- **Bypass**: explicit `retrieval_method` override skips cache read/write to prevent cross-contamination between modes.

## Distance Threshold Gating

Before results enter `multi_query`, they pass through `_get` (line 1134) which applies strict distance filtering:

```python
# source: talemate:src/talemate/agents/memory/__init__.py:1173-1210
max_distance = self.max_distance  # distance * distance_mod from preset

for i in range(len(drow)):  # drow = _results["distances"][0]
    distance = drow[i]
    # ...
    if meta.get("pin_only", False):
        continue  # skip pin-only entries
    if distance < max_distance:
        # Accept — wrap in MemoryDocument, add date prefix from ts
        rid = _results["ids"][0][i]
        doc = MemoryDocument(doc, meta, rid, raw)
        results.append(doc)
```

`max_distance = distance * distance_mod` from the embedding preset. Documents with `pin_only=True` are always excluded from retrieval results regardless of distance.

## Design Rationale

Round-robin interleaving ensures diversity across queries — no single query dominates the context window. The per-query `iterate` cap prevents one highly-productive query from starving others. Token budgeting as the hard stop (rather than result count) adapts to varying document lengths.

The three-mode hierarchy trades latency for recall quality:

- **Direct** — fastest, no LLM calls
- **Queries** — one LLM round-trip for better coverage
- **Questions** — synthesis at the cost of two LLM calls

## See Also

- [[talemate/memory-architecture]]
- [[talemate/context-assembly]]
- [[talemate/comparison]]
