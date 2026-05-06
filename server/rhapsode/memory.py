"""ChromaDB-backed long-term memory for resolved plot nodes."""

from __future__ import annotations

import json
import re
import logging

import chromadb
from sentence_transformers import SentenceTransformer

log = logging.getLogger(__name__)

EMBEDDING_MODEL      = "Alibaba-NLP/gte-base-en-v1.5"
DISTANCE_THRESHOLD   = 1.0
MAX_RESULTS_PER_QUERY = 5
MAX_TOTAL_CHARS       = 4096  # ~1024 tokens


def _split_sentences(text: str) -> list[str]:
    """Split text on sentence boundaries, keeping non-empty fragments."""
    parts = re.split(r'(?<=[.!?])\s+', text.strip())
    return [s for s in parts if len(s) > 5]


class ResolvedMemory:
    """Stores resolved node facts as vector embeddings in ChromaDB.

    Provides retrieval of semantically relevant facts given recent dialogue.
    Designed to be called synchronously from the Director tick cycle.
    """

    def __init__(self, scene_id: str, chroma_path: str = "./chroma"):
        self.model = SentenceTransformer(EMBEDDING_MODEL, trust_remote_code=True)
        self.client = chromadb.PersistentClient(path=chroma_path)
        self.collection = self.client.get_or_create_collection(
            name=f"resolved-{scene_id}",
            metadata={"hnsw:space": "cosine"},
        )
        log.info("ResolvedMemory ready (collection=%s, docs=%d)",
                 self.collection.name, self.collection.count())

    def store_batch(self, nodes: list[dict]) -> None:
        """Embed and upsert resolved nodes into ChromaDB."""
        if not nodes:
            return

        ids        = [str(n["id"]) for n in nodes]
        documents  = [n["fact"] for n in nodes]
        metadatas  = [
            {
                "type":        n.get("type", ""),
                "entities":    ",".join(n.get("entities", [])),
                "known_by":    ",".join(n.get("known_by", [])),
                "resolved_at": n.get("resolved_at", -1),
            }
            for n in nodes
        ]

        embeddings = self.model.encode(documents).tolist()
        self.collection.upsert(
            ids=ids,
            documents=documents,
            metadatas=metadatas,
            embeddings=embeddings,
        )
        log.info("Stored %d resolved node(s) in ChromaDB", len(nodes))

    def retrieve(self, context_json: str) -> str:
        """Query ChromaDB with recent dialogue sentences.

        Args:
            context_json: JSON string with at minimum a text body;
                          the scene_context passed from C++ Director.

        Returns:
            JSON array of relevant resolved fact strings.
        """
        if self.collection.count() == 0:
            return "[]"

        sentences = _split_sentences(context_json)
        if not sentences:
            return "[]"

        query_embeddings = self.model.encode(sentences).tolist()
        n_results = min(MAX_RESULTS_PER_QUERY, self.collection.count())
        results = self.collection.query(
            query_embeddings=query_embeddings,
            n_results=n_results,
        )

        seen: set[str] = set()
        output: list[str] = []
        total_chars = 0

        max_idx = max(len(docs) for docs in results["documents"])
        for i in range(max_idx):
            for q_idx in range(len(sentences)):
                docs = results["documents"][q_idx]
                dists = results["distances"][q_idx]
                if i >= len(docs):
                    continue
                doc  = docs[i]
                dist = dists[i]
                if dist > DISTANCE_THRESHOLD or doc in seen:
                    continue
                seen.add(doc)
                total_chars += len(doc)
                if total_chars > MAX_TOTAL_CHARS:
                    return json.dumps(output)
                output.append(doc)

        return json.dumps(output)
