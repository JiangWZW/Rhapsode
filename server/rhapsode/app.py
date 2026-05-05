import asyncio
import os
import pathlib

from dotenv import load_dotenv
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from rhapsode._core import Scene, SceneLoop, SceneMessage
from rhapsode.prompt import build_prompt
from rhapsode.gemini import complete

load_dotenv()

_server_dir = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = _server_dir / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")

app = FastAPI(title="Rhapsode")


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()

    scene = Scene.load_json(str(SCENARIO_PATH))
    loop = SceneLoop()
    loop.load_scene(scene)

    response_text: str | None = None

    def prompt_cb(history_snapshot, scene_obj):
        return build_prompt(history_snapshot, scene_obj)

    def llm_cb(prompt: str) -> str:
        return complete([{"role": "user", "parts": [{"text": prompt}]}])

    def turn_complete_cb(msg: SceneMessage):
        nonlocal response_text
        response_text = msg.content

    loop.set_prompt_callback(prompt_cb)
    loop.set_llm_callback(llm_cb)
    loop.set_turn_complete_callback(turn_complete_cb)

    # Send seed messages on connect
    for msg in scene.history.messages():
        await ws.send_json({
            "type": "assistant_message",
            "content": msg.content,
        })

    try:
        while True:
            data = await ws.receive_json()
            if data.get("type") != "player_message":
                continue

            text = data.get("content", "").strip()
            if not text:
                continue

            await ws.send_json({"type": "status", "state": "processing"})

            response_text = None
            try:
                await asyncio.get_event_loop().run_in_executor(
                    None, loop.submit_input, text
                )
            except Exception as exc:
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                loop = SceneLoop()
                loop.load_scene(scene)
                loop.set_prompt_callback(prompt_cb)
                loop.set_llm_callback(llm_cb)
                loop.set_turn_complete_callback(turn_complete_cb)
                continue

            if response_text:
                await ws.send_json({
                    "type": "assistant_message",
                    "content": response_text,
                })

            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        pass
