"""Read-only HTTP endpoints for inspecting engine state: the world graph, each
character's belief graph, the minds debug page, live scenes, graph analysis, and
a manual weave trigger. Mounted as an APIRouter on the app."""

import html
import json
import logging
import subprocess

from fastapi import APIRouter
from fastapi.responses import Response

from rhapsode._core import Story, analyze_graph
from rhapsode.config import SAVES_DIR, SCENARIO_PATH
from rhapsode.llm_tools import make_weaver_callback

log = logging.getLogger(__name__)

router = APIRouter()


def _load_saved_story() -> Story:
    story = Story.load_scenario(str(SCENARIO_PATH))
    if story.has_save(SAVES_DIR):
        story.load_save(SAVES_DIR)
    return story


def _load_saved_state():
    """Keep Story alive beside its active SceneData reference."""
    story = _load_saved_story()
    return story, story.active_scene()


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
        return mem.view_of([]) or "(no live beliefs yet)"
    except UnicodeDecodeError:
        return "(beliefs unavailable: invalid UTF-8 in belief data)"


def _mind_snapshot(mem) -> dict:
    core = ""
    streams: list = []
    try:
        raw = mem.render_mind_query()
        payload = json.loads(raw) if isinstance(raw, str) else {}
        core = str(payload.get("core") or "")
        streams = payload.get("streams") or []
        if not isinstance(streams, list):
            streams = []
    except Exception:
        pass
    return {
        "core": core,
        "streams": streams,
        "beliefs": _safe_render_thoughts(mem),
        "active_stream_count": getattr(mem, "active_stream_count", len(streams)),
    }


def _format_streams_html(streams: list) -> str:
    if not streams:
        return "<p class='muted'>(no active monologue streams)</p>"
    blocks = []
    for stream in streams:
        if not isinstance(stream, dict):
            continue
        sid = html.escape(str(stream.get("id") or "?"))
        focus = html.escape(str(stream.get("focus") or ""))
        lines = stream.get("recent_lines") or stream.get("lines") or []
        line_bits = []
        if isinstance(lines, list):
            for line in lines[-5:]:
                if not isinstance(line, dict):
                    continue
                turn = line.get("turn", "?")
                text = html.escape(str(line.get("text") or ""))
                line_bits.append(f"<li><span class='turn'>t{turn}</span> {text}</li>")
        body = (
            f"<ul class='lines'>{''.join(line_bits)}</ul>"
            if line_bits else "<p class='muted'>(empty — listening)</p>"
        )
        blocks.append(
            f"<div class='stream'><div class='stream-head'>"
            f"<strong>{sid}</strong> — {focus}</div>{body}</div>"
        )
    return "".join(blocks) or "<p class='muted'>(no active monologue streams)</p>"


def _find_character_memory(world, name: str):
    """Look up a character's mind by name (case-insensitive), or None."""
    mems = world.character_memories
    if name in mems:
        return mems[name]
    for key in mems.keys():
        if key.lower() == name.lower():
            return mems[key]
    return None


@router.get("/graph.dot", response_class=Response)
def graph_dot():
    story, scene = _load_saved_state()
    if scene is None:
        return Response("// no active scene", media_type="text/plain", status_code=404)
    return Response(story.world().world_graph.to_dot(), media_type="text/vnd.graphviz")


@router.get("/graph.svg", response_class=Response)
def graph_svg():
    story, scene = _load_saved_state()
    if scene is None:
        return Response("no active scene", media_type="text/plain", status_code=404)
    return _dot_to_svg_response(story.world().world_graph.to_dot())


@router.get("/characters")
def characters_endpoint():
    """List characters with a mind: core, streams, and factual beliefs."""
    story, scene = _load_saved_state()
    if scene is None:
        return {"error": "no active scene"}
    characters = []
    for name, mem in story.world().character_memories.items():
        snap = _mind_snapshot(mem)
        characters.append({
            "name": name,
            "core": snap["core"],
            "streams": snap["streams"],
            "beliefs": snap["beliefs"],
        })
    return {"characters": characters}


@router.get("/character/{name}/graph.dot", response_class=Response)
def character_graph_dot(name: str):
    """Graphviz dot of one character's subjective belief graph."""
    story, scene = _load_saved_state()
    if scene is None:
        return Response("// no active scene", media_type="text/plain", status_code=404)
    mem = _find_character_memory(story.world(), name)
    if mem is None:
        return Response(f"// no character '{name}'", media_type="text/plain", status_code=404)
    return Response(mem.beliefs.to_dot(), media_type="text/vnd.graphviz")


@router.get("/character/{name}/graph.svg", response_class=Response)
def character_graph_svg(name: str):
    """Rendered SVG of one character's subjective belief graph."""
    story, scene = _load_saved_state()
    if scene is None:
        return Response("no active scene", media_type="text/plain", status_code=404)
    mem = _find_character_memory(story.world(), name)
    if mem is None:
        return Response(f"no character '{name}'", media_type="text/plain", status_code=404)
    return _dot_to_svg_response(mem.beliefs.to_dot())


@router.get("/character/{name}/mind")
def character_mind_endpoint(name: str):
    """JSON mind snapshot: core, active streams, compact beliefs."""
    story, scene = _load_saved_state()
    if scene is None:
        return {"error": "no active scene"}
    mem = _find_character_memory(story.world(), name)
    if mem is None:
        return {"error": f"no character '{name}'"}
    snap = _mind_snapshot(mem)
    snap["name"] = mem.name
    return snap


@router.get("/minds", response_class=Response)
def minds_endpoint():
    """Debug view: core, monologue streams, beliefs text, and belief graph SVG."""
    story, scene = _load_saved_state()
    if scene is None:
        return Response("<h1>no active scene</h1>", media_type="text/html", status_code=404)

    parts = [
        "<html><head><meta charset='utf-8'><title>Character minds</title><style>",
        "body{background:#1e1e2e;color:#cdd6f4;font-family:'Segoe UI',sans-serif;margin:24px}",
        "h1{color:#cdd6f4} h2{color:#f9e2af;border-bottom:1px solid #45475a;padding-bottom:4px}",
        "h3{color:#89b4fa;font-size:14px;margin:16px 0 6px;text-transform:uppercase;"
        "letter-spacing:.04em}",
        ".core,.beliefs{color:#a6adc8;white-space:pre-wrap;margin:8px 0 12px;"
        "padding:10px;background:#313244;border-radius:6px}",
        ".stream{margin:8px 0 12px;padding:10px;background:#313244;border-radius:6px;"
        "border-left:3px solid #cba6f7}",
        ".stream-head{color:#cba6f7;margin-bottom:6px}",
        ".lines{margin:0;padding-left:18px;color:#cdd6f4}",
        ".lines .turn{color:#6c7086;margin-right:6px}",
        ".muted{color:#6c7086;font-style:italic}",
        ".mind{margin-bottom:48px}",
        ".graphwrap{overflow:auto;max-height:85vh;border:1px solid #45475a;"
        "border-radius:6px;resize:vertical}",
        "svg{display:block;background:#1e1e2e}",
        ".legend{font-size:12px;color:#6c7086;margin-bottom:24px}",
        "</style></head><body><h1>Character minds</h1>",
        "<p class='legend'>Core = continuity sheet (not thoughts) &nbsp;|&nbsp; "
        "Streams = rendered subtext &nbsp;|&nbsp; "
        "Graph: green = live belief/perception &nbsp;|&nbsp; "
        "blue = superseded &nbsp;|&nbsp; yellow = foreshadowed</p>",
    ]
    for name, mem in story.world().character_memories.items():
        snap = _mind_snapshot(mem)
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

        core_html = (
            html.escape(snap["core"])
            if snap["core"] else "(empty continuity sheet)"
        )
        beliefs_html = html.escape(snap["beliefs"])
        parts.append(
            f"<div class='mind'><h2>{html.escape(name)}</h2>"
            f"<h3>Core</h3><div class='core'>{core_html}</div>"
            f"<h3>Monologue streams "
            f"({html.escape(str(snap.get('active_stream_count', 0)))} active)</h3>"
            f"{_format_streams_html(snap['streams'])}"
            f"<h3>Factual beliefs</h3><div class='beliefs'>{beliefs_html}</div>"
            f"<h3>Belief graph</h3>"
            f"<div class='graphwrap'>{svg}</div></div>"
        )
    parts.append("</body></html>")
    return Response("".join(parts), media_type="text/html")


@router.get("/scenes")
def scenes_endpoint():
    """Observability: every live storyline with the scheduler's view of it."""
    story = _load_saved_story()
    try:
        rows = json.loads(story.tool_list_scenes())
    except (json.JSONDecodeError, TypeError):
        rows = []
    return {
        "active": story.active_scene_id,
        "beat_clock": story.beat_clock,
        "scenes": rows,
    }


@router.get("/analyze")
def analyze_endpoint():
    story, scene = _load_saved_state()
    if scene is None:
        return {"error": "no active scene"}
    a = analyze_graph(story.world().world_graph)
    return {
        "live_node_count": a.live_node_count,
        "active_edge_count": a.active_edge_count,
        "orphan_count": a.orphan_count,
    }


@router.post("/weave")
def weave_endpoint():
    story, scene = _load_saved_state()
    if scene is None:
        return {"error": "no active scene"}
    story.set_weaver_llm_callback(make_weaver_callback())
    result = story.weave_scene(scene.scene_id)
    story.save(SAVES_DIR)
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
