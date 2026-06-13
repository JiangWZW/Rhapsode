---
sources:
  - AGENTS.md
  - README.md
last_updated: 2026-05-17
confidence: verified
tier: semantic
related:
  - "[[concepts/narrative-philosophy]]"
  - "[[architecture/system-overview]]"
  - "[[architecture/stack]]"
  - "[[decisions/coding-guidelines]]"
tags:
  - design
---

# Rhapsode — overview

**Rhapsode** is an AI text RPG engine built on a specific belief: the LLM is a world simulator, not a storyteller.

The name comes from the ancient Greek *rhapsōidos* — a performer who arranged existing oral traditions into great stories. Homer did not invent Achilles or Troy; he arranged the fragments into the Iliad. Rhapsode's architecture mirrors this: the LLM generates raw dramatic material, the Director structures and arranges it, and the LLM performs it as prose. The name is the architectural thesis.

## What Rhapsode does

A player connects through a web UI, types actions in natural language, and receives narrative responses from an LLM-powered world. A C++ engine manages game state. A Director tracks plot nodes in a directed graph and injects context into a merged LLM prompt. A memory system ensures the world remembers what happened and what it meant. NPC dialogue is synthesized by a local model from speech cues.

A typical turn:

1. Player types: *"I ask the barkeep about the knight in the corner."*
2. The C++ SceneLoop appends the message to history.
3. The Director builds a focus payload — the current WorldGraph nodes plus a 2-hop neighborhood of relevant facts.
4. The memory system retrieves established facts for contradiction prevention.
5. The prompt builder assembles a merged prompt: system prompt + narrative frame + graph rules + speech cues + active characters + established facts + Director context + graph snapshot + recent history.
6. A single LLM call (Gemini or DeepSeek) produces narrative prose followed by structured JSON containing graph updates and NPC speech cues.
7. The response is split: prose becomes the narrator message, JSON is applied to the WorldGraph by the Director.
8. NPC dialogue lines are generated from the speech cues by a local llama.cpp model.
9. All messages (narrator + character lines) are pushed to the frontend and appended to history.
10. New facts are distilled, scored, and stored in the memory system.

## Architecture at a glance

| Layer | Technology | Role |
|-------|------------|------|
| **Core** | C++17 | Scene state, history, WorldGraph (Boost.Graph with typed edges), Director logic, SceneLoop FSM, memory scoring/retrieval |
| **Bindings** | pybind11 | Exposes the entire C++ API to Python |
| **Server** | FastAPI + Python | WebSocket endpoint, multi-provider LLM (Gemini/DeepSeek), merged prompt builder, Chroma vector store, embedding model, local LLM for memory pipeline + NPC dialogue |
| **Frontend** | Vue 3 + TypeScript | Panel-based UI with story/status/conversation panels, markdown-it scene parser |

See [stack](../architecture/stack.md) for the full layer diagram and dependency tables.

## Core beliefs

These principles constrain every design decision. See [narrative philosophy](narrative-philosophy.md) for the full treatment.

1. **The Director is a rhapsode.** It arranges LLM-generated fragments, never puppeteers the story. The LLM composes raw material; the Director structures it; the LLM performs it as prose.
2. **Long-term memory is the emotional backbone.** Weighted facts with quality scores and entity links — not equal-weight text chunks.
3. **Ambiguity is depth.** No fortune tracker. The arc emerges from accumulated memory and active plot nodes.
4. **The LLM simulates, it does not decide.** Two roles: composer (dramatic potential) and performer (rendering the moment). It never manages structure.
5. **The player breaks everything.** Elastic arc: steer consequences, not actions.

## Current state

**Built and working:**

- C++ core: SceneMessage, History, Character, Scene, Node, WorldGraph (directed graph with typed edges — Related, Supersedes, Contradicts, CausedBy), Director (merged-prompt flow with focus payload + apply_planned_turn), SceneLoop (single LLM call, character synthesis), MemorySystem — all with save/load and hybrid retrieval
- Python server: FastAPI WebSocket, multi-provider LLM (Gemini + DeepSeek via llm.py), merged narrator+graph prompt with `<<<RHAPSODE_JSON>>>` splitting, character synthesis via local llama.cpp, Chroma vector store with BAAI/bge-base-en-v1.5 embeddings, spaCy lemmatization for BM25, local llama.cpp for memory quality pipeline
- Vue 3 frontend: panel-based layout (StatusPanel, StoryPanel, ConversationPanel), markdown-it scene text parser with custom dialogue/bracket/paren rules, scene_message protocol with scene_kind distinction
- Scenario system with seed nodes and save/load (including graph edges)

**Not yet built (planned):**

- Trigger predicates on graph edges — the WorldGraph has typed directed edges, but transitions are still LLM-driven rather than predicate-based
- Session layer for multi-scene concurrency
- GitStore for graph history
- World-background loop (off-screen NPC events)
- Input mode spectrum: constrained choices at graph nodes
- Per-NPC memory and information asymmetry
- Visual plot graph editor

## History

Renamed from DigitalDream to Rhapsode.
