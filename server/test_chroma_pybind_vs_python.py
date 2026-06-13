"""Definitive test: pybind11 callback path vs pure Python path.

From narrowing tests:
  - Pure Python upserts across collections: WORKS
  - C++ sync_to_chroma() via pybind11, then C++ search_nodes(): FAILS
  
This test isolates whether the failure is caused by the pybind11 callback layer.
"""

import json
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from sentence_transformers import SentenceTransformer
from rhapsode._core import MemorySystem, Scene, CharacterMemory as CppCharMem
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model, _get_client, _make_embed, _make_chroma_callbacks,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# ====================================================================
# Test A: Pure Python upserts to char collections, then C++ search_nodes
# ====================================================================
print("\n=== Test A: Python upserts + C++ search_nodes ===")

scene = Scene.load_json(SCENARIO)
if scene.has_save(SAVES_DIR):
    scene.load_save(SAVES_DIR)

memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())

# Store graph nodes (via C++)
for n in scene.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored graph nodes via C++")

# Verify query works BEFORE char memory ops
try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  search_nodes (before char ops): OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes (before char ops): FAILED -- {e}")

# Now do character memory upserts from PURE PYTHON (not via C++ sync_to_chroma)
client = _get_client(CHROMA_PATH)
model = SentenceTransformer("BAAI/bge-base-en-v1.5")

for name, mem in scene.character_memories.items():
    col = client.get_or_create_collection(
        f"char_{'_'.join(c if c.isalnum() else '_' for c in name)}",
        metadata={"hnsw:space": "cosine"})
    # simulate sync_to_chroma: upsert all memories
    # (We can't easily iterate C++ memories, so just do a few test upserts)
    for i in range(10):
        text = f"test memory {i} for {name}"
        emb = model.encode(text).tolist()
        col.upsert(ids=[f"pymem_{i}"], documents=[text],
                   embeddings=[emb], metadatas=[{"type": 0}])
    print(f"  Python upserted 10 docs into {col.name}")

# Verify query works AFTER pure Python char upserts
try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  search_nodes (after Python char upserts): OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes (after Python char upserts): FAILED -- {e}")

# Clean up test docs
for name in scene.character_memories:
    col = client.get_or_create_collection(
        f"char_{'_'.join(c if c.isalnum() else '_' for c in name)}",
        metadata={"hnsw:space": "cosine"})
    try:
        col.delete(ids=[f"pymem_{i}" for i in range(10)])
    except:
        pass

# ====================================================================
# Test B: C++ sync_to_chroma via pybind11, then C++ search_nodes
# ====================================================================
print("\n=== Test B: C++ sync_to_chroma + C++ search_nodes ===")

scene2 = Scene.load_json(SCENARIO)
if scene2.has_save(SAVES_DIR):
    scene2.load_save(SAVES_DIR)

memory2 = MemorySystem(scene2.scene_id)
register_callbacks(memory2, scene2.scene_id, CHROMA_PATH)
memory2.set_local_llm_callback(make_local_llm_callback())

for n in scene2.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory2.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
print(f"  Stored graph nodes via C++")

try:
    ids = memory2.search_nodes("what happened", 5)
    print(f"  search_nodes (before C++ char sync): OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes (before C++ char sync): FAILED -- {e}")

# Now do character memory upserts via C++ sync_to_chroma (pybind11)
for name, mem in scene2.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()
    print(f"  C++ synced '{name}'")

try:
    ids = memory2.search_nodes("what happened", 5)
    print(f"  search_nodes (after C++ char sync): OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes (after C++ char sync): FAILED -- {e}")
    
    # Does querying siege_nodes directly from Python still work?
    try:
        col = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
        r = col.query(query_embeddings=[model.encode("what happened").tolist()],
                       n_results=3, include=["documents"])
        print(f"    ...but direct Python query: OK ({len(r['ids'][0])} results)")
    except Exception as e2:
        print(f"    ...direct Python query also FAILED: {e2}")

    # Does storing one more node fix it?
    memory2.store_node(9999, "a fix attempt", "active", "scene", 99)
    try:
        ids = memory2.search_nodes("what happened", 5)
        print(f"    ...after one more store_node: OK ({len(ids)} results)")
    except Exception as e2:
        print(f"    ...after one more store_node: STILL FAILED -- {e2}")
    try:
        memory2.delete_nodes([9999])
    except:
        pass

# ====================================================================
# Test C: C++ sync_to_chroma, then Python query on siege_nodes
# ====================================================================
print("\n=== Test C: C++ sync_to_chroma + Python query on siege_nodes ===")

scene3 = Scene.load_json(SCENARIO)
if scene3.has_save(SAVES_DIR):
    scene3.load_save(SAVES_DIR)

memory3 = MemorySystem(scene3.scene_id)
register_callbacks(memory3, scene3.scene_id, CHROMA_PATH)
memory3.set_local_llm_callback(make_local_llm_callback())

for n in scene3.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory3.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

for name, mem in scene3.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# Query siege_nodes DIRECTLY from Python (bypass C++ search_nodes)
try:
    col = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
    emb_json = json.dumps(model.encode("what happened").tolist())
    where = {"state": {"$ne": "dormant"}}
    r = col.query(query_embeddings=[json.loads(emb_json)],
                   n_results=min(col.count(), 5),
                   where=where,
                   include=["documents", "metadatas", "distances"])
    print(f"  Direct Python query on siege_nodes: OK ({len(r['ids'][0])} results)")
except Exception as e:
    print(f"  Direct Python query on siege_nodes: FAILED -- {e}")

# Now try C++ search_nodes
try:
    ids = memory3.search_nodes("what happened", 5)
    print(f"  C++ search_nodes: OK ({len(ids)} results)")
except Exception as e:
    print(f"  C++ search_nodes: FAILED -- {e}")

print("\nDone.")
