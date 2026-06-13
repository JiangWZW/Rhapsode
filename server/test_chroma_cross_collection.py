"""Minimal reproduction: does upserting into collection B break queries on collection A?

Theory: ChromaDB 1.5.9 Rust backend invalidates HNSW segment readers for ALL
collections when ANY collection is modified through the same PersistentClient.
"""

import math
import os
import shutil
import sys
import tempfile

import chromadb

print(f"chromadb {chromadb.__version__}")

DIM = 768
def emb(seed: float):
    return [math.sin(seed + i * 0.01) for i in range(DIM)]

# ---- Test with a FRESH chroma directory ----
TEST_DIR = os.path.join(tempfile.gettempdir(), "rhapsode_cross_col_test")
os.makedirs(TEST_DIR, exist_ok=True)

print("\n=== Fresh directory test ===")
client = chromadb.PersistentClient(path=TEST_DIR)

# Create collection A with data
col_a = client.get_or_create_collection("collection-aaa", metadata={"hnsw:space": "cosine"})
for i in range(10):
    col_a.upsert(ids=[f"a_{i}"], documents=[f"doc A {i}"],
                 embeddings=[emb(float(i))], metadatas=[{"type": "world"}])
print(f"  col A count: {col_a.count()}")

# Query A - should work
r = col_a.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
print(f"  query A (before B upserts): OK ({len(r['ids'][0])} results)")

# Create collection B and upsert
col_b = client.get_or_create_collection("collection-bbb", metadata={"hnsw:space": "cosine"})
for i in range(15):
    col_b.upsert(ids=[f"b_{i}"], documents=[f"doc B {i}"],
                 embeddings=[emb(float(i + 100))], metadatas=[{"type": "char"}])
print(f"  col B count: {col_b.count()}")

# Query A again - does this fail?
try:
    r = col_a.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query A (after B upserts): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query A (after B upserts): FAILED -- {e}")

# ---- Test with the REAL chroma directory ----
print("\n=== Real chroma directory test ===")
real_client = chromadb.PersistentClient(path="./chroma")

collections = real_client.list_collections()
print(f"  collections: {[c.name for c in collections]}")

# Query siege_nodes first
siege = real_client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
print(f"  siege_nodes count: {siege.count()}")

try:
    r = siege.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query siege_nodes (before any char upserts): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query siege_nodes (before any char upserts): FAILED -- {e}")

# Now upsert into char_Sergeant_Maren
maren = real_client.get_or_create_collection("char_Sergeant_Maren", metadata={"hnsw:space": "cosine"})
print(f"  char_Sergeant_Maren count before: {maren.count()}")
maren.upsert(ids=["test_doc_1"], documents=["test upsert"],
             embeddings=[emb(999.0)], metadatas=[{"type": 0}])
print(f"  char_Sergeant_Maren count after upsert: {maren.count()}")

# Query siege_nodes again
try:
    r = siege.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query siege_nodes (after Maren upsert): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query siege_nodes (after Maren upsert): FAILED -- {e}")
    print("  >>> CONFIRMED: cross-collection corruption <<<")

# Try with fresh handle
try:
    siege2 = real_client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
    r = siege2.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query siege_nodes (fresh handle): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query siege_nodes (fresh handle): FAILED -- {e}")

# Cleanup test doc
try:
    maren.delete(ids=["test_doc_1"])
except:
    pass

# ---- Test 3: Does it happen with ANY two collections? ----
print("\n=== Control: upsert into NEW collection, then query siege_nodes ===")
real_client2 = chromadb.PersistentClient(path="./chroma")
siege3 = real_client2.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})

try:
    r = siege3.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query siege_nodes (fresh client): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query siege_nodes (fresh client): FAILED -- {e}")

# Create a brand new collection
new_col = real_client2.get_or_create_collection("temp-test-col", metadata={"hnsw:space": "cosine"})
for i in range(5):
    new_col.upsert(ids=[f"t_{i}"], documents=[f"temp {i}"],
                   embeddings=[emb(float(i + 500))], metadatas=[{"x": 1}])
print(f"  temp-test-col count: {new_col.count()}")

try:
    r = siege3.query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
    print(f"  query siege_nodes (after new col upserts): OK ({len(r['ids'][0])} results)")
except chromadb.errors.InternalError as e:
    print(f"  query siege_nodes (after new col upserts): FAILED -- {e}")

# Cleanup
try:
    real_client2.delete_collection("temp-test-col")
except:
    pass

print("\nDone.")
