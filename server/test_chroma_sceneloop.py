"""Manual ChromaDB smoke test through the production Story API."""

import json

import chromadb

from rhapsode._core import MemorySystem, Story
from rhapsode.memory import register_callbacks, warmup_model
from rhapsode.validator import make_local_llm_callback


SCENARIO = "scenarios/siege.json"
SAVES_DIR = "saves"
CHROMA_PATH = "./chroma"
turn_count = 0


def stub_llm(_prompt: str) -> str:
    return '"I have nothing to say right now."'


def stub_narrator(scene_id: str, _instructions: str, _turn_state: str) -> str:
    global turn_count
    turn_count += 1
    plan = {
        "transitions": [],
        "new_nodes": [],
        "speech_turns": [],
        "new_characters": [],
        "active_cast": ["Sergeant Maren"],
    }
    return (
        f"Test narration for {scene_id} turn {turn_count}.\n"
        "<<<RHAPSODE_JSON>>>\n" + json.dumps(plan)
    )


def main() -> None:
    print(f"chromadb {chromadb.__version__}")
    warmup_model()

    story = Story.load_scenario(SCENARIO)
    scene = story.active_scene()
    memory = MemorySystem(scene.scene_id)
    register_callbacks(memory, scene.scene_id, CHROMA_PATH)
    story.set_memory(memory)

    resuming = story.has_save(SAVES_DIR)
    if resuming:
        story.load_save(SAVES_DIR)
        scene = story.active_scene()

    nodes = story.world().world_graph.all_nodes_including_expired()
    for node in nodes:
        if node.id:
            memory.store_node(
                node.id, node.fact, node.state.name.lower(), node.type,
                node.created_at,
            )
    expired = [node for node in nodes if node.valid_until != -1]
    if expired:
        memory.sync_expired(expired)

    story.set_reflection_llm_callback(make_local_llm_callback())
    story.set_llm_callback(stub_llm)
    story.set_narrator_llm_callback(stub_narrator)
    story.set_weaver_llm_callback(stub_llm)
    story.set_weaver_local_llm_callback(make_local_llm_callback())
    story.set_downsampler_callback(make_local_llm_callback())
    story.set_resuming(resuming)
    story.set_saves_dir(SAVES_DIR)

    for text in (
        "I look around the courtyard.",
        "I check the eastern gate.",
        "I speak with Maren.",
        "I look at the sky.",
    ):
        outputs = story.advance_scene(text)
        print(f"turn={story.active_scene().turn_index}: {len(outputs)} output(s)")

    print("Story/ChromaDB smoke test complete")


if __name__ == "__main__":
    main()
