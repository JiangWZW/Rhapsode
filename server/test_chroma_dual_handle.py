"""Minimal reproduction: duplicate Collection handles for the same collection.

Theory: When two different Python Collection objects reference the same
ChromaDB collection (via the same PersistentClient), and upserts go through
one handle then a different collection is modified, the HNSW reader for the
first collection becomes invalid for the other handle.
"""

import json
import math
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import MemorySystem, Scene
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model, _get_client, _make_chroma_callbacks, _make_embed,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# ====================================================================
# Test 1: TWO callback closures for MemorySystem (duplicate handles)
#         Then sync char memories, then query siege_nodes
# ====================================================================
print("\n=== Test 1: Duplicate siege_nodes handles + char_mem sync ===")

scene = Scene.load_json(SCENARIO)
if scene.has_save(SAVES_DIR):
    scene.load_save(SAVES_DIR)

# Create FIRST set of callbacks (simulates previous session)
memory_old = MemorySystem(scene.scene_id)
register_callbacks(memory_old, scene.scene_id, CHROMA_PATH)
memory_old.set_local_llm_callback(make_local_llm_callback())

# Store nodes through first set
for n in scene.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory_old.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  'Old session' stored {scene.world_graph.size()} nodes")

# Verify query works
try:
    ids = memory_old.search_nodes("what happened", 5)
    print(f"  'Old session' search: OK ({len(ids)} results)")
except Exception as e:
    print(f"  'Old session' search: FAILED -- {e}")

# Create SECOND set of callbacks (simulates new session, old not GC'd)
memory_new = MemorySystem(scene.scene_id)
register_callbacks(memory_new, scene.scene_id, CHROMA_PATH)
memory_new.set_local_llm_callback(make_local_llm_callback())

# Store nodes through second set (re-upsert same data)
for n in scene.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory_new.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  'New session' stored {scene.world_graph.size()} nodes")

# Now sync character memories (also through new callbacks)
for name, mem in scene.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# Query siege_nodes via the NEW callbacks
try:
    ids = memory_new.search_nodes("what happened", 5)
    print(f"  'New session' search: OK ({len(ids)} results)")
except Exception as e:
    print(f"  'New session' search: FAILED -- {e}")
    print("  >>> CONFIRMED: duplicate handles + char sync breaks siege_nodes <<<")

# ====================================================================
# Test 2: Same but WITHOUT the old session (control)
# ====================================================================
print("\n=== Test 2: Single handle + char_mem sync (control) ===")

scene2 = Scene.load_json(SCENARIO)
if scene2.has_save(SAVES_DIR):
    scene2.load_save(SAVES_DIR)

memory_single = MemorySystem(scene2.scene_id)
register_callbacks(memory_single, scene2.scene_id, CHROMA_PATH)
memory_single.set_local_llm_callback(make_local_llm_callback())

for n in scene2.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory_single.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

for name, mem in scene2.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

try:
    ids = memory_single.search_nodes("what happened", 5)
    print(f"  Single handle search: OK ({len(ids)} results)")
except Exception as e:
    print(f"  Single handle search: FAILED -- {e}")

# ====================================================================
# Test 3: Pure Python duplicate handles (no pybind11)
# ====================================================================
print("\n=== Test 3: Pure Python duplicate Collection handles ===")

client = _get_client(CHROMA_PATH)
from sentence_transformers import SentenceTransformer
model = SentenceTransformer("BAAI/bge-base-en-v1.5")

# Create two handles for siege_nodes
handle_a = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
handle_b = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})

# Upsert through handle_a
for i in range(12):
    emb = model.encode(f"test fact {i}").tolist()
    handle_a.upsert(ids=[f"node_{i+1}"], documents=[f"test fact {i}"],
                     embeddings=[emb], metadatas=[{"type": "test"}])

# Upsert through handle_b (same data, different handle)
for i in range(12):
    emb = model.encode(f"test fact {i}").tolist()
    handle_b.upsert(ids=[f"node_{i+1}"], documents=[f"test fact {i}"],
                     embeddings=[emb], metadatas=[{"type": "test"}])

# Upsert into char collection
char_col = client.get_or_create_collection("char_Sergeant_Maren", metadata={"hnsw:space": "cosine"})
for i in range(11):
    emb = model.encode(f"char memory {i}").tolist()
    char_col.upsert(ids=[f"mem_{i}"], documents=[f"char memory {i}"],
                     embeddings=[emb], metadatas=[{"type": 0}])

# Query siege_nodes through handle_b
q_emb = model.encode("what happened").tolist()
try:
    r = handle_b.query(query_embeddings=[q_emb], n_results=3,
                        where={"state": {"$ne": "dormant"}},
                        include=["documents"])
    print(f"  Pure Python dual-handle query: OK ({len(r['ids'][0])} results)")
except Exception as e:
    print(f"  Pure Python dual-handle query: FAILED -- {e}")

# ====================================================================
# Test 4: Duplicate handles via _make_chroma_callbacks closures
# ====================================================================
print("\n=== Test 4: Duplicate handles via callback closures ===")

store_a, query_a, _, _, _ = _make_chroma_callbacks(client)
store_b, query_b, _, _, _ = _make_chroma_callbacks(client)
embed = _make_embed()

# Store through closure A
for i in range(12):
    emb_json = embed(f"test fact {i}")
    meta = json.dumps({"node_id": i+1, "state": "active", "type": "world", "created_at": 0})
    store_a("siege_nodes", f"node_{i+1}", f"test fact {i}", emb_json, meta)

# Store through closure B (same collection)
for i in range(12):
    emb_json = embed(f"test fact {i}")
    meta = json.dumps({"node_id": i+1, "state": "active", "type": "world", "created_at": 0})
    store_b("siege_nodes", f"node_{i+1}", f"test fact {i}", emb_json, meta)

# Store char memories through closure B
for i in range(11):
    emb_json = embed(f"char memory {i}")
    meta = json.dumps({"type": 0, "created_at": 0, "weight": 5.0, "depth": 0})
    store_b("char_Sergeant_Maren", f"mem_{i}", f"char memory {i}", emb_json, meta)

# Query siege_nodes through closure B
q_emb_json = embed("what happened")
try:
    result = query_b("siege_nodes", q_emb_json, 5, json.dumps({"state": {"$ne": "dormant"}}))
    parsed = json.loads(result)
    print(f"  Closure-based dual-handle query: OK ({len(parsed['ids'][0])} results)")
except Exception as e:
    print(f"  Closure-based dual-handle query: FAILED -- {e}")

print("\nDone.")
