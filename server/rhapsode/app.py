import asyncio
import json
import logging
import os
import pathlib
from contextlib import asynccontextmanager

from dotenv import load_dotenv
from fastapi import FastAPI, WebSocket, WebSocketDisconnect

from rhapsode._core import Director, MemorySystem, NodeState, Scene, SceneLoop, SceneMessage
from rhapsode.gemini import complete
from rhapsode.memory import register_callbacks, warmup_model
from rhapsode.prompt import build_prompt
from rhapsode.spacy_models import get_nlp_lemma
from rhapsode.validator import make_local_llm_callback

load_dotenv()
log = logging.getLogger(__name__)

_server_dir = pathlib.Path(__file__).resolve().parent.parent
SCENARIO_PATH = _server_dir / os.environ.get("RHAPSODE_SCENARIO", "scenarios/tavern.json")
SAVES_DIR = str(_server_dir / "saves")

DIRECTOR_SYSTEM_PROMPT = """\
You are the narrative director for a text RPG. Input is JSON with nodes and scene context.
Return ONLY valid JSON with two keys:

  transitions: [{{"id": <number>, "state": "dormant|foreshadowed|active|resolved"}}]
  new_nodes:   [{{"fact": <string>, "type": <string>,
                 "state": "dormant|foreshadowed|active|resolved",
                 "foreshadow_ctx": <string>, "active_ctx": <string>,
                 "known_by": [<string>]}}]

PLAYER AGENCY (strict):
- NEVER generate facts describing Player actions the Player has not taken.
- Only the Player's own words in scene_context determine what the Player does.
- You MAY foreshadow Player options (state: "foreshadowed") but NEVER assert them as active or resolved.
- You MAY create NPC actions, world consequences, and environmental changes freely.

NARRATIVE DIRECTION:
- Prefer tension over resolution. Do NOT resolve threats the same turn they appear.
- Use "foreshadowed" state to let threats simmer for 2-3 turns before activation.
- Introduce complications, twists, and reversals — not just linear escalation.
- NPCs should have their own motivations, secrets, and agendas that create dramatic irony.
- Max 2-3 new nodes per turn. Develop existing threads before spawning new ones.
- When the graph has many active nodes, focus on connecting and resolving existing threads.
- active_ctx and foreshadow_ctx should be vivid and atmospheric, not dry summaries.

FACT FORMAT (strict):
- One atomic assertion per node. Two facts = two nodes.
- Max 15 words. Shorter is better.
- No articles (the, a, an) unless grammatically required.
- No hedging (seems, probably, might, apparently).
- No compound sentences (no "and", "which", "because" joining clauses).
- Numbers as digits.

GOOD: "barkeep owes thieves guild 200g"
BAD:  "The barkeep finally reveals that he owes money to the guild"

NODE QUALITY RULES:
- Each fact must name at least one entity.
- type: one of "plot", "scene", "world", "relationship".
- No vague facts ("The situation escalates"), no meta-commentary.

{established_facts}"""


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


def _extract_json_object(text: str) -> str:
    start = text.find("{")
    end = text.rfind("}")
    if start >= 0 and end > start:
        return text[start : end + 1]
    return text


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
    active_names = set()
    for node in scene_obj.world_graph.all_nodes():
        if node.state != NodeState.Active:
            continue
        for ent in node.entities:
            ent_l = ent.lower()
            for key, display in char_lookup.items():
                if key in ent_l or ent_l in key:
                    active_names.add(display)
    return sorted(active_names)


def _established_facts(memory: MemorySystem, history: list[SceneMessage], scene_obj: Scene, director_out) -> list[str]:
    try:
        raw = memory.retrieve_for_injection(_build_memory_query(history, scene_obj, director_out), 8)
        facts = json.loads(raw)
        if isinstance(facts, list):
            return [str(f) for f in facts]
    except Exception:
        log.exception("Failed to retrieve narrator established facts")
    return []


def _wire_loop(scene: Scene, director: Director, memory: MemorySystem, on_turn_complete) -> SceneLoop:
    loop = SceneLoop()
    loop.load_scene(scene)
    loop.set_director(director)

    loop.set_prompt_callback(
        lambda history, scene_obj, director_out: build_prompt(
            history,
            scene_obj,
            director_out,
            established_facts=_established_facts(memory, history, scene_obj, director_out),
            active_characters=_active_characters(scene_obj),
        )
    )
    loop.set_llm_callback(_call_llm)
    loop.set_turn_complete_callback(on_turn_complete)
    return loop


async def _send_seed_messages(ws: WebSocket, scene: Scene, is_resuming: bool = False):
    if is_resuming:
        recent = scene.history.snapshot(6)
        for msg in recent:
            await ws.send_json({"type": "assistant_message", "content": msg.content})
    else:
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

    scene = Scene.load_json(str(SCENARIO_PATH))
    memory = _init_memory(scene.scene_id)
    scene.set_memory(memory)

    is_resuming = scene.has_save(SAVES_DIR)
    if is_resuming:
        scene.load_save(SAVES_DIR)
        log.info("=== SESSION RESUMED from save ===")
        log.info("  scene_id=%s, turn=%d, graph=%d nodes, history=%d msgs",
                 scene.scene_id, scene.turn_index, scene.world_graph.size(),
                 scene.history.size())
    else:
        log.info("=== FRESH START from scenario ===")
        log.info("  scene_id=%s, graph=%d seed nodes, history=%d seed msgs",
                 scene.scene_id, scene.world_graph.size(), scene.history.size())

    director = Director(scene.world_graph)

    response_text: str | None = None

    def on_turn_complete(msg: SceneMessage):
        nonlocal response_text
        response_text = msg.content

    def director_llm_cb(prompt_json: str) -> str:
        established_json = memory.retrieve_for_injection(prompt_json)
        established = json.loads(established_json)
        facts_block = ""
        if established:
            lines = [f"- {f}" for f in established]
            facts_block = (
                "=== ESTABLISHED FACTS (do NOT contradict) ===\n"
                + "\n".join(lines)
                + "\n===\n"
            )
        prompt = DIRECTOR_SYSTEM_PROMPT.format(established_facts=facts_block)
        full_prompt = f"{prompt}\n\nInput JSON:\n{prompt_json}"
        raw = _call_llm(full_prompt).strip()
        extracted = _extract_json_object(raw)
        try:
            parsed = json.loads(extracted)
            return json.dumps(parsed)
        except json.JSONDecodeError:
            log.warning("Director LLM returned invalid JSON: %s", extracted[:200])
            return '{"transitions":[],"new_nodes":[]}'

    director.set_llm_callback(director_llm_cb)
    loop = _wire_loop(scene, director, memory, on_turn_complete)
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
            response_text = None

            try:
                await asyncio.get_event_loop().run_in_executor(
                    None, loop.submit_input, text
                )
            except Exception as exc:
                log.exception("Turn failed")
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                loop = _wire_loop(scene, director, memory, on_turn_complete)
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

            if response_text:
                await ws.send_json({"type": "assistant_message", "content": response_text})
            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        pass
