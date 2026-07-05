"""Python callback implementations for C++ MemorySystem and CharacterMemory."""

from __future__ import annotations

import json
import logging

import chromadb
from sentence_transformers import SentenceTransformer


log = logging.getLogger(__name__)

EMBEDDING_MODEL = "BAAI/bge-base-en-v1.5"
_shared_model: SentenceTransformer | None = None
_shared_client: chromadb.ClientAPI | None = None


def warmup_model() -> None:
    global _shared_model
    if _shared_model is None:
        log.info("Loading embedding model %s ...", EMBEDDING_MODEL)
        _shared_model = SentenceTransformer(EMBEDDING_MODEL)
        log.info("Embedding model ready.")


def _get_client(chroma_path: str = "./chroma") -> chromadb.ClientAPI:
    global _shared_client
    if _shared_client is None:
        _shared_client = chromadb.PersistentClient(path=chroma_path)
    return _shared_client


def _make_embed():
    """Return a shared embed callback."""
    global _shared_model
    if _shared_model is None:
        warmup_model()
    model = _shared_model

    def embed(text: str) -> str:
        return json.dumps(model.encode(text).tolist())
    return embed


def _make_chroma_callbacks(client: chromadb.ClientAPI):
    """Build store/query callbacks backed by a shared ChromaDB client.

    Collections are created lazily and cached.
    """
    cache: dict[str, chromadb.Collection] = {}

    def _col(name: str) -> chromadb.Collection:
        if name not in cache:
            cache[name] = client.get_or_create_collection(
                name=name, metadata={"hnsw:space": "cosine"})
        return cache[name]

    def store(collection: str, doc_id: str, doc: str, embedding_json: str, metadata_json: str):
        _col(collection).upsert(
            ids=[doc_id], documents=[doc],
            embeddings=[json.loads(embedding_json)],
            metadatas=[json.loads(metadata_json)])

    def query(collection: str, embedding_json: str, n: int, where_json: str) -> str:
        col = _col(collection)
        count = col.count()
        n = min(n, count) if count > 0 else 0
        if n == 0:
            return json.dumps({"ids": [[]], "distances": [[]], "documents": [[]], "metadatas": [[]]})
        where = json.loads(where_json)
        results = col.query(
            query_embeddings=[json.loads(embedding_json)],
            n_results=n,
            where=where if where else None,
            include=["documents", "metadatas", "distances"],
        )
        return json.dumps(results)

    def update_meta(collection: str, doc_id: str, metadata_json: str):
        _col(collection).update(ids=[doc_id], metadatas=[json.loads(metadata_json)])

    def delete(collection: str, ids_json: str):
        _col(collection).delete(ids=json.loads(ids_json))

    return store, query, update_meta, delete


def register_callbacks(memory_system, scene_id: str, chroma_path: str = "./chroma"):
    """Register all Python callbacks on a C++ MemorySystem instance."""
    embed = _make_embed()
    client = _get_client(chroma_path)
    store, query, update_meta, delete = _make_chroma_callbacks(client)

    memory_system.set_embed_callback(embed)
    memory_system.set_store_callback(store)
    memory_system.set_query_callback(query)
    memory_system.set_update_meta_callback(update_meta)
    memory_system.set_delete_callback(delete)

    log.info("MemorySystem callbacks registered for scene %s", scene_id)
