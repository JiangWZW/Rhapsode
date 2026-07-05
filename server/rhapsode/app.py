import asyncio
import html
import json
import logging
import os
import pathlib
import re
import subprocess
from contextlib import asynccontextmanager
from dataclasses import dataclass

from dotenv import load_dotenv
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import Response

from rhapsode._core import (
    Annotator, Director, MemorySystem, Scene, SceneLoop, SceneMessage,
    Weaver, analyze_graph,
)
from rhapsode.llm import complete, complete_with_tools
from rhapsode.memory import register_callbacks, warmup_model
from rhapsode.fable import make_ner_callback, warmup_fable
from rhapsode.validator import make_local_llm_callback

load_dotenv()
log = logging.getLogger(__name__)

_server_dir = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = _server_dir / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")
SAVES_DIR = str(_server_dir / "saves")


@asynccontextmanager
async def lifespan(application: FastAPI):
    warmup_model()
    warmup_fable()
    yield


app = FastAPI(title="Rhapsode", lifespan=lifespan)


def _init_memory(scene_id: str) -> MemorySystem:
    memory = MemorySystem(scene_id)
    register_callbacks(memory, scene_id)
    return memory


def _init_character_memories(scene: Scene):
    """Wire the reflection LLM callback on all CharacterMemory instances."""
    for name, mem in scene.character_memories.items():
        # Background reflection (contradict-vs-extend + weight judgments) runs on
        # the cloud provider (DeepSeek when RHAPSODE_PROVIDER=deepseek); it is
        # async/off the turn's critical path so the latency hides.
        mem.set_reflection_llm_callback(_call_llm)


def _sync_graph_to_memory(scene: Scene, memory: MemorySystem) -> None:
    """Ensure all graph nodes are indexed in ChromaDB (catches seed nodes from scenario load).

    Uses upsert so it's safe to call on every startup — already-indexed nodes
    are overwritten with the same data.  ~30 nodes embeds in 1-2 seconds.
    """
    all_nodes = scene.world_graph.all_nodes_including_expired()
    if not all_nodes:
        return
    for n in all_nodes:
        if n.id == 0:
            continue
        memory.store_node(n.id, n.fact, n.state.name.lower(), n.type, n.created_at)
    expired = [n for n in all_nodes if n.valid_until != -1]
    if expired:
        memory.sync_expired(expired)
    log.info("  [memory] Synced %d graph nodes to ChromaDB (%d expired)",
             len(all_nodes), len(expired))


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


# -- Narrator tool-use infrastructure ------------------------------------------

NARRATOR_TOOLS = [
    {
        "name": "query_graph",
        "description": (
            "Search the world graph by entity name or free text. For entity matches, "
            "returns the full entity-timeline chain (chronologically ordered nodes with "
            "valid_until annotations: -1 = still true, N = was true until turn N). "
            "For text matches, returns matching nodes with their chain predecessors. "
            "Use entity names like 'Player', 'Aqua' to trace an entity's history."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "Entity name (exact match, e.g. 'Player') or free-text search",
                },
            },
            "required": ["query"],
        },
    },
    {
        "name": "query_mind",
        "description": (
            "Get a character's current thoughts, beliefs, emotional state, "
            "and dialogue voice."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "character": {
                    "type": "string",
                    "description": "Character name",
                },
            },
            "required": ["character"],
        },
    },
    {
        "name": "query_history",
        "description": "Search past conversation history for relevant events by keyword.",
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "What to search for",
                },
            },
            "required": ["query"],
        },
    },
]


def _make_tool_dispatcher(scene: Scene):
    """Create a tool dispatcher that calls C++ Scene methods via pybind11."""
    def dispatch(name: str, args: dict) -> str:
        if name == "query_graph":
            return scene.tool_query_graph(args.get("query", ""))
        elif name == "query_mind":
            return scene.tool_query_mind(args.get("character", ""))
        elif name == "query_history":
            return scene.tool_query_history(args.get("query", ""))
        return json.dumps({"error": f"unknown tool: {name}"})
    return dispatch


def _make_narrator_callback(scene: Scene):
    """Create a narrator callback that uses the tool-use loop."""
    dispatcher = _make_tool_dispatcher(scene)

    def _call_narrator_llm(instructions: str, turn_state: str) -> str:
        messages = [
            {"role": "system", "parts": [{"text": instructions}]},
            {"role": "user", "parts": [{"text": turn_state}]},
        ]
        return complete_with_tools(messages, NARRATOR_TOOLS, dispatcher)
    return _call_narrator_llm


def _wire_loop(scene: Scene, director: Director, memory: MemorySystem,
               weaver: Weaver | None = None) -> SceneLoop:
    loop = SceneLoop()
    loop.load_scene(scene)
    loop.set_director(director)

    scene.downsampler.set_llm_callback(make_local_llm_callback(repair_json=False))

    loop.set_narrator_llm_callback(_make_narrator_callback(scene))
    loop.set_llm_callback(_call_llm)
    if weaver:
        loop.set_weaver(weaver)
    loop.set_saves_dir(SAVES_DIR)
    return loop


async def _send_seed_messages(ws: WebSocket, scene: Scene, is_resuming: bool = False):
    seeds = scene.display_timeline(8) if is_resuming else scene.display_timeline()
    for msg in seeds:
        role = msg.role.name.lower()
        if role == "assistant":
            await ws.send_json(_scene_ws_payload(msg))
        elif role == "user":
            await ws.send_json({"type": "user_message", "content": msg.content})


def _player_text(data: dict) -> str | None:
    if data.get("type") != "player_message":
        return None
    text = data.get("content", "").strip()
    return text or None


def _load_saved_scene() -> Scene | None:
    scene = Scene.load_json(str(SCENARIO_PATH))
    if scene.has_save(SAVES_DIR):
        scene.load_save(SAVES_DIR)
        return scene
    return scene


@app.get("/graph.dot", response_class=Response)
def graph_dot():
    scene = _load_saved_scene()
    if scene is None:
        return Response("// no active scene", media_type="text/plain", status_code=404)
    return Response(scene.world_graph.to_dot(), media_type="text/vnd.graphviz")


@app.get("/graph.svg", response_class=Response)
def graph_svg():
    scene = _load_saved_scene()
    if scene is None:
        return Response("no active scene", media_type="text/plain", status_code=404)
    return _dot_to_svg_response(scene.world_graph.to_dot())


def _dot_to_svg_string(dot: str) -> tuple[bool, str]:
    """Render a Graphviz dot string to an SVG string.

    Returns (success, result): on success result is the SVG string; on failure
    result is an error message.
    """
    try:
        proc = subprocess.run(
            ["dot", "-Tsvg"],
            input=dot.encode("utf-8"),
            capture_output=True,
            timeout=10,
        )
        if proc.returncode != 0:
            detail = proc.stderr.decode(errors="replace")
            return False, f"dot failed: {detail}"
        return True, proc.stdout.decode("utf-8", errors="replace")
    except FileNotFoundError:
        return False, "Graphviz 'dot' not found. Install: winget install Graphviz"


def _dot_to_svg_response(dot: str) -> Response:
    """Render a Graphviz dot string to an SVG Response (shared by graph endpoints)."""
    ok, result = _dot_to_svg_string(dot)
    if not ok:
        return Response(result, media_type="text/plain", status_code=500)
    return Response(result.encode("utf-8"), media_type="image/svg+xml")


def _safe_render_thoughts(mem) -> str:
    try:
        return mem.render_thoughts([]) or "(no live thoughts yet)"
    except UnicodeDecodeError:
        return "(thoughts unavailable: invalid UTF-8 in belief data)"


def _find_character_memory(scene, name: str):
    """Look up a character's mind by name (case-insensitive), or None."""
    mems = scene.character_memories
    if name in mems:
        return mems[name]
    for key in mems.keys():
        if key.lower() == name.lower():
            return mems[key]
    return None


@app.get("/characters")
def characters_endpoint():
    """List characters with a mind, and their current inner state."""
    scene = _load_saved_scene()
    if scene is None:
        return {"error": "no active scene"}
    return {
        "characters": [
            {"name": name, "interior": _safe_render_thoughts(mem)}
            for name, mem in scene.character_memories.items()
        ]
    }


@app.get("/character/{name}/graph.dot", response_class=Response)
def character_graph_dot(name: str):
    """Graphviz dot of one character's subjective belief graph."""
    scene = _load_saved_scene()
    if scene is None:
        return Response("// no active scene", media_type="text/plain", status_code=404)
    mem = _find_character_memory(scene, name)
    if mem is None:
        return Response(f"// no character '{name}'", media_type="text/plain", status_code=404)
    return Response(mem.beliefs.to_dot(), media_type="text/vnd.graphviz")


@app.get("/character/{name}/graph.svg", response_class=Response)
def character_graph_svg(name: str):
    """Rendered SVG of one character's subjective belief graph."""
    scene = _load_saved_scene()
    if scene is None:
        return Response("no active scene", media_type="text/plain", status_code=404)
    mem = _find_character_memory(scene, name)
    if mem is None:
        return Response(f"no character '{name}'", media_type="text/plain", status_code=404)
    return _dot_to_svg_response(mem.beliefs.to_dot())


@app.get("/minds", response_class=Response)
def minds_endpoint():
    """Debug view: every character's inner state + belief graph on one page."""
    scene = _load_saved_scene()
    if scene is None:
        return Response("<h1>no active scene</h1>", media_type="text/html", status_code=404)

    parts = [
        "<html><head><meta charset='utf-8'><title>Character minds</title><style>",
        "body{background:#1e1e2e;color:#cdd6f4;font-family:'Segoe UI',sans-serif;margin:24px}",
        "h1{color:#cdd6f4} h2{color:#f9e2af;border-bottom:1px solid #45475a;padding-bottom:4px}",
        ".state{color:#a6adc8;font-style:italic;white-space:pre-wrap;margin:8px 0 12px}",
        ".mind{margin-bottom:48px}",
        # Render the SVG at its natural size inside a scrollable box.  Clamping
        # to the page width (max-width:100%) shrank large graphs to illegibility
        # and made browser zoom useless -- it just re-fit to the viewport.
        ".graphwrap{overflow:auto;max-height:85vh;border:1px solid #45475a;"
        "border-radius:6px;resize:vertical}",
        "svg{display:block;background:#1e1e2e}",
        ".legend{font-size:12px;color:#6c7086;margin-bottom:24px}",
        "</style></head><body><h1>Character minds</h1>",
        "<p class='legend'>green = current belief / perception &nbsp;|&nbsp; "
        "blue = superseded (history) &nbsp;|&nbsp; yellow = foreshadowed</p>",
    ]
    for name, mem in scene.character_memories.items():
        try:
            dot = mem.beliefs.to_dot()
        except Exception as exc:  # noqa: BLE001 -- one bad mind must not 500 the page
            parts.append(
                f"<div class='mind'><h2>{html.escape(name)}</h2>"
                f"<pre>belief graph unavailable: {html.escape(str(exc))}</pre></div>"
            )
            continue
        try:
            ok, svg = _dot_to_svg_string(dot)
            if ok:
                idx = svg.find("<svg")
                svg = svg[idx:] if idx >= 0 else svg
            else:
                svg = f"<pre>{html.escape(svg)}</pre>"
        except Exception as exc:  # noqa: BLE001 -- one bad mind must not 500 the page
            svg = f"<pre>belief graph unavailable: {html.escape(str(exc))}</pre>"
        state = _safe_render_thoughts(mem)
        parts.append(
            f"<div class='mind'><h2>{html.escape(name)}</h2>"
            f"<div class='state'>{html.escape(state)}</div>"
            f"<div class='graphwrap'>{svg}</div></div>"
        )
    parts.append("</body></html>")
    return Response("".join(parts), media_type="text/html")


@app.get("/analyze")
def analyze_endpoint():
    scene = _load_saved_scene()
    if scene is None:
        return {"error": "no active scene"}
    a = analyze_graph(scene.world_graph)
    return {
        "live_node_count": a.live_node_count,
        "active_edge_count": a.active_edge_count,
        "orphan_count": a.orphan_count,
    }


@app.post("/weave")
def weave_endpoint():
    scene = _load_saved_scene()
    if scene is None:
        return {"error": "no active scene"}
    w = Weaver(scene.world_graph)
    w.set_llm_callback(_call_llm)
    result = w.weave(scene.turn_index)
    scene.save(SAVES_DIR)
    return {
        "connected": len(result.connected),
        "disconnected": len(result.disconnected),
        "reweighted": len(result.reweighted),
        "analysis": {
            "live_node_count": result.analysis.live_node_count,
            "active_edge_count": result.analysis.active_edge_count,
            "orphan_count": result.analysis.orphan_count,
        },
    }


@dataclass
class WsSession:
    scene: Scene
    memory: MemorySystem
    director: Director
    weaver: Weaver
    annotator: Annotator
    loop: SceneLoop
    is_resuming: bool


def _setup_ws_session() -> WsSession:
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

    _sync_graph_to_memory(scene, memory)
    _init_character_memories(scene)

    director = Director(scene.world_graph)

    weaver = Weaver(scene.world_graph)
    weaver.set_llm_callback(_call_llm)
    weaver.set_local_llm_callback(make_local_llm_callback())

    annotator = Annotator(scene)
    annotator.set_ner_callback(make_ner_callback())

    loop = _wire_loop(scene, director, memory, weaver)
    if is_resuming:
        loop.set_resuming(True)

    return WsSession(scene=scene, memory=memory, director=director,
                     weaver=weaver, annotator=annotator, loop=loop,
                     is_resuming=is_resuming)


def _rewire_loop(session: WsSession, *, resuming: bool = False) -> None:
    session.loop = _wire_loop(session.scene, session.director, session.memory,
                              session.weaver)
    if resuming:
        session.loop.set_resuming(True)


async def _handle_undo(ws: WebSocket, session: WsSession, n: int) -> None:
    loop = session.loop
    scene = session.scene
    await asyncio.get_event_loop().run_in_executor(None, loop.join_background)
    reverted = scene.revert_turns(n)
    log.info("/undo %d -> reverted %d turns, now at turn %d",
             n, reverted, scene.turn_index)
    scene.save(SAVES_DIR)
    _rewire_loop(session, resuming=True)
    await ws.send_json({"type": "undo", "turns_reverted": reverted,
                        "turn_index": scene.turn_index})
    await _send_seed_messages(ws, scene, is_resuming=True)
    await ws.send_json({"type": "status", "state": "idle"})


def _sync_post_turn(scene: Scene, memory: MemorySystem, loop: SceneLoop) -> None:
    try:
        expiry_ops = loop.take_completed_expiry_ops()
        if expiry_ops:
            nodes = [scene.world_graph.get_node(op.id) for op in expiry_ops]
            nodes = [n for n in nodes if n is not None]
            if nodes:
                memory.sync_expired(nodes)
                log.info("  [expiry] synced %d superseded fact(s)", len(nodes))
    except Exception:
        log.exception("Expiry sync failed")

    try:
        output = loop.last_director_output()
        if output.new_nodes:
            memory.process_new_nodes(output.new_nodes, scene.turn_index)
        if output.newly_expired:
            memory.sync_expired(output.newly_expired)
    except Exception:
        log.exception("Post-generation pipeline failed")


async def _stream_turn_outputs(ws: WebSocket, annotator: Annotator,
                               loop: SceneLoop) -> None:
    for chunk in loop.take_last_turn_outputs():
        payload = _scene_ws_payload(chunk)
        if payload.get("scene_kind") == "narrator":
            try:
                spans = annotator.annotate(chunk.content)
                payload["entities"] = [
                    {"start": s.start, "end": s.end_,
                     "text": s.text, "category": s.category}
                    for s in spans
                ]
            except Exception:
                log.exception("Entity annotation failed")
        await ws.send_json(payload)


@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()

    session = _setup_ws_session()
    await _send_seed_messages(ws, session.scene, session.is_resuming)

    try:
        while True:
            data = await ws.receive_json()
            text = _player_text(data)
            if not text:
                continue

            undo_match = re.match(r"^/undo(?:\s+(\d+))?$", text.strip())
            if undo_match:
                n = int(undo_match.group(1) or "1")
                await _handle_undo(ws, session, n)
                continue

            await ws.send_json({"type": "status", "state": "processing"})

            try:
                await asyncio.get_event_loop().run_in_executor(
                    None, session.loop.submit_input, text)
            except Exception as exc:
                log.exception("Turn failed")
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                await asyncio.get_event_loop().run_in_executor(
                    None, session.loop.join_background)
                _rewire_loop(session)
                continue

            _sync_post_turn(session.scene, session.memory, session.loop)
            await _stream_turn_outputs(ws, session.annotator, session.loop)
            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        session.loop.join_background()
