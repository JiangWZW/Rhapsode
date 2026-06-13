"""Add diagnostic logging to the query callback and run a real session.

Instead of guessing, let's instrument the actual callbacks to see exactly
what collection is being queried, from which thread, and when it fails.
"""

import json
import logging
import os
import sys
import threading
import traceback

sys.path.insert(0, os.path.dirname(__file__))

import chromadb

# Monkey-patch _make_chroma_callbacks to add detailed logging
import rhapsode.memory as mem_module

_original_make_chroma = mem_module._make_chroma_callbacks
_call_counter = 0
_call_lock = threading.Lock()

def _instrumented_make_chroma(client: chromadb.ClientAPI):
    """Wrapper that logs every ChromaDB operation."""
    store, query, update_meta, get_by_meta, delete = _original_make_chroma(client)

    # Each closure instance gets an ID
    with _call_lock:
        closure_id = id(store)

    def logged_store(collection, doc_id, doc, embedding_json, metadata_json):
        global _call_counter
        with _call_lock:
            _call_counter += 1
            n = _call_counter
        tid = threading.current_thread().name
        try:
            store(collection, doc_id, doc, embedding_json, metadata_json)
            # only log occasionally to avoid spam
            if n % 10 == 0:
                print(f"  [diag] store #{n} col={collection} id={doc_id} tid={tid} -- OK", flush=True)
        except Exception as e:
            print(f"  [diag] store #{n} col={collection} id={doc_id} tid={tid} -- FAILED: {e}", flush=True)
            raise

    def logged_query(collection, embedding_json, n_results, where_json):
        global _call_counter
        with _call_lock:
            _call_counter += 1
            num = _call_counter
        tid = threading.current_thread().name
        where = json.loads(where_json)
        print(f"  [diag] query #{num} col={collection} n={n_results} where={where} tid={tid} ...", flush=True)
        try:
            result = query(collection, embedding_json, n_results, where_json)
            parsed = json.loads(result)
            n_ids = len(parsed.get("ids", [[]])[0])
            print(f"  [diag] query #{num} col={collection} -- OK ({n_ids} results)", flush=True)
            return result
        except Exception as e:
            print(f"  [diag] query #{num} col={collection} -- FAILED: {e}", flush=True)
            print(f"  [diag] traceback:\n{''.join(traceback.format_exc())}", flush=True)
            raise

    def logged_update_meta(collection, doc_id, metadata_json):
        global _call_counter
        with _call_lock:
            _call_counter += 1
            n = _call_counter
        tid = threading.current_thread().name
        meta = json.loads(metadata_json)
        print(f"  [diag] update_meta #{n} col={collection} id={doc_id} meta={meta} tid={tid}", flush=True)
        try:
            update_meta(collection, doc_id, metadata_json)
        except Exception as e:
            print(f"  [diag] update_meta #{n} FAILED: {e}", flush=True)
            raise

    def logged_delete(collection, ids_json):
        global _call_counter
        with _call_lock:
            _call_counter += 1
            n = _call_counter
        tid = threading.current_thread().name
        ids = json.loads(ids_json)
        print(f"  [diag] delete #{n} col={collection} ids={ids} tid={tid}", flush=True)
        try:
            delete(collection, ids_json)
        except Exception as e:
            print(f"  [diag] delete #{n} FAILED: {e}", flush=True)
            raise

    return logged_store, logged_query, logged_update_meta, get_by_meta, logged_delete


# Apply the monkey-patch
mem_module._make_chroma_callbacks = _instrumented_make_chroma
print("=== Instrumented _make_chroma_callbacks ===\n", flush=True)

# Now run the real app init + 4 turns via SceneLoop
from rhapsode._core import (
    Director, MemorySystem, Scene, SceneLoop,
    Validator, Weaver,
)
from rhapsode.memory import (
    register_callbacks, register_character_memory_callbacks,
    warmup_model,
)
from rhapsode.prompt import build_system_message, build_user_message
from rhapsode.validator import make_local_llm_callback

SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"

warmup_model()

# --- Replicate exact app.py init ---

scene = Scene.load_json(SCENARIO)
memory = MemorySystem(scene.scene_id)
register_callbacks(memory, scene.scene_id, CHROMA_PATH)
memory.set_local_llm_callback(make_local_llm_callback())
scene.set_memory(memory)

is_resuming = scene.has_save(SAVES_DIR)
if is_resuming:
    scene.load_save(SAVES_DIR)
    print(f"Resumed: turn={scene.turn_index}")

print("\n--- _sync_graph_to_memory ---")
all_nodes = scene.world_graph.all_nodes_including_expired()
for n in all_nodes:
    if n.id == 0: continue
    memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
expired = [n for n in all_nodes if n.valid_until != -1]
if expired:
    memory.sync_expired(expired)
print(f"Synced {len(all_nodes)} nodes ({len(expired)} expired)")

print("\n--- _init_character_memories ---")
for name, mem in scene.character_memories.items():
    register_character_memory_callbacks(mem, CHROMA_PATH)
    mem.set_reflection_llm_callback(make_local_llm_callback())
    mem.sync_to_chroma()

# --- Wire up loop ---
director = Director(scene.world_graph)
validator = Validator(scene.world_graph)
validator.set_llm_callback(make_local_llm_callback())
validator.set_search_callback(lambda q, k: memory.search_nodes(q, k))
validator.set_dead_check(lambda: [c.name for c in scene.characters if c.dead])
director.set_validator(validator)

weaver = Weaver(scene.world_graph)
weaver.set_llm_callback(lambda p: '"ok"')
weaver.set_local_llm_callback(make_local_llm_callback())

def stub_narrator(system, user):
    plan = json.dumps({
        "transitions": [], "new_nodes": [],
        "speech_turns": [
            {"character": "Sergeant Maren", "cue": "react",
             "dramatic_intent": "reflect", "emotional_state": "watchful",
             "responds_to": "events"}
        ],
        "new_characters": [], "active_cast": ["Sergeant Maren"],
    })
    return f"The courtyard is quiet.\n<<<RHAPSODE_JSON>>>\n{plan}"

def stub_actor(prompt):
    return '"I have nothing to say."'

system_msg = build_system_message(scene)

def _established_facts(hist, scene_obj, director_out):
    try:
        ids = memory.search_nodes("recent events", 6)
        return [scene_obj.world_graph.get_node(nid).fact for nid in ids
                if scene_obj.world_graph.get_node(nid)]
    except Exception as e:
        print(f"  [established_facts] EXCEPTION: {e}", flush=True)
        return []

def _active_chars(scene_obj):
    return [f"- {c.name}" for c in scene_obj.characters
            if c.on_stage and not c.dead and not c.is_player]

def prompt_cb(hist, scene_obj, director_out, focus_text, inner_states):
    user_msg = build_user_message(
        hist, scene_obj,
        director_focus_text=focus_text,
        established_facts=_established_facts(hist, scene_obj, director_out),
        active_characters=_active_chars(scene_obj),
        story_so_far=scene_obj.downsampler.render(),
        inner_states=inner_states,
    )
    return (system_msg, user_msg)

loop = SceneLoop()
loop.load_scene(scene)
loop.set_director(director)
loop.set_prompt_callback(prompt_cb)
loop.set_narrator_llm_callback(stub_narrator)
loop.set_llm_callback(stub_actor)
loop.set_actor_llm_callback(stub_actor)
loop.set_weaver(weaver)
loop.set_saves_dir(SAVES_DIR)
if is_resuming:
    loop.set_resuming(True)
scene.downsampler.set_llm_callback(make_local_llm_callback())

# --- Run turns with full post-turn processing ---
for turn_num in range(1, 5):
    print(f"\n{'='*60}")
    print(f"=== SUBMITTING TURN {turn_num} ===")
    print(f"{'='*60}", flush=True)

    try:
        loop.submit_input(f"I do something on turn {turn_num}.")
        print(f"\n  Turn {turn_num}: submit_input OK", flush=True)
    except Exception as e:
        print(f"\n  Turn {turn_num}: submit_input FAILED: {e}", flush=True)
        try:
            loop.join_background()
        except:
            pass
        from rhapsode._core import SceneLoop as SL
        loop = SceneLoop()
        loop.load_scene(scene)
        loop.set_director(director)
        loop.set_prompt_callback(prompt_cb)
        loop.set_narrator_llm_callback(stub_narrator)
        loop.set_llm_callback(stub_actor)
        loop.set_actor_llm_callback(stub_actor)
        loop.set_weaver(weaver)
        loop.set_saves_dir(SAVES_DIR)
        scene.downsampler.set_llm_callback(make_local_llm_callback())
        continue

    # Post-turn processing (same as app.py)
    try:
        expiry_ops = loop.take_completed_expiry_ops()
        if expiry_ops:
            nodes = [scene.world_graph.get_node(op.id) for op in expiry_ops]
            nodes = [n for n in nodes if n is not None]
            if nodes:
                print(f"  Post-turn: syncing {len(nodes)} expired nodes", flush=True)
                memory.sync_expired(nodes)
    except Exception as e:
        print(f"  Post-turn expiry FAILED: {e}", flush=True)

    try:
        output = loop.last_director_output()
        if output.new_nodes:
            print(f"  Post-turn: processing {len(output.new_nodes)} new nodes", flush=True)
            memory.process_new_nodes(output.new_nodes, scene.turn_index)
        if output.newly_expired:
            print(f"  Post-turn: syncing {len(output.newly_expired)} newly expired", flush=True)
            memory.sync_expired(output.newly_expired)
    except Exception as e:
        print(f"  Post-turn pipeline FAILED: {e}", flush=True)

    for chunk in loop.take_last_turn_outputs():
        pass  # consume outputs

loop.join_background()
print(f"\n{'='*60}")
print("DONE - all turns processed")
print(f"{'='*60}", flush=True)
