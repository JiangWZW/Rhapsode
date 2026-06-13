"""Test against EXISTING chroma data from real sessions.

The previous tests created fresh collections and everything worked.
Maybe the issue is with the pre-existing chroma state -- corrupt HNSW
segments from a previous session that was interrupted mid-write.
"""

import json
import os
import sys
import traceback

import chromadb

print(f"chromadb {chromadb.__version__}")

CHROMA_PATH = "./chroma"
if not os.path.exists(CHROMA_PATH):
    print(f"No chroma directory at {CHROMA_PATH}")
    sys.exit(1)

client = chromadb.PersistentClient(path=CHROMA_PATH)

# --- List all collections ---
print("\n=== Existing collections ===")
collections = client.list_collections()
for c in collections:
    print(f"  {c.name} (id={c.id})")

# --- Try count + query on each ---
print("\n=== Test: count + query on each collection ===")
from sentence_transformers import SentenceTransformer
model = SentenceTransformer("BAAI/bge-base-en-v1.5")

test_query = "what happened recently"
query_emb = model.encode(test_query).tolist()

for c in collections:
    col = client.get_or_create_collection(c.name, metadata={"hnsw:space": "cosine"})
    try:
        count = col.count()
        print(f"\n  [{c.name}] count = {count}")
        if count == 0:
            print(f"    (empty, skipping query)")
            continue

        n = min(count, 5)
        results = col.query(
            query_embeddings=[query_emb],
            n_results=n,
            include=["documents", "metadatas", "distances"],
        )
        docs = results["documents"][0]
        print(f"    query OK: {len(docs)} results")
        for d in docs[:2]:
            print(f"      - {d[:80]}...")
    except Exception as e:
        print(f"    FAILED: {e}")
        traceback.print_exc()

# --- Test: get a fresh handle and query again ---
print("\n=== Test: fresh handles ===")
for c in collections:
    col_fresh = client.get_or_create_collection(c.name, metadata={"hnsw:space": "cosine"})
    try:
        count = col_fresh.count()
        if count == 0:
            continue
        results = col_fresh.query(
            query_embeddings=[query_emb],
            n_results=min(count, 3),
            include=["documents"],
        )
        print(f"  [{c.name}] fresh handle query: OK")
    except Exception as e:
        print(f"  [{c.name}] fresh handle FAILED: {e}")

# --- Test: upsert then query (simulates sync_to_chroma + first-turn query) ---
print("\n=== Test: upsert-then-query cycle ===")
for c in collections:
    col = client.get_or_create_collection(c.name, metadata={"hnsw:space": "cosine"})
    count = col.count()
    if count == 0:
        continue

    # Get existing docs to re-upsert (simulates sync_to_chroma)
    try:
        existing = col.get(limit=min(count, 5), include=["documents", "embeddings", "metadatas"])
        if existing["ids"]:
            # Re-upsert the first doc (should be a no-op)
            col.upsert(
                ids=[existing["ids"][0]],
                documents=[existing["documents"][0]],
                embeddings=[existing["embeddings"][0]],
                metadatas=[existing["metadatas"][0]],
            )
            # Now query
            results = col.query(
                query_embeddings=[query_emb],
                n_results=min(count, 3),
                include=["documents"],
            )
            print(f"  [{c.name}] upsert+query cycle: OK")
    except Exception as e:
        print(f"  [{c.name}] FAILED: {e}")
        traceback.print_exc()

# --- Test: simulate the exact first-turn-to-second-turn pattern ---
print("\n=== Test: simulate turn 1 -> bg reflect -> turn 2 pattern ===")
import threading

for c in collections:
    col = client.get_or_create_collection(c.name, metadata={"hnsw:space": "cosine"})
    count = col.count()
    if count < 5:
        continue

    print(f"\n  [{c.name}] count={count}")

    # Turn 1: main thread queries (primes reader)
    try:
        r = col.query(query_embeddings=[query_emb], n_results=min(count, 5),
                       include=["documents", "metadatas", "distances"])
        print(f"    turn 1 query: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"    turn 1 query FAILED: {e}")
        continue

    # Turn 1: main thread upserts (speak/observe)
    new_emb = model.encode("a new memory from the current turn").tolist()
    col.upsert(
        ids=["turn1_test_mem"],
        documents=["test memory from turn 1 simulated speak"],
        embeddings=[new_emb],
        metadatas=[{"type": 2, "turn": 99, "weight": 8.0, "depth": 0}],
    )
    print(f"    turn 1 upsert: OK (count now {col.count()})")

    # Background: simulate reflect (query + multiple upserts)
    bg_err = None
    def bg_reflect():
        global bg_err
        try:
            # Evidence retrieval
            col.query(query_embeddings=[query_emb], n_results=min(col.count(), 10),
                       include=["documents", "metadatas", "distances"])
            # Insight upserts
            for j in range(4):
                ins_emb = model.encode(f"insight {j} about the character").tolist()
                col.upsert(
                    ids=[f"bg_insight_{j}"],
                    documents=[f"test insight {j}"],
                    embeddings=[ins_emb],
                    metadatas=[{"type": 1, "turn": 99, "weight": 9.0, "depth": 1}],
                )
            # Dedup query with where filter
            dedup_emb = model.encode("insight about the character").tolist()
            col.query(query_embeddings=[dedup_emb], n_results=3,
                       where={"type": 1}, include=["documents", "distances"])
            print(f"    [bg] reflect sim: OK (count now {col.count()})")
        except Exception as e:
            bg_err = e
            print(f"    [bg] reflect FAILED: {e}")

    t = threading.Thread(target=bg_reflect)
    t.start()
    t.join()

    # Turn 2: main thread queries
    try:
        r = col.query(query_embeddings=[query_emb], n_results=min(col.count(), 5),
                       include=["documents", "metadatas", "distances"])
        print(f"    turn 2 query: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"    turn 2 query FAILED: {e}")
        traceback.print_exc()

    # Clean up test docs
    try:
        col.delete(ids=["turn1_test_mem"] + [f"bg_insight_{j}" for j in range(4)])
    except:
        pass

print("\nDone.")
