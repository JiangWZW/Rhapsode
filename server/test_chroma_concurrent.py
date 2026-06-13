"""Reproduce the ACTUAL race condition in the real app.

The real app has concurrent ChromaDB writes:
  - Main thread (asyncio): memory.sync_expired() + process_new_nodes() on siege_nodes
  - std::async thread: reflect() on char_* collections

Both go through the same PersistentClient simultaneously.
"""

import json
import math
import os
import sys
import threading
import time
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import MemorySystem, Scene
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model, _get_client,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

scene = Scene.load_json(SCENARIO)
if scene.has_save(SAVES_DIR):
    scene.load_save(SAVES_DIR)

memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())
scene.set_memory(memory)

for n in scene.world_graph.all_nodes_including_expired():
    if n.id == 0: continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)

for name, mem in scene.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

print("Setup complete.\n")

# === Simulate the exact concurrent pattern ===

# Step 1: Main thread queries siege_nodes (like advance() Phase 1 + Phase 4)
print("=== Step 1: Main thread queries siege_nodes ===")
try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  search_nodes: OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes: FAILED -- {e}")

# Step 2: simulate dispatch_background() + concurrent main-thread writes
print("\n=== Step 2: Concurrent bg reflect + main-thread siege_nodes writes ===")

bg_error = None
bg_done = threading.Event()

def background_reflect():
    """Simulates the std::async thread running reflect()."""
    global bg_error
    try:
        for name, mem in scene.character_memories.items():
            if mem.needs_reflection():
                print(f"  [bg] reflecting {name}...", flush=True)
                mem.reflect()
                print(f"  [bg] {name} reflect done", flush=True)
            else:
                # Even without reflection, simulate some char_mem queries
                try:
                    r = mem.retrieve("recent events", 5)
                    print(f"  [bg] {name} retrieve: {len(r)} chars", flush=True)
                except Exception as e:
                    print(f"  [bg] {name} retrieve failed: {e}", flush=True)
    except Exception as e:
        bg_error = e
        print(f"  [bg] ERROR: {e}", flush=True)
    finally:
        bg_done.set()

# Start background thread (simulates dispatch_background's std::async)
bg_thread = threading.Thread(target=background_reflect, daemon=True)
bg_thread.start()

# Main thread immediately does post-turn processing (concurrent with bg)
# This is what happens in app.py after run_in_executor returns
print("  [main] Doing post-turn processing (concurrent with bg)...", flush=True)

# Simulate memory.process_new_nodes - upsert into siege_nodes
memory.store_node(999, "A test new node from the director", "active", "scene", scene.turn_index)
print("  [main] Stored new node into siege_nodes", flush=True)

# Simulate memory.sync_expired - update metadata in siege_nodes
# (In real app this calls update_meta which does col.update())
# For now just do another store
memory.store_node(998, "Another concurrent write", "foreshadowed", "plot", scene.turn_index)
print("  [main] Stored another node into siege_nodes", flush=True)

# Wait for background to finish (simulates join_background at start of next turn)
print("  [main] Waiting for background...", flush=True)
bg_thread.join()
print(f"  [main] Background done. Error: {bg_error}", flush=True)

# Step 3: Query siege_nodes (simulates next turn's Phase 1)
print("\n=== Step 3: Query siege_nodes after concurrent ops ===")
try:
    ids = memory.search_nodes("what happened", 5)
    print(f"  search_nodes: OK ({len(ids)} results)")
except Exception as e:
    print(f"  search_nodes: FAILED -- {e}")
    print("  >>> RACE CONDITION CONFIRMED <<<")

# Cleanup
try:
    memory.delete_nodes([999, 998])
except:
    pass

# === Run multiple rounds to increase chance of triggering ===
print("\n=== Multi-round race test ===")
for round_num in range(1, 6):
    print(f"\n--- Round {round_num} ---")

    # Pre-query (like advance Phase 1)
    try:
        ids = memory.search_nodes("events", 3)
        print(f"  pre-query: OK ({len(ids)} results)")
    except Exception as e:
        print(f"  pre-query: FAILED -- {e}")

    # Launch background work
    bg_err2 = None
    def bg_work(rn=round_num):
        global bg_err2
        try:
            for name, mem in scene.character_memories.items():
                _ = mem.retrieve("what is happening", 3)
                mem.observe(f"Round {rn} observation", scene.turn_index)
                mem.speak(f"Round {rn} dialogue line", scene.turn_index)
        except Exception as e:
            bg_err2 = e

    t = threading.Thread(target=bg_work)
    t.start()

    # Concurrent main-thread writes to siege_nodes
    memory.store_node(900 + round_num, f"concurrent node {round_num}", "active", "plot", scene.turn_index)

    t.join()

    # Post-query (like next turn's Phase 1)
    try:
        ids = memory.search_nodes("events", 3)
        print(f"  post-query: OK ({len(ids)} results)")
    except Exception as e:
        print(f"  post-query: FAILED -- {e}")
        print("  >>> RACE CONDITION TRIGGERED <<<")

    # Cleanup
    try:
        memory.delete_nodes([900 + round_num])
    except:
        pass

print("\nDone.")
