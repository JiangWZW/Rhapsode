"""Narrow down: which step in the pipeline setup corrupts siege_nodes?

From test_chroma_real_flow.py we know:
  - After pipeline setup: search_nodes FAILS
  - After one more store_node: search_nodes WORKS
  - Character collections always work

Suspects:
  A) The 12 rapid store_node() calls from _sync_graph_to_memory
  B) The sync_expired() call
  C) The character memory sync_to_chroma() calls
  D) Having multiple Collection handles via different callback closures
"""

import json
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import MemorySystem, Scene
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model, _get_client, _make_chroma_callbacks,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# ===========================================================================
# Test 1: Just store_node + search_nodes (no character memories)
# ===========================================================================
print("\n=== Test 1: store_node + search_nodes only ===")

scene = Scene.load_json(SCENARIO)
if scene.has_save(SAVES_DIR):
    scene.load_save(SAVES_DIR)

memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())

all_nodes = scene.world_graph.all_nodes_including_expired()
expired = [n for n in all_nodes if n.valid_until != -1]
print(f"  {len(all_nodes)} nodes, {len(expired)} expired")

for n in all_nodes:
    if n.id == 0:
        continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored {len([n for n in all_nodes if n.id != 0])} nodes")

if expired:
    memory.sync_expired(expired)
    print(f"  sync_expired: {len(expired)} nodes")

try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  search_nodes: OK, {len(ids)} results")
except Exception as e:
    print(f"  search_nodes: FAILED -- {e}")

# ===========================================================================
# Test 2: Same but with character memory init too
# ===========================================================================
print("\n=== Test 2: store_node + char_mem sync + search_nodes ===")

# Need to recreate because the callbacks are already set
scene2 = Scene.load_json(SCENARIO)
if scene2.has_save(SAVES_DIR):
    scene2.load_save(SAVES_DIR)

memory2 = MemorySystem(scene2.scene_id)
register_callbacks(memory2, scene2.scene_id, CHROMA_PATH)
memory2.set_local_llm_callback(make_local_llm_callback())

all_nodes2 = scene2.world_graph.all_nodes_including_expired()
for n in all_nodes2:
    if n.id == 0:
        continue
    memory2.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored {len([n for n in all_nodes2 if n.id != 0])} nodes")

# NOW init character memories
for name, mem in scene2.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()
    print(f"  CharMem '{name}' synced")

try:
    ids = memory2.search_nodes("what happened", 5)
    print(f"  search_nodes: OK, {len(ids)} results")
except Exception as e:
    print(f"  search_nodes: FAILED -- {e}")

# ===========================================================================
# Test 3: What if we DON'T store nodes first, just query existing?
# ===========================================================================
print("\n=== Test 3: NO store, just register callbacks + search ===")

scene3 = Scene.load_json(SCENARIO)
if scene3.has_save(SAVES_DIR):
    scene3.load_save(SAVES_DIR)

memory3 = MemorySystem(scene3.scene_id)
register_callbacks(memory3, scene3.scene_id, CHROMA_PATH)
memory3.set_local_llm_callback(make_local_llm_callback())

# Skip store_node, skip char_mem init
try:
    ids = memory3.search_nodes("what happened", 5)
    print(f"  search_nodes (no prior store): OK, {len(ids)} results")
except Exception as e:
    print(f"  search_nodes (no prior store): FAILED -- {e}")

# ===========================================================================
# Test 4: Store nodes, then query, store more, then query again
# ===========================================================================
print("\n=== Test 4: Incremental store + query pattern ===")

scene4 = Scene.load_json(SCENARIO)
if scene4.has_save(SAVES_DIR):
    scene4.load_save(SAVES_DIR)

memory4 = MemorySystem(scene4.scene_id)
register_callbacks(memory4, scene4.scene_id, CHROMA_PATH)
memory4.set_local_llm_callback(make_local_llm_callback())

all_nodes4 = scene4.world_graph.all_nodes_including_expired()
nodes_to_store = [n for n in all_nodes4 if n.id != 0]

# Store first half
half = len(nodes_to_store) // 2
for n in nodes_to_store[:half]:
    memory4.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored first {half} nodes")

try:
    ids = memory4.search_nodes("what happened", 3)
    print(f"  search after first half: OK, {len(ids)} results")
except Exception as e:
    print(f"  search after first half: FAILED -- {e}")

# Store second half
for n in nodes_to_store[half:]:
    memory4.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored remaining {len(nodes_to_store) - half} nodes")

try:
    ids = memory4.search_nodes("what happened", 3)
    print(f"  search after all: OK, {len(ids)} results")
except Exception as e:
    print(f"  search after all: FAILED -- {e}")

# ===========================================================================
# Test 5: Does creating a second PersistentClient matter?
# ===========================================================================
print("\n=== Test 5: Check PersistentClient singleton behavior ===")

# _get_client returns a singleton. How many times was it created?
client = _get_client(CHROMA_PATH)
print(f"  client id: {id(client)}")
print(f"  collections: {[c.name for c in client.list_collections()]}")

# Query siege_nodes directly through the client
col = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
print(f"  siege_nodes count: {col.count()}")
try:
    from sentence_transformers import SentenceTransformer
    model = SentenceTransformer("BAAI/bge-base-en-v1.5")
    q = model.encode("what happened").tolist()
    r = col.query(query_embeddings=[q], n_results=3,
                   where={"state": {"$ne": "dormant"}},
                   include=["documents", "metadatas", "distances"])
    print(f"  direct query: OK ({len(r['ids'][0])} results)")
except Exception as e:
    print(f"  direct query: FAILED -- {e}")

print("\nDone.")
