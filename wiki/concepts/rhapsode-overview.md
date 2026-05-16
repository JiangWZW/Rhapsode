---
sources:
  - AGENTS.md
  - README.md
last_updated: 2026-05-12
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

A player connects through a web UI, types actions in natural language, and receives narrative responses from an LLM-powered world. A C++ engine manages game state. A Director tracks plot nodes and injects context into the LLM prompt. A memory system ensures the world remembers what happened and what it meant.

A typical turn:

1. Player types: *"I ask the barkeep about the knight in the corner."*
2. The C++ SceneLoop appends the message to history.
3. The Director evaluates the active node pool and injects context hints into the prompt — *"The barkeep is nervous about his debt."*
4. The memory system retrieves relevant established facts.
5. The prompt builder assembles system prompt + character list + director context + recent history.
6. Gemini generates the narrative response.
7. The response is appended to history, pushed to the frontend, and the Director processes any new plot nodes from this turn.
8. New facts are distilled, scored, and stored in the memory system.

## Architecture at a glance

| Layer | Technology | Role |
|-------|------------|------|
| **Core** | C++17 | Scene state, history, node pool, Director logic, SceneLoop FSM, memory scoring/retrieval |
| **Bindings** | pybind11 | Exposes the entire C++ API to Python |
| **Server** | FastAPI + Python | WebSocket endpoint, Gemini LLM client, Chroma vector store, embedding model, local LLM for memory pipeline |
| **Frontend** | Vue 3 + TypeScript | Chat UI with WebSocket connectivity |

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

- C++ core: SceneMessage, History, Character, Scene, Node, NodePool, Director, SceneLoop, MemorySystem — all with save/load, Director integration, and hybrid retrieval
- Python server: FastAPI WebSocket, Gemini client, Chroma vector store with BAAI/bge-base-en-v1.5 embeddings, spaCy lemmatization for BM25, local llama.cpp for memory quality pipeline
- Vue 3 frontend: chat view with message list, input bar, connection status
- Scenario system with seed nodes and save/load

**Not yet built (planned):**

- Plot graph as a full DAG with edges and trigger predicates — the Node/NodePool is a flat pool without edges
- Session layer for multi-scene concurrency
- GitStore for graph history
- World-background loop (off-screen NPC events)
- Input mode spectrum: constrained choices at graph nodes
- Per-NPC memory and information asymmetry
- Visual plot graph editor

## History

Renamed from DigitalDream to Rhapsode.
