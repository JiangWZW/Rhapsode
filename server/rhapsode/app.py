import asyncio
import json
import logging
import os
import pathlib

from dotenv import load_dotenv
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from rhapsode._core import Director, NodePool, Scene, SceneLoop, SceneMessage
from rhapsode.gemini import complete
from rhapsode.prompt import build_prompt

load_dotenv()
log = logging.getLogger(__name__)

_server_dir = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = _server_dir / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")

app = FastAPI(title="Rhapsode")

DIRECTOR_SYSTEM_PROMPT = """\
You are the narrative director. Input is JSON with nodes and scene context.
Return ONLY valid JSON with two keys:

  transitions: [{"id": <number>, "state": "dormant|foreshadowed|active|resolved"}]
  new_nodes:   [{"fact": <string>, "type": <string>,
                 "state": "dormant|foreshadowed|active|resolved",
                 "foreshadow_ctx": <string>, "active_ctx": <string>,
                 "entities": [<string>], "known_by": [<string>]}]

If resolved_context is provided, these are facts from earlier in the story.
Use them to maintain consistency — do NOT contradict or resurrect resolved events.
Keep node text concise."""


def _load_node_pool(scenario_path: pathlib.Path) -> NodePool:
    raw = json.loads(scenario_path.read_text(encoding="utf-8"))
    pool_json = json.dumps({"next_id": 1, "nodes": raw.get("nodes", [])})
    return NodePool.from_json_str(pool_json)


def _scene_id(scenario_path: pathlib.Path) -> str:
    return scenario_path.stem


def _extract_json_object(text: str) -> str:
    start = text.find("{")
    end = text.rfind("}")
    if start >= 0 and end > start:
        return text[start : end + 1]
    return text


def _call_llm(prompt: str) -> str:
    return complete([{"role": "user", "parts": [{"text": prompt}]}])


def _node_to_dict(node) -> dict:
    return {
        "id":          node.id,
        "fact":        node.fact,
        "type":        node.type,
        "entities":    list(node.entities),
        "known_by":    list(node.known_by),
        "resolved_at": node.resolved_at,
    }


def _wire_loop(scene: Scene, director: Director, on_turn_complete) -> SceneLoop:
    loop = SceneLoop()
    loop.load_scene(scene)
    loop.set_director(director)
    loop.set_prompt_callback(
        lambda history, scene_obj, director_out: build_prompt(history, scene_obj, director_out)
    )
    loop.set_llm_callback(_call_llm)
    loop.set_turn_complete_callback(on_turn_complete)
    return loop


async def _send_seed_messages(ws: WebSocket, scene: Scene):
    for msg in scene.history.messages():
        await ws.send_json({"type": "assistant_message", "content": msg.content})


def _player_text(data: dict) -> str | None:
    if data.get("type") != "player_message":
        return None
    text = data.get("content", "").strip()
    return text or None


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()

    from rhapsode.memory import ResolvedMemory

    scene = Scene.load_json(str(SCENARIO_PATH))
    node_pool = _load_node_pool(SCENARIO_PATH)
    director = Director(node_pool)
    memory = ResolvedMemory(_scene_id(SCENARIO_PATH))

    director.set_retrieval_callback(memory.retrieve)

    response_text: str | None = None

    def on_turn_complete(msg: SceneMessage):
        nonlocal response_text
        response_text = msg.content

    def director_llm_cb(prompt_json: str) -> str:
        full_prompt = f"{DIRECTOR_SYSTEM_PROMPT}\n\nInput JSON:\n{prompt_json}"
        raw = _call_llm(full_prompt).strip()
        return _extract_json_object(raw)

    director.set_llm_callback(director_llm_cb)
    loop = _wire_loop(scene, director, on_turn_complete)
    await _send_seed_messages(ws, scene)

    try:
        while True:
            data = await ws.receive_json()
            text = _player_text(data)
            if not text:
                continue

            await ws.send_json({"type": "status", "state": "processing"})
            response_text = None

            try:
                await asyncio.get_event_loop().run_in_executor(
                    None, loop.submit_input, text
                )
            except Exception as exc:
                log.exception("Turn failed")
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                loop = _wire_loop(scene, director, on_turn_complete)
                continue

            resolved = loop.last_director_output().newly_resolved
            if resolved:
                dicts = [_node_to_dict(n) for n in resolved]
                memory.store_batch(dicts)

            if response_text:
                await ws.send_json({"type": "assistant_message", "content": response_text})
            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        pass
