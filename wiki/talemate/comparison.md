---
sources:
  - server/rhapsode/memory.py
  - server/rhapsode/prompt.py
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[architecture/python-server]]"
  - "[[architecture/system-overview]]"
tags:
  - third-party-analysis
  - design
---

# Talemate vs Rhapsode — comparison and improvement roadmap

Gap analysis between Rhapsode's current memory implementation and Talemate's mature system. Identifies what to adopt, what doesn't fit, and the phased path to close the gap.

## Feature Matrix

| Feature | Rhapsode (current) | Talemate | Gap |
|---------|-------------------|----------|-----|
| **Vector store** | ChromaDB, one collection per scene | ChromaDB, one collection per scene | Parity |
| **Embedding model** | BAAI/bge-base-en-v1.5 (hardcoded) | Configurable presets (MiniLM, GTE, OpenAI) | Need preset system |
| **Document metadata** | Minimal (text only) | Typed metadata: `character`, `source`, `session`, `ts`, `typ`, `pin_only` | Need rich metadata |
| **Retrieval** | Single-query semantic search | Multi-query with round-robin, token-capped | Major gap |
| **AI-assisted retrieval** | None | Three modes (direct, queries, questions) | Major gap |
| **History management** | Full history in memory (C++ vector) | Archive cascade + layered meta-summaries | Major gap |
| **Token budgeting** | None (send all history) | Per-section budgets, `context_history` allocation | Major gap |
| **Prompt structure** | Ad-hoc string concatenation | Section-ordered Jinja templates with volatile placement | Moderate gap |
| **Fact maintenance** | None | Reinforcements (periodic Q&A), context pins with decay | Major gap |
| **Embedding backend** | Fixed SentenceTransformer | Pluggable (ST, OpenAI, client API) | Nice-to-have |
| **Result caching** | None | Fingerprint-keyed per-turn cache | Minor gap |
| **Distance gating** | Default ChromaDB behavior | Configurable `max_distance = distance * distance_mod` | Minor gap |

## What Rhapsode Should Adopt

### 1. Multi-query round-robin retrieval

Rhapsode's single-query retrieval misses relevant memories when the query doesn't align with stored phrasing. Multi-query with round-robin interleaving provides diversity without any single query dominating results.

**Adaptation**: implement `multi_query` in `memory.py` with `iterate` per-query cap and token-based cutoff. Expose `iterate` and `max_tokens` as configurable parameters.

### 2. Token budgeting for context assembly

Sending all history works for short scenes but fails as conversations grow. A budget-based system prevents context overflow and ensures each section gets proportional space.

**Adaptation**: add `count_tokens` utility (tiktoken), implement budget calculation in `prompt.py`: `max_tokens - reserve - static_tokens - volatile_tokens = history_budget`.

### 3. History summarization

The C++ `History` class stores all `SceneMessage` objects in a vector with no compression. Long sessions will exhaust context windows.

**Adaptation**: implement archive trigger in Python via a token threshold on history length, then LLM-driven summarization and a `scene.archived_history` list alongside live history.

### 4. Typed metadata in vector store

Rhapsode stores bare text without metadata. This prevents filtering by character, session, or memory type.

**Adaptation**: extend `ResolvedMemory.store_batch` to accept and store metadata fields: `character`, `source`, `session`, `ts`, `typ`. Update `where` filters in retrieval.

### 5. Configurable distance gating

Rhapsode relies on ChromaDB's default distance behavior. Explicit thresholding prevents low-relevance noise from consuming token budget.

**Adaptation**: add `max_distance` parameter to retrieval, filter results before returning.

## What Doesn't Fit

### Agent-per-concern architecture

Talemate uses separate agent classes — `MemoryAgent`, `SummarizeAgent`, `WorldStateAgent` — each with their own LLM interaction patterns. Rhapsode uses a C++ core with a single Python orchestration layer.

**Why it doesn't fit**: Rhapsode's Director (C++) already handles orchestration. Adding Python-level agent proliferation would duplicate control flow and create synchronization complexity with the C++ scene loop.

**Alternative**: keep Python-side memory and summarization as library functions called by the existing orchestration path.

### Jinja template system

Talemate builds prompts from Jinja2 templates with complex inheritance and group-aware loading. Rhapsode builds prompts in Python code.

**Why it doesn't fit**: Rhapsode's prompt construction is simple enough that Python string building is clear and debuggable. Jinja adds indirection without proportional benefit at current complexity.

**Alternative**: structured Python prompt builder with named sections and budget allocation — captures the token-budgeting benefit without template engine complexity.

### Reinforcement Q&A system

Talemate's reinforcements periodically re-ask questions to maintain world state. Rhapsode's Director + PlotGraph already maintains canonical facts through the node system.

**Why it doesn't fit**: PlotGraph nodes are the source of truth for world state in Rhapsode. A parallel reinforcement system would create competing authorities.

**Alternative**: the Director's existing node-traversal pass can be extended to inject canonical facts into context — similar effect with a single authority.

## Improvement Phases

### Phase 1: Metadata and distance gating

- Add metadata fields to `store_batch` and collection schema
- Implement configurable `max_distance` threshold
- Add `where` filters to retrieval path

### Phase 2: Multi-query retrieval

- Implement `multi_query` with round-robin and token cap
- Add `count_tokens` utility
- Add per-turn result caching (fingerprint-keyed)

### Phase 3: Token-budgeted context assembly

- Implement section-based budget calculation in prompt builder
- Add `limit_tokens` utility for hard truncation
- Track token usage per section

### Phase 4: History summarization

- Implement archive trigger using a token threshold on history length
- Add LLM summarization call for completed segments
- Store archived summaries alongside live history
- Integrate archived summaries into prompt builder's history section

### Phase 5: Advanced retrieval modes (optional)

- Implement AI-assisted sub-query generation (queries mode)
- Evaluate whether Q&A compilation (questions mode) benefits Rhapsode's use case
- Consider embedding preset system for model flexibility

## Design Decisions

### Multi-query over single-query

**Alternative**: retrieve more results from a single query (increase `n_results`).
**Why multi-query wins**: a single query's embedding captures one semantic angle. Multiple queries from different parts of recent conversation cover more ground. Round-robin prevents any single angle from dominating.

### Token budgeting over message-count limits

**Alternative**: keep last N messages (fixed count).
**Why budgeting wins**: message lengths vary dramatically — compare one-word replies to multi-paragraph turns. Token budgeting adapts to actual content density, ensuring consistent LLM utilization regardless of message verbosity.

### Archive threshold over time-based summarization

**Alternative**: summarize every N turns regardless of content length.
**Why threshold wins**: token accumulation directly measures context pressure. Short exchanges don't trigger premature summarization; verbose narration triggers it sooner.

### Python prompt builder over Jinja templates

**Alternative**: adopt Talemate's template system wholesale.
**Why Python builder wins**: current prompt complexity doesn't justify template indirection. Budget allocation logic is easier to debug in Python. Can always migrate to templates later if complexity grows.

## See Also

- [[talemate/_index]]
- [[architecture/python-server]]
- [[architecture/system-overview]]
