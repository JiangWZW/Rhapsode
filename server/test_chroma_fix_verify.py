"""Verify the fix: sync graph AFTER character memory init."""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import MemorySystem, Scene
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# ====================================================================
# Test: Current order (FAILS) -- sync_graph THEN init_char_memories
# ====================================================================
print("\n=== Current order: sync_graph -> char_mem_sync -> search ===")

scene = Scene.load_json(SCENARIO)
if scene.has_save(SAVES_DIR):
    scene.load_save(SAVES_DIR)

memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())
scene.set_memory(memory)

# 1. sync graph to memory FIRST
for n in scene.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

# 2. init character memories SECOND
for name, mem in scene.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# 3. test
try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  Result: OK ({len(ids)} results)")
except Exception as e:
    print(f"  Result: FAILED -- {e}")

# ====================================================================
# Test: Fixed order -- init_char_memories THEN sync_graph
# ====================================================================
print("\n=== Fixed order: char_mem_sync -> sync_graph -> search ===")

scene2 = Scene.load_json(SCENARIO)
if scene2.has_save(SAVES_DIR):
    scene2.load_save(SAVES_DIR)

memory2 = MemorySystem(scene2.scene_id)
register_callbacks(memory2, scene2.scene_id, CHROMA_PATH)
memory2.set_local_llm_callback(make_local_llm_callback())
scene2.set_memory(memory2)

# 1. init character memories FIRST
for name, mem in scene2.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# 2. sync graph to memory SECOND
for n in scene2.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory2.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

# 3. test
try:
    ids = memory2.search_nodes("what happened", 5)
    print(f"  Result: OK ({len(ids)} results)")
except Exception as e:
    print(f"  Result: FAILED -- {e}")

# Also test char memory queries still work
for name, mem in scene2.character_memories.items():
    try:
        r = mem.retrieve("what happened", 3)
        print(f"  {name} retrieve: OK ({len(r)} chars)")
    except Exception as e:
        print(f"  {name} retrieve: FAILED -- {e}")

# ====================================================================
# Test: Same order as current, but do a "dummy" store_node at the end
# ====================================================================
print("\n=== Alt fix: current order + dummy re-store at end ===")

scene3 = Scene.load_json(SCENARIO)
if scene3.has_save(SAVES_DIR):
    scene3.load_save(SAVES_DIR)

memory3 = MemorySystem(scene3.scene_id)
register_callbacks(memory3, scene3.scene_id, CHROMA_PATH)
memory3.set_local_llm_callback(make_local_llm_callback())
scene3.set_memory(memory3)

# 1. sync graph FIRST (current order)
all_nodes = scene3.world_graph.all_nodes_including_expired()
for n in all_nodes:
    if n.id == 0: continue
    memory3.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

# 2. init char memories SECOND (current order)
for name, mem in scene3.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# 3. "Touch" siege_nodes by re-storing the first node
first_node = next((n for n in all_nodes if n.id != 0), None)
if first_node:
    memory3.store_node(first_node.id, first_node.fact,
                       first_node.state.name.lower(), first_node.type, first_node.created_at)
    print("  Re-stored first node as 'touch'")

# 4. test
try:
    ids = memory3.search_nodes("what happened", 5)
    print(f"  Result: OK ({len(ids)} results)")
except Exception as e:
    print(f"  Result: FAILED -- {e}")

print("\nDone.")
