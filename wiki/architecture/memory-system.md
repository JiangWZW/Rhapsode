---
sources:
  - core/include/rhapsode/memory_system.h
  - core/src/memory_system.cpp
  - server/rhapsode/memory.py
  - server/rhapsode/validator.py
  - server/rhapsode/lemmatization.py
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/python-server]]"
  - "[[talemate/comparison]]"
  - "[[research/memory-systems-survey]]"
  - "[[research/memory-systems-internals]]"
tags:
  - cpp-core
  - python-server
  - cross-layer
---

# Memory system

The memory system gives Rhapsode persistent, weighted knowledge of what happened during a game session. It stores facts extracted from plot nodes, retrieves them using hybrid search, and enriches them through a multi-step quality pipeline.

## Architecture split

The memory system is split across C++ and Python:

| Concern | Language | Why |
|---------|----------|-----|
| Scoring logic (BM25, entity boosting, dedup) | C++ | Deterministic, testable, no I/O |
| Retrieval orchestration | C++ | Combines semantic + keyword + entity signals |
| Post-turn pipeline control flow | C++ | Sequences distill → score → extract → conflict → store |
| Embedding (BAAI/bge-base-en-v1.5) | Python | sentence-transformers is Python-native |
| Vector storage (ChromaDB) | Python | Chroma is Python-native |
| Lemmatization (spaCy) | Python | spaCy is Python-native |
| Local LLM calls (llama.cpp) | Python | HTTP client to localhost:8012 |

C++ owns the `MemorySystem` class. Python registers seven callbacks at startup:

| Callback | Signature | Python implementation |
|----------|-----------|---------------------|
| `embed` | `text → embedding_json` | `sentence_transformers.encode()` → JSON array |
| `lemmatize` | `text → lemmatized_text` | `spacy_models.get_nlp_lemma()` → stop-word removal + lemma |
| `store` | `(collection, id, doc, embedding_json, metadata_json) → void` | `chromadb.Collection.add()` |
| `query` | `(collection, embedding_json, n, where_json) → results_json` | `chromadb.Collection.query()` |
| `update_meta` | `(collection, id, metadata_json) → void` | `chromadb.Collection.update()` |
| `get_by_meta` | `(collection, where_json) → results_json` | `chromadb.Collection.get()` |
| `local_llm` | `prompt → response_text` | `httpx.post()` to llama.cpp `/v1/chat/completions` |

## Storage layout

Each scene gets two ChromaDB collections (persistent, cosine distance):

| Collection | Contents |
|------------|----------|
| `{scene_id}_facts` | Distilled facts from plot nodes. Each document has metadata: `state`, `type`, `known_by`, `entities`, `turn`, `hash`, `lemmatized` |
| `{scene_id}_entities` | Entity index. Links entity names to the fact IDs that mention them |

Embeddings use **BAAI/bge-base-en-v1.5** (768-dimensional). The model is loaded once at server startup (`warmup_model()`).

## Fact storage

`store_fact()` takes a fact string, its state, type, `known_by` list, entity list, and turn number. It:

1. Computes an MD5 hash of the fact text
2. Checks for duplicates — same hash already in the collection
3. Embeds the fact — or accepts a pre-computed embedding
4. Lemmatizes the fact for BM25 keyword matching
5. Stores in the facts collection with full metadata
6. Links each entity to the fact via the entities collection

## Retrieval

Two retrieval methods:

### `retrieve(query, top_k)`

General-purpose retrieval combining three signals:

| Signal | Mechanism | Weight |
|--------|-----------|--------|
| **Semantic** | Embed the query, Chroma cosine query, top-k | Primary ranking |
| **BM25** | Lemmatize query, compute Okapi BM25 against stored lemmatized facts | Re-ranking boost |
| **Entity** | Query the entities collection, boost facts linked to matched entities | Re-ranking boost |

BM25 uses adaptive parameters based on query length via `get_bm25_params()`. Scores are normalized through a sigmoid function.

### `retrieve_for_injection(scene_context, max_results)`

Specialized for the Director's established-facts injection. Retrieves facts relevant to the scene context and returns them as a JSON array of strings. The output is formatted for the "ESTABLISHED FACTS" block in the Director system prompt.

## Post-turn pipeline

After each turn completes, `process_new_nodes()` runs on newly created and resolved nodes:

```
Input: vector<Node> new_nodes, int turn
                │
                ▼
        ┌── Distill verbose facts ──┐
        │   Local LLM shortens      │
        │   facts > threshold        │
        └───────────┬───────────────┘
                    ▼
        ┌── Quality score batch ────┐
        │   Local LLM rates nodes   │
        │   against existing pool   │
        └───────────┬───────────────┘
                    ▼
        ┌── Entity extraction ──────┐
        │   Local LLM identifies    │
        │   named entities          │
        └───────────┬───────────────┘
                    ▼
        ┌── For each surviving node ┐
        │   ├── Embed fact          │
        │   ├── Conflict detection      │
        │   │   — semantic proximity    │
        │   │     to existing facts     │
        │   └── store_fact()        │
        └───────────────────────────┘
```

Each step uses the local LLM callback at llama.cpp port 8012. If unreachable, each step returns an empty string and the pipeline continues with reduced quality. Facts are stored without distillation, scoring, or enriched entity links.

### Distill

Facts longer than a threshold are shortened by the local LLM into concise atomic statements. Follows the Director's fact format guidelines — max ~15 words, no hedging, no compound sentences.

### Quality scoring

New nodes are batch-scored against the existing pool. The LLM rates whether each fact is meaningful, non-redundant, and relevant. Low-quality facts may be filtered or downgraded.

### Entity extraction

The LLM identifies named entities from each fact: characters, locations, factions, objects. These populate the `entities` field and are linked in the entities collection for entity-boosted retrieval.

### Conflict detection

Before storing, each fact is embedded and compared to existing facts by semantic proximity. High-similarity matches are checked for contradiction. Detected conflicts can be flagged or resolved — current behavior: store anyway, log the conflict.

## Duplicate detection

Every fact is MD5-hashed before storage. If a fact with the same hash already exists in the collection, it is skipped. This prevents identical facts from accumulating across turns.

## Graceful degradation

The memory system is designed to work at multiple quality levels:

| Condition | Behavior |
|-----------|----------|
| Local LLM available | Full pipeline: distill, score, entity extract, conflict check |
| Local LLM unreachable | Facts stored raw, entities from node metadata only, no quality scoring |
| Chroma empty | Retrieval returns nothing; Director operates without established facts |
| spaCy unavailable | BM25 falls back to lowercased input tokens |

## Configuration

| Setting | Default | Source |
|---------|---------|--------|
| Embedding model | BAAI/bge-base-en-v1.5 | `memory.py:EMBEDDING_MODEL` |
| Chroma path | `./chroma` | `register_callbacks()` parameter |
| Local LLM URL | `http://localhost:8012` | `validator.py:LLAMA_URL` |
| Local LLM timeout | 120 seconds | `validator.py:LLAMA_TIMEOUT` |
| Default retrieval top_k | 8 | `MemorySystem::retrieve()` parameter |
