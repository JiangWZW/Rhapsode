"""The play session: construct the engine and its Python callbacks, then run the
/ws loop. All turn sequencing -- player beat, scheduler, off-stage beat, memory
sync, persistence -- lives in the C++ engine behind `Story.advance_scene`; this
module only builds the pieces, streams output, and handles the socket."""

import asyncio
import json
import logging
import re
from dataclasses import dataclass

from fastapi import WebSocket, WebSocketDisconnect

from rhapsode._core import (
    Annotator, MemorySystem, SceneData, SceneMessage, Story,
)
from rhapsode.config import SAVES_DIR, SCENARIO_PATH
from rhapsode.llm_tools import call_llm, make_narrator_callback
from rhapsode.scheduler import make_scheduler_callback
from rhapsode.lifecycle import make_lifecycle_callback
from rhapsode.memory import register_callbacks
from rhapsode.fable import make_ner_callback
from rhapsode.validator import make_local_llm_callback

log = logging.getLogger(__name__)


# -- Construction --------------------------------------------------------------

def _init_memory(scene_id: str) -> MemorySystem:
    memory = MemorySystem(scene_id)
    register_callbacks(memory, scene_id)
    return memory


def _sync_graph_to_memory(story: Story, memory: MemorySystem) -> None:
    """Ensure all graph nodes are indexed in ChromaDB (catches seed nodes from scenario load).

    Uses upsert so it's safe to call on every startup — already-indexed nodes
    are overwritten with the same data.  ~30 nodes embeds in 1-2 seconds.
    """
    all_nodes = story.world().world_graph.all_nodes_including_expired()
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


def _configure_story(story: Story, *, resuming: bool) -> None:
    """Configure callbacks on the Story-owned native runtime."""
    story.set_llm_callback(call_llm)
    story.set_narrator_llm_callback(make_narrator_callback())
    story.set_weaver_llm_callback(call_llm)
    story.set_weaver_local_llm_callback(make_local_llm_callback())
    story.set_resuming(resuming)
    story.set_scheduler_callback(make_scheduler_callback())
    story.set_lifecycle_callback(make_lifecycle_callback())
    story.set_downsampler_callback(make_local_llm_callback(repair_json=False))
    story.set_saves_dir(SAVES_DIR)


@dataclass
class WsSession:
    story: Story
    memory: MemorySystem
    annotator: Annotator
    is_resuming: bool

    @property
    def scene(self) -> SceneData:
        """The player's active scene."""
        return self.story.active_scene()


def _setup_ws_session() -> WsSession:
    story = Story.load_scenario(str(SCENARIO_PATH))
    scene = story.active_scene()
    memory = _init_memory(scene.scene_id)
    story.set_memory(memory)

    is_resuming = story.has_save(SAVES_DIR)
    if is_resuming:
        story.load_save(SAVES_DIR)
        scene = story.active_scene()
        log.info(
            "=== SESSION RESUMED === scene_id=%s scenes=%d turn=%d graph=%d hist=%d",
            scene.scene_id, len(story.scene_ids()), scene.turn_index,
            story.world().world_graph.size(), scene.history.size(),
        )
    else:
        log.info(
            "=== FRESH START === scene_id=%s graph=%d hist=%d",
            scene.scene_id, story.world().world_graph.size(), scene.history.size(),
        )

    _sync_graph_to_memory(story, memory)
    # Story keeps reflection configuration outside the persisted World value.
    story.set_reflection_llm_callback(call_llm)

    annotator = Annotator(story.world())
    annotator.set_ner_callback(make_ner_callback())

    _configure_story(story, resuming=is_resuming)

    return WsSession(story=story, memory=memory, annotator=annotator,
                     is_resuming=is_resuming)


# -- Transport -----------------------------------------------------------------

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


async def _send_seed_messages(ws: WebSocket, story: Story,
                              scene: SceneData, is_resuming: bool = False):
    cap = 8 if is_resuming else None
    seeds = story.display_timeline(scene.scene_id, cap)
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


async def _stream_outputs(ws: WebSocket, annotator: Annotator,
                          outputs: list[SceneMessage]) -> None:
    """Send one turn's outputs to the player, annotating narrator prose with NER."""
    for chunk in outputs:
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


async def _handle_undo(ws: WebSocket, session: WsSession, n: int) -> None:
    reverted = await asyncio.get_event_loop().run_in_executor(
        None, session.story.revert_active_turns, n)
    scene = session.scene
    log.info("/undo %d -> reverted %d turns, now at turn %d",
             n, reverted, scene.turn_index)
    await ws.send_json({"type": "undo", "turns_reverted": reverted,
                        "turn_index": scene.turn_index})
    await _send_seed_messages(ws, session.story, scene, is_resuming=True)
    await ws.send_json({"type": "status", "state": "idle"})


async def run_session(ws: WebSocket) -> None:
    """Accept the socket and drive turns until the client disconnects.

    Each turn is a single `story.advance_scene(text)` call: the engine runs the
    player beat, any staged lifecycle ops, one scheduled off-stage beat, memory
    sync, and persistence, and returns the player-facing outputs to stream.
    """
    await ws.accept()

    session = _setup_ws_session()
    await _send_seed_messages(ws, session.story, session.scene, session.is_resuming)

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
                outputs = await asyncio.get_event_loop().run_in_executor(
                    None, session.story.advance_scene, text)
            except Exception as exc:
                log.exception("Turn failed")
                await ws.send_json({"type": "error", "detail": str(exc)})
                await ws.send_json({"type": "status", "state": "idle"})
                continue

            await _stream_outputs(ws, session.annotator, outputs)
            await ws.send_json({"type": "status", "state": "idle"})

    except WebSocketDisconnect:
        pass
