"""Focused test on the siege_nodes collection which shows HNSW errors.

Previous test showed:
  - First query on siege_nodes: OK
  - All subsequent queries: FAILED with "Nothing found on disk"
  - All character collections: always OK

This test checks the actual failure pattern across repeated queries.
"""

import sys
import traceback
import chromadb

print(f"chromadb {chromadb.__version__}")

client = chromadb.PersistentClient(path="./chroma")

from sentence_transformers import SentenceTransformer
model = SentenceTransformer("BAAI/bge-base-en-v1.5")
query_emb = model.encode("what happened recently").tolist()

# --- Repeated queries on siege_nodes ---
print("\n=== Repeated queries on siege_nodes (same handle) ===")
col = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
print(f"  count = {col.count()}")

for i in range(5):
    try:
        r = col.query(query_embeddings=[query_emb], n_results=3,
                       include=["documents", "metadatas", "distances"])
        print(f"  query {i+1}: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"  query {i+1}: FAILED -- {e}")

# --- Fresh handle each time ---
print("\n=== Fresh handle per query on siege_nodes ===")
for i in range(5):
    try:
        c = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
        r = c.query(query_embeddings=[query_emb], n_results=3,
                     include=["documents", "metadatas", "distances"])
        print(f"  query {i+1}: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"  query {i+1}: FAILED -- {e}")

# --- Contrast with a character collection ---
print("\n=== Repeated queries on char_Sergeant_Maren (same handle) ===")
col2 = client.get_or_create_collection("char_Sergeant_Maren", metadata={"hnsw:space": "cosine"})
print(f"  count = {col2.count()}")

for i in range(5):
    try:
        r = col2.query(query_embeddings=[query_emb], n_results=3,
                        include=["documents", "metadatas", "distances"])
        print(f"  query {i+1}: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"  query {i+1}: FAILED -- {e}")

# --- Check what data siege_nodes actually has ---
print("\n=== siege_nodes: peek at stored data ===")
try:
    data = col.peek(5)
    print(f"  peek: {len(data['ids'])} docs")
    for i, (doc_id, doc) in enumerate(zip(data["ids"], data["documents"])):
        print(f"    {doc_id}: {doc[:80]}...")
except Exception as e:
    print(f"  peek FAILED: {e}")

# --- Check: does col.get() without embeddings work? ---
print("\n=== siege_nodes: get() without embeddings ===")
try:
    data = col.get(include=["documents", "metadatas"])
    print(f"  get (no embeddings): OK, {len(data['ids'])} docs")
    for doc_id, doc in zip(data["ids"], data["documents"]):
        print(f"    {doc_id}: {doc[:80]}...")
except Exception as e:
    print(f"  get (no embeddings) FAILED: {e}")

# --- Check: does col.get() WITH embeddings work? ---
print("\n=== siege_nodes: get() WITH embeddings ===")
try:
    data = col.get(include=["documents", "embeddings"])
    print(f"  get (with embeddings): OK, {len(data['ids'])} docs")
except Exception as e:
    print(f"  get (with embeddings) FAILED: {e}")

# --- Nuclear option: delete and recreate siege_nodes ---
print("\n=== Would deleting + recreating siege_nodes fix it? ===")
print("  (not actually doing it, just checking if we can)")

# Check what the MemorySystem stores
print("\n=== siege_nodes: metadata inspection ===")
try:
    data = col.get(include=["metadatas"])
    print(f"  {len(data['ids'])} documents")
    for doc_id, meta in zip(data["ids"], data["metadatas"]):
        print(f"    {doc_id}: {meta}")
except Exception as e:
    print(f"  metadata get FAILED: {e}")

# --- New client (simulates server restart) ---
print("\n=== New PersistentClient + query siege_nodes ===")
client2 = chromadb.PersistentClient(path="./chroma")
col3 = client2.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
print(f"  count = {col3.count()}")
for i in range(3):
    try:
        r = col3.query(query_embeddings=[query_emb], n_results=3,
                        include=["documents", "metadatas", "distances"])
        print(f"  query {i+1}: OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"  query {i+1}: FAILED -- {e}")

print("\nDone.")
