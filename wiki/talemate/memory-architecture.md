---
sources:
  - "talemate:src/talemate/agents/memory/__init__.py"
  - "talemate:src/talemate/config/schema.py"
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[talemate/retrieval-pipeline]]"
  - "[[talemate/comparison]]"
  - "[[architecture/python-server]]"
tags:
  - third-party-analysis
---

# Talemate — memory architecture

Talemate stores all memory types in a single ChromaDB collection per scene, discriminated by metadata fields rather than separate collections. The collection identity is derived from a fingerprint hash of the embedding configuration, allowing multiple embedding backends to coexist across scenes without collision.

## Single Collection Design

Each scene gets exactly one ChromaDB collection. The collection name is:

```
{scene.memory_id}-tm-{MD5(fingerprint)[:32]}
```

Where `fingerprint` is a lowercased string built from the embedding configuration:

```python
# source: talemate:src/talemate/agents/memory/__init__.py:214-221
@property
def fingerprint(self) -> str:
    model_name = self.model.replace("/", "-") if self.model else "none"
    return f"{self.embeddings}-{model_name}-{self.distance_function}-{self.device}-{self.trust_remote_code}".lower()
```

The MD5 hash satisfies ChromaDB's collection naming constraints — 3–63 chars, alphanumeric start/end, no consecutive periods.

All memory types — dialogue, history summaries, world state entries, narrator observations — coexist in this single collection, distinguished only by metadata.

## Embedding Backend Abstraction

Three paths exist for computing embeddings, selected by the `embeddings` config field:

| Backend | Config value | Implementation |
|---------|--------------|----------------|
| **SentenceTransformer** | `"default"` or `"sentence-transformer"` | `SentenceTransformerEmbeddingFunction` args: `model_name`, `trust_remote_code`, `device` |
| **OpenAI** | `"openai"` | `OpenAIEmbeddingFunction` — passes `api_key`, `model_name="text-embedding-3-small"` |
| **Client API** | `"client-api"` | Delegates to a connected client's `embeddings_function` |

```python
# source: talemate:src/talemate/agents/memory/__init__.py:890-989
def _set_db(self):
    self._ready_to_add = False
    if not getattr(self, "db_client", None):
        self.db_client = chromadb.PersistentClient(
            settings=Settings(anonymized_telemetry=False)
        )
    # ... selects branch based on self.embeddings:
    # - using_openai_embeddings → OpenAIEmbeddingFunction
    # - using_client_api_embeddings → client.embeddings_function
    # - else → SentenceTransformerEmbeddingFunction
    # Sets collection_metadata = {"hnsw:space": distance_function}
    self._ready_to_add = True
```

The `_set_db` method runs synchronously inside an executor via `asyncio.get_event_loop().run_in_executor`. The `_ready_to_add` flag gates all write operations until the embedding backend is fully initialized.

## Collection Identity

The `make_collection_name` function produces a deterministic, ChromaDB-compliant name:

```python
# source: talemate:src/talemate/agents/memory/__init__.py:871-884
def make_collection_name(self, scene) -> str:
    collection_name = f"{self.fingerprint}"
    md5_hash = hashlib.md5(collection_name.encode()).hexdigest()
    hashed_collection_name = md5_hash[:32]
    return f"{scene.memory_id}-tm-{hashed_collection_name}"
```

Switching embedding models or distance functions produces a different collection, so old embeddings from incompatible models are never mixed with new ones.

## Document Metadata Schema

Every document stored via `_add` (line 1046) or `_add_many` (line 1084) carries these metadata fields:

| Field | Type | Description |
|-------|------|-------------|
| `character` | str | Character name, or `"__narrator__"` for narrator entries |
| `source` | str | Always `"talemate"` (origin marker) |
| `session` | str | `scene.memory_session_id` — groups entries by play session |
| `ts` | str (optional) | ISO timestamp when provided |
| `typ` | str (optional) | Memory type discriminator, e.g. `"history"` for archived entries |
| `pin_only` | bool (optional) | If true, document is excluded from retrieval results (line 1193) |
| additional kwargs | varies | Passed through `meta.update(kwargs)` — enables arbitrary metadata |

Document IDs follow the pattern `{character_name}-{counter}` or `__narrator__-{counter}`, tracked per character via `memory_tracker`. The write operation uses `db.upsert()` (line 1082), enabling idempotent re-insertion.

## Embedding Presets

The `EmbeddingFunctionPreset` Pydantic model (from `config/schema.py:282`):

```python
# source: talemate:src/talemate/config/schema.py:282-294
class EmbeddingFunctionPreset(pydantic.BaseModel):
    embeddings: str = "sentence-transformer"
    model: str = "all-MiniLM-L6-v2"
    trust_remote_code: bool = False
    device: str = "cpu"
    distance: float = 1.5
    distance_mod: int = 1
    distance_function: str = "l2"
    fast: bool = True
    gpu_recommendation: bool = False
    local: bool = True
    custom: bool = False
    client: str | None = None
```

`generate_chromadb_presets()` (line 297) ships three built-in presets:

| Preset name | Model | Distance fn | Threshold | Local |
|-------------|-------|-------------|-----------|-------|
| `"default"` | `all-MiniLM-L6-v2` | l2 | 1.5 | yes |
| `"Alibaba-NLP/gte-base-en-v1.5"` | `Alibaba-NLP/gte-base-en-v1.5` | cosine | 1.0 | yes |
| `"openai"` | `text-embedding-3-small` | l2 | 1.0 | no |

## Distance Configuration

Retrieval distance thresholding uses a two-factor formula:

```python
# source: talemate:src/talemate/agents/memory/__init__.py:182-186
@property
def max_distance(self) -> float:
    cfg = self.embeddings_config
    distance = float(cfg.distance)
    distance_mod = float(cfg.distance_mod)
    return distance * distance_mod
```

The `distance` field is preset-defined (tuned per model); `distance_mod` is a user-adjustable multiplier. Documents with `distance >= max_distance` are rejected at retrieval time (line 1201).

## Design Rationale

The single-collection-per-scene approach avoids the coordination overhead of multiple collections. Metadata typing enables flexible filtering (`where` clauses) without collection proliferation. The fingerprint-based naming ensures model changes produce clean collections rather than corrupted mixed-embedding spaces.

## See Also

- [[talemate/retrieval-pipeline]]
- [[talemate/comparison]]
- [[architecture/python-server]]
