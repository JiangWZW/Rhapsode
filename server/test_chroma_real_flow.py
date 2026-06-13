"""Test through the REAL C++ pipeline: Scene + MemorySystem + CharacterMemory.

Simulate two turns of the actual app lifecycle to reproduce the error.
"""

import json
import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import (
    Director, MemorySystem, Scene, SceneLoop,
    Validator, Weaver, analyze_graph,
)
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model, _get_client,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

# --- Step 0: Warm up ---
warmup_model()

# --- Step 1: Test the where-filter query that MemorySystem uses ---
print("\n=== Test A: MemorySystem where-filter queries ===")
client = _get_client(CHROMA_PATH)
col = client.get_or_create_collection("siege_nodes", metadata={"hnsw:space": "cosine"})
print(f"  siege_nodes count = {col.count()}")

from sentence_transformers import SentenceTransformer
model = SentenceTransformer("BAAI/bge-base-en-v1.5")

# This is the EXACT where filter used by MemorySystem::search_nodes
where_filter = {"state": {"$ne": "dormant"}}
query_emb = model.encode("Sergeant Maren what happened").tolist()

for i in range(3):
    try:
        r = col.query(
            query_embeddings=[query_emb],
            n_results=min(col.count(), 6),
            where=where_filter,
            include=["documents", "metadatas", "distances"],
        )
        print(f"  query {i+1} (with $ne filter): OK ({len(r['ids'][0])} results)")
    except Exception as e:
        print(f"  query {i+1} (with $ne filter): FAILED -- {e}")

# --- Step 2: Load scene + wire up the REAL pipeline ---
print("\n=== Test B: Full pipeline simulation ===")

scene = Scene.load_json(SCENARIO)
memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())
scene.set_memory(memory)

is_resuming = scene.has_save(SAVES_DIR)
if is_resuming:
    scene.load_save(SAVES_DIR)
    print(f"  Resumed: turn={scene.turn_index} graph={scene.world_graph.size()} hist={scene.history.size()}")
else:
    print(f"  Fresh start: graph={scene.world_graph.size()} hist={scene.history.size()}")

# Sync graph to memory (same as app startup)
all_nodes = scene.world_graph.all_nodes_including_expired()
for n in all_nodes:
    if n.id == 0:
        continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
expired = [n for n in all_nodes if n.valid_until != -1]
if expired:
    memory.sync_expired(expired)
print(f"  Synced {len(all_nodes)} graph nodes")

# Init character memories
for name, mem in scene.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()
    print(f"  CharMem '{name}' synced")

# --- Step 3: Test the exact query path from build_actor_prompt ---
print("\n=== Test C: MemorySystem.search_nodes() ===")

try:
    ids = memory.search_nodes("Sergeant Maren what happened", 5)
    print(f"  search_nodes: OK, {len(ids)} node IDs: {ids}")
except Exception as e:
    print(f"  search_nodes FAILED: {e}")
    traceback.print_exc()

# Test character memory retrieval
print("\n=== Test D: CharacterMemory.retrieve() + briefing() ===")
for name, mem in scene.character_memories.items():
    try:
        r = mem.retrieve("what happened recently", 5)
        print(f"  {name} retrieve: OK ({len(r)} chars)")
    except Exception as e:
        print(f"  {name} retrieve FAILED: {e}")
        traceback.print_exc()

    try:
        b = mem.briefing("current situation", 5)
        print(f"  {name} briefing: OK ({len(b)} chars)")
    except Exception as e:
        print(f"  {name} briefing FAILED: {e}")
        traceback.print_exc()

# --- Step 4: Simulate the post-turn mutations + next-turn query ---
print("\n=== Test E: Simulate post-turn mutations ===")

# memory.process_new_nodes stores new facts
# Let's simulate by storing a fake node
print("  Storing fake new node...")
memory.store_node(999, "A test fact for debugging", "active", "scene", scene.turn_index)

# memory.sync_expired updates metadata
# NOTE: sync_expired uses update() which REPLACES metadata entirely!
# Check if this corrupts the collection

print("  Running search_nodes AFTER store_node...")
try:
    ids = memory.search_nodes("test fact debugging", 5)
    print(f"  search_nodes: OK, {len(ids)} node IDs")
except Exception as e:
    print(f"  search_nodes FAILED: {e}")
    traceback.print_exc()

# Clean up fake node
try:
    memory.delete_nodes([999])
    print("  Cleaned up fake node")
except:
    pass

# --- Step 5: Simulate bg reflect + main-thread query ---
print("\n=== Test F: Background reflect + main-thread query ===")
import concurrent.futures

for name, mem in scene.character_memories.items():
    if not mem.needs_reflection():
        print(f"  {name}: countdown > 0, skipping")
        continue

    print(f"  {name}: running reflect in background...")
    with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
        future = pool.submit(mem.reflect)
        future.result()
    print(f"  {name}: reflect done")

    # Now test main-thread queries on BOTH collections
    try:
        ids = memory.search_nodes("Sergeant Maren strategy", 5)
        print(f"  search_nodes (siege_nodes): OK, {len(ids)} results")
    except Exception as e:
        print(f"  search_nodes (siege_nodes): FAILED -- {e}")
        traceback.print_exc()

    try:
        r = mem.retrieve("what happened", 5)
        print(f"  {name} retrieve: OK ({len(r)} chars)")
    except Exception as e:
        print(f"  {name} retrieve FAILED: {e}")
        traceback.print_exc()

print("\nDone.")
