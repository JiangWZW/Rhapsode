import asyncio
import json
import logging
import os
import pathlib
from contextlib import asynccontextmanager

from dotenv import load_dotenv
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from rhapsode._core import Director, MemorySystem, NodeState, Scene, SceneLoop, SceneMessage
from rhapsode.character_agent import make_character_synth
from rhapsode.llm import complete
from rhapsode.memory import register_callbacks, warmup_model
from rhapsode.prompt import build_merged_prompt
from rhapsode.spacy_models import get_nlp_lemma
from rhapsode.validator import make_local_llm_callback

load_dotenv()
log = logging.getLogger(__name__)

_server_dir = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = _server_dir / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")
SAVES_DIR = str(_server_dir / "saves")


@asynccontextmanager
async def lifespan(application: FastAPI):
    warmup_model()
    get_nlp_lemma()
    yield


app = FastAPI(title="Rhapsode", lifespan=lifespan)


def _init_memory(scene_id: str) -> MemorySystem:
    memory = MemorySystem(scene_id)
    register_callbacks(memory, scene_id)
    memory.set_local_llm_callback(make_local_llm_callback())
    return memory


def _scene_ws_payload(msg: SceneMessage) -> dict:
    payload = {"type": "scene_message", "content": msg.content}
    try:
        meta = json.loads(msg.metadata)
    except (json.JSONDecodeError, TypeError):
        meta = {}
    payload["scene_kind"] = meta.get("scene_kind") or "narrator"
    if meta.get("speaker"):
        payload["speaker"] = meta["speaker"]
    return payload


def _call_llm(prompt: str) -> str:
    return complete([{"role": "user", "parts": [{"text": prompt}]}])


def _build_memory_query(history: list[SceneMessage], scene_obj: Scene, director_out) -> str:
    parts = [scene_obj.title]
    for msg in history[-4:]:
        role = msg.role.name.lower()
        parts.append(f"{role}: {msg.content}")
    if director_out and director_out.context_blocks:
        parts.extend(director_out.context_blocks)
    return "\n".join(parts)


def _active_characters(scene_obj: Scene) -> list[str]:
    char_lookup = {
        c.name.lower(): c.name
        for c in scene_obj.characters
        if not c.is_player
    }
    active_names: set[str] = set()
    for node in scene_obj.world_graph.all_nodes():
        if node.state != NodeState.Active:
            continue
        for ent in node.entities:
            ent_l = ent.lower()
            for key, display in char_lookup.items():
                if key in ent_l or ent_l in key:
                    active_names.add(display)
    return sorted(active_names)


def _established_facts(
    memory: MemorySystem, history: list[SceneMessage], scene_obj: Scene, director_out
) -> list[str]:
    try:
        raw = memory.retrieve_for_injection(_build_memory_query(history, scene_obj, director_out), 8)
        facts = json.loads(raw)
        if isinstance(facts, list):
            return [str(f) for f in facts]
    except Exception:
        log.exception("Failed to retrieve narrator established facts")
    return []


def _wire_loop(scene: Scene, director: Director, memory: MemorySystem) -> SceneLoop:
    loop = SceneLoop()
    loop.load_scene(scene)
    loop.set_director(director)
    loop.set_character_synth_callback(make_character_synth(scene))

    loop.set_prompt_callback(
        lambda hist, scene_obj, director_out, focus_json: build_merged_prompt(
            hist,
            scene_obj,
            director_out,
            director_focus_json=focus_json,
            established_facts=_established_facts(memory, hist, scene_obj, director_out),
            active_characters=_active_characters(scene_obj),
        )
    )
    loop.set_llm_callback(_call_llm)
    return loop


async def _send_seed_messages(ws: WebSocket, scene: Scene, is_resuming: bool = False):
    seeds = scene.history.snapshot(8) if is_resuming else scene.history.messages()
    for msg in seeds:
        if msg.role.name.lower() != "assistant":
            continue
        await ws.send_json(_scene_ws_payload(msg))


def _player_text(data: dict) -> str | None:
    if data.get("type") != "player_message":
        return None
    text = data.get("content", "").strip()
    return text or None


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()

    scene = Scene.load_json(str(SCENARIO_PATH))
    memory = _init_memory(scene.scene_id)
    scene.set_memory(memory)

    is_resuming = scene.has_save(SAVES_DIR)
    if is_resuming:
        scene.load_save(SAVES_DIR)
        log.info(
            "=== SESSION RESUMED === scene_id=%s turn=%d graph=%d hist=%d",
            scene.scene_id,
            scene.turn_index,
            scene.world_graph.size(),
            scene.history.size(),
        )
    else:
        log.info(
            "=== FRESH START === scene_id=%s graph=%d hist=%d",
            scene.scene_id,
            scene.world_graph.size(),
            scene.history.size(),
        )

    director = Director(scene.world_graph)
    loop = _wire_loop(scene, director, memory)
    if is_resuming:
        loop.set_resuming(True)

    await _send_seed_messages(ws, scene, is_resuming)

    try:
        while True:
            data = await ws.receive_json()
            text = _player_text(data)
            if not text:
                continue

            await ws.send_json({"type": "status", "state": "processing"})

            try:
                await asyncio.get_event_loop().run_in_executor(None, loop.submit_input, text)
            except Exception as exc:
                log.exception("Turn failed")
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                loop = _wire_loop(scene, director, memory)
                continue

            try:
                output = loop.last_director_output()
                if output.new_nodes:
                    memory.process_new_nodes(output.new_nodes, scene.turn_index)
                if output.newly_resolved:
                    memory.sync_resolved(output.newly_resolved, scene.turn_index)
            except Exception:
                log.exception("Post-generation pipeline failed")

            try:
                scene.save(SAVES_DIR)
            except Exception:
                log.exception("Auto-save failed")

            for chunk in loop.take_last_turn_outputs():
                await ws.send_json(_scene_ws_payload(chunk))

            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        pass
