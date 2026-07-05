"""Reproduce using the ACTUAL SceneLoop with mocked LLM, simulating two turns.

This matches the real app's C++ std::async background thread path.
"""

import json
import os
import sys
import time
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb
from rhapsode._core import (
    Director, MemorySystem, Scene, SceneLoop,
    Weaver,
)
from rhapsode.memory import (
    register_callbacks,
    warmup_model,
)
from rhapsode.validator import make_local_llm_callback

print(f"chromadb {chromadb.__version__}")

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# --- Stub LLM that returns minimal valid responses ---
TURN_COUNT = 0

def stub_llm(prompt: str) -> str:
    """Minimal LLM response for actor synthesis."""
    return '"I have nothing to say right now."'

def stub_narrator_llm(instructions: str, turn_state: str) -> str:
    """Minimal narrator response with valid JSON plan."""
    global TURN_COUNT
    TURN_COUNT += 1
    prose = f"Test narration for turn {TURN_COUNT}. The courtyard is quiet."
    plan = json.dumps({
        "transitions": [],
        "new_nodes": [],
        "speech_turns": [
            {"character": "Sergeant Maren", "cue": "react to the quiet",
             "dramatic_intent": "reflect", "emotional_state": "watchful",
             "responds_to": "the silence"}
        ],
        "new_characters": [],
        "active_cast": ["Sergeant Maren"],
    })
    return prose + "\n<<<RHAPSODE_JSON>>>\n" + plan


# --- Init exactly as the real app does ---
scene = Scene.load_json(SCENARIO)
memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
scene.set_memory(memory)

is_resuming = scene.has_save(SAVES_DIR)
if is_resuming:
    scene.load_save(SAVES_DIR)
    print(f"Resumed: turn={scene.turn_index} graph={scene.world_graph.size()} hist={scene.history.size()}")
else:
    print(f"Fresh: graph={scene.world_graph.size()} hist={scene.history.size()}")

# Sync graph to memory
all_nodes = scene.world_graph.all_nodes_including_expired()
for n in all_nodes:
    if n.id == 0: continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
expired = [n for n in all_nodes if n.valid_until != -1]
if expired:
    memory.sync_expired(expired)
print(f"Synced {len(all_nodes)} graph nodes")

# Init character memories
for name, mem in scene.character_memories.items():
    mem.set_reflection_llm_callback(make_local_llm_callback())
print("Character memories initialized")

# --- Wire up SceneLoop exactly as _wire_loop does ---
director = Director(scene.world_graph)

weaver = Weaver(scene.world_graph)
weaver.set_llm_callback(stub_llm)  # use stub for weaver too
weaver.set_local_llm_callback(make_local_llm_callback())

loop = SceneLoop()
loop.load_scene(scene)
loop.set_director(director)

scene.downsampler.set_llm_callback(make_local_llm_callback())

loop.set_narrator_llm_callback(stub_narrator_llm)
loop.set_llm_callback(stub_llm)
loop.set_weaver(weaver)
loop.set_saves_dir(SAVES_DIR)

if is_resuming:
    loop.set_resuming(True)

# --- Submit turns ---
print("\n=== Turn 1 ===")
try:
    loop.submit_input("I look around the courtyard.")
    outputs = loop.take_last_turn_outputs()
    print(f"  Turn 1 OK: {len(outputs)} output(s)")
    for o in outputs:
        kind = o.metadata.get("scene_kind", "?")
        print(f"    [{kind}] {o.content[:80]}...")
except Exception as e:
    print(f"  Turn 1 FAILED: {e}")
    traceback.print_exc()

print(f"\n  (waiting 2s for background to start...)")
time.sleep(2)

print("\n=== Turn 2 ===")
try:
    loop.submit_input("I check the eastern gate.")
    outputs = loop.take_last_turn_outputs()
    print(f"  Turn 2 OK: {len(outputs)} output(s)")
    for o in outputs:
        kind = o.metadata.get("scene_kind", "?")
        print(f"    [{kind}] {o.content[:80]}...")
except Exception as e:
    print(f"  Turn 2 FAILED: {e}")
    traceback.print_exc()

print("\n=== Turn 3 ===")
try:
    loop.submit_input("I speak with Maren.")
    outputs = loop.take_last_turn_outputs()
    print(f"  Turn 3 OK: {len(outputs)} output(s)")
except Exception as e:
    print(f"  Turn 3 FAILED: {e}")

print("\n=== Turn 4 ===")
try:
    loop.submit_input("I look at the sky.")
    outputs = loop.take_last_turn_outputs()
    print(f"  Turn 4 OK: {len(outputs)} output(s)")
except Exception as e:
    print(f"  Turn 4 FAILED: {e}")

loop.join_background()
print("\nDone.")
