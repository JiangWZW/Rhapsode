"""Python callback implementations for C++ MemorySystem."""

from __future__ import annotations

import json
import logging

import chromadb
from sentence_transformers import SentenceTransformer

from rhapsode.lemmatization import lemmatize_for_bm25

log = logging.getLogger(__name__)

EMBEDDING_MODEL = "BAAI/bge-base-en-v1.5"
_shared_model: SentenceTransformer | None = None


def warmup_model() -> None:
    global _shared_model
    if _shared_model is None:
        log.info("Loading embedding model %s ...", EMBEDDING_MODEL)
        _shared_model = SentenceTransformer(EMBEDDING_MODEL)
        log.info("Embedding model ready.")


def register_callbacks(memory_system, scene_id: str, chroma_path: str = "./chroma"):
    """Register all Python callbacks on a C++ MemorySystem instance."""
    global _shared_model
    if _shared_model is None:
        warmup_model()
    model = _shared_model

    client = chromadb.PersistentClient(path=chroma_path)
    collections = {
        f"{scene_id}_facts": client.get_or_create_collection(
            name=f"{scene_id}_facts", metadata={"hnsw:space": "cosine"}),
        f"{scene_id}_entities": client.get_or_create_collection(
            name=f"{scene_id}_entities", metadata={"hnsw:space": "cosine"}),
    }

    def _col(name: str):
        return collections[name]

    def embed(text: str) -> str:
        return json.dumps(model.encode(text).tolist())

    def lemmatize(text: str) -> str:
        return lemmatize_for_bm25(text)

    def store(collection: str, id: str, doc: str, embedding_json: str, metadata_json: str):
        col = _col(collection)
        col.add(ids=[id], documents=[doc],
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

    def update_meta(collection: str, id: str, metadata_json: str):
        _col(collection).update(ids=[id], metadatas=[json.loads(metadata_json)])

    def get_by_meta(collection: str, where_json: str) -> str:
        col = _col(collection)
        where = json.loads(where_json)
        results = col.get(where=where if where else None)
        return json.dumps(results)

    memory_system.set_embed_callback(embed)
    memory_system.set_lemmatize_callback(lemmatize)
    memory_system.set_store_callback(store)
    memory_system.set_query_callback(query)
    memory_system.set_update_meta_callback(update_meta)
    memory_system.set_get_by_meta_callback(get_by_meta)

    log.info("Memory callbacks registered (facts=%d, entities=%d)",
             collections[f"{scene_id}_facts"].count(),
             collections[f"{scene_id}_entities"].count())
