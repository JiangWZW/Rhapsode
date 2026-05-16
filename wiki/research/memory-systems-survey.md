---
sources:
  - "arxiv:2310.08560 (MemGPT)"
  - "arxiv:2501.13956 (Graphiti/Zep)"
  - "arxiv:2407.04363 (AriGraph)"
  - "arxiv:2502.12110 (A-Mem)"
  - "arxiv:2504.19413 (Mem0)"
last_updated: 2026-05-11
confidence: verified
tier: semantic
related:
  - "[[talemate/memory-architecture]]"
  - "[[architecture/system-overview]]"
tags:
  - research
  - memory-architecture
---

# Memory Systems Survey

Five systems for persistent LLM memory, evaluated for Rhapsode's node-based memory design. **Mem0 v3** was selected as the architectural basis.

## MemGPT / Letta (UC Berkeley, Oct 2023)

arXiv:2310.08560 | 505 citations | NeurIPS 2023 workshop

The LLM manages its own memory like an OS manages RAM.

Three tiers modeled after the OS memory hierarchy:

- **Core memory** — ~5000 chars, always in context — like registers. Holds persona and key user facts. The LLM can call `core_memory_replace` with persona `"human"`, prior value `"Lives in Austin"`, and new value `"Lives in Denver"`.
- **Recall storage** (full conversation log) — like RAM. Searchable by timestamp or keyword via `conversation_search()`.
- **Archival storage** (unlimited, vector-indexed) — like disk. The LLM calls `archival_memory_insert()` and `archival_memory_search()` to page data in and out.

The LLM decides what to page in/out via function calls. Each turn, it sees core memory + a sliding window of recent messages, and can pull older context on demand.

**Strengths**: elegant abstraction, works with any LLM via function calling.
**Weaknesses**: the LLM must learn when to page — poor paging decisions lose information silently. No structured conflict detection.

## Graphiti / Zep (Zep AI, Jan 2025)

arXiv:2501.13956 | 114 citations | 25,600 GitHub stars

Temporal knowledge graph for agent memory. Key contribution: handling **changing facts over time** without losing history.

- Extracts entities and relationships from each message via LLM.
- Builds a Neo4j property graph with temporal edges (valid_from, valid_to).
- When new info contradicts old edges, creates `SUPERSEDES` relationships rather than deleting.
- Hybrid retrieval: fulltext + vector + graph traversal.
- Entity deduplication via LLM-assisted resolution.

**Strengths**: strong temporal reasoning, explicit contradiction handling, production-grade (funded company).
**Weaknesses**: requires Neo4j (heavy dependency), LLM calls on both read and write paths.

## AriGraph (IJCAI 2025)

arXiv:2407.04363 | 54 citations

Knowledge graph world models for text-based games — the most directly relevant domain to Rhapsode.

- Agent explores a text game, building a knowledge graph of rooms, objects, NPCs, and their relationships.
- **Episodic memory**: raw observations stored chronologically.
- **Semantic memory**: structured knowledge graph extracted from episodes.
- Graph updates happen after each action: new entities and edges are extracted, existing ones are updated or invalidated.
- Retrieval: subgraph extraction around entities mentioned in the current observation.

**Strengths**: proven in text-game domain, clean separation of episodic vs semantic memory.
**Weaknesses**: graph construction requires per-step LLM calls, evaluated only on TextWorld environments.

## A-Mem (NeurIPS 2025)

arXiv:2502.12110

Zettelkasten-inspired agentic memory. Each memory is a "note" with rich metadata:

```
Note = {content, keywords, tags, context_description, embedding}
```

- **Link generation**: when a new note is stored, find related notes via embedding similarity, then ask the LLM whether a meaningful link exists.
- **Memory evolution**: new memories trigger updates to old ones — the LLM can rewrite a note's context description when new related information arrives.
- **No graph DB**: everything lives in a vector store. Links are metadata pointers, not graph edges.
- Tested on small models (Qwen 1.5B, Llama 1B).

**Strengths**: lightweight (vector store only), memories improve over time, works with tiny models.
**Weaknesses**: LLM-in-the-loop for link generation adds latency per write.

### Overlap with Rhapsode

| Rhapsode design | A-Mem equivalent |
|----------------|-----------------|
| Nodes with `{fact, type, entities, known_by}` | Notes with `{content, keywords, tags, context}` |
| Conflict detection — embedding similarity + entity overlap | Link generation — embedding similarity + LLM judgment |
| ChromaDB, no graph DB | Vector store, no graph DB |
| Qwen3 8B for validation | Tested on Qwen 1.5B and Llama 1B |

## Mem0 v3 (ECAI 2025) — SELECTED

arXiv:2504.19413 | 55,000 GitHub stars | Apache 2.0

Production memory layer. Moved *away* from graph memory (v2 used Neo4j) to a pure vector-store approach with entity linking inside the store.

### Write path

1. Extract atomic facts from conversation via LLM.
2. Hash-based deduplication (skip exact duplicates).
3. Embed each fact, store in a **facts collection**.
4. Extract entities, store in a separate **entities collection** with `linked_memory_ids` pointing back to facts.
5. Conflict detection via embedding similarity against existing facts.

### Read path — multi-signal fusion

Three retrieval signals combined additively:

- **Semantic**: cosine similarity from vector search on facts collection.
- **BM25**: keyword match with sigmoid normalization, applied to semantic candidates.
- **Entity boost**: embed the query, search the entity collection, boost facts linked to matching entities. Frequency attenuation prevents common entities from dominating.

No LLM call on the read path.

### Why we chose it

| Mem0 v3 | Rhapsode mapping |
|---------|-----------------|
| Dual collection (facts + entities) | ChromaDB facts + entities collections |
| Hash dedup | MD5 hash in metadata |
| Entity linking inside vector store | `linked_memory_ids` (JSON-serialized list) |
| Multi-signal retrieval (semantic + BM25 + entity) | Same three signals with sigmoid BM25 and entity attenuation |
| No graph DB needed | ChromaDB only |
| ADD-only extraction | Supersession over deletion (`turn_superseded` metadata) |

### Where Rhapsode diverges

- **Entity extraction**: Mem0 uses the main LLM; we use Qwen3 8B locally (batched).
- **Quality gate**: Mem0 trusts the extraction LLM; we add a validation step via Qwen3.
- **Conflict resolution**: Mem0 does similarity-based update/delete; we discard-and-log, preserving both sides.
- **Domain**: Mem0 targets general chat; we target terse RPG facts — max 15 words, node states, character knowledge tracking.

## Decision Rationale

| System | Why not |
|--------|---------|
| MemGPT | No structured conflict detection; relies on LLM paging decisions |
| Graphiti/Zep | Requires Neo4j; LLM on read path; over-engineered for our scale |
| AriGraph | Per-step LLM graph extraction too expensive; TextWorld-specific evaluation |
| A-Mem | Very close to our design, but Mem0 is more production-proven and has clearer retrieval fusion |
| **Mem0 v3** | **Selected** — dual-collection vector store, hash dedup, multi-signal retrieval, 55k stars, no graph DB |
