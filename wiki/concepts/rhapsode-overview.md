# Rhapsode -- overview

**Rhapsode** is an AI-powered text RPG engine built on a specific belief: **the LLM is a world simulator, not a storyteller.**

## Architecture

- **C++** -- scene data, history, characters, and a code-defined **SceneLoop** (FSM). The engine owns state and structure.
- **Python** -- FastAPI, WebSocket, prompt assembly, LLM API calls. Python is the bridge between the engine and the model.
- **Vue 3** -- minimal playable chat UI.

## Core beliefs

See [[narrative-philosophy]] for the full treatment. Summary:

1. **The Director is a rhapsode.** An arranger of narrative fragments, not a puppeteer. The LLM composes raw material; the Director structures it into the plot graph; the LLM performs it as prose.
2. **Long-term memory is the emotional backbone.** Weighted memories, not equal-weight text chunks. What happened, what it meant, what changed.
3. **Ambiguity is depth.** There is no fortune tracker. The arc emerges from accumulated memory + active plot nodes, never computed.
4. **The LLM simulates, it doesn't decide.** It has two roles: composer (generates dramatic potential) and performer (renders the current moment). It never manages structure.
5. **The player breaks everything.** Elastic arc: steer consequences, not actions.

## Design constraints

- [[coding-guidelines|Karpathy coding guidelines]]: simplicity first, no speculative abstractions.
- Thin **Scene coordinator** -- avoid Talemate-style god objects.
- **Native C++ structs** for messages and serialization (JSON).
- **SceneLoop in C++**, prompts and completions from Python.

## Roadmap

- **MVP v0** (done): C++ core, pybind11, FastAPI server, Vue frontend, Gemini integration.
- **Next**: Long-term memory (weighted event log, vector retrieval).
- **Then**: Director + plot graph (DAG of latent plot nodes, deterministic traversal, two loops). See [[plot-graph]].
- **Later**: Visual plot graph editor, scenario authoring tools, save/load sessions.

## Naming

From the ancient Greek *rhapsōidos* -- a performer who arranged existing oral traditions and fused them into great stories. Homer didn't invent Achilles or Troy. He arranged the fragments into the Iliad.

The name is the architectural thesis: the system takes raw dramatic material (LLM-generated) and arranges it into coherent narrative experience. The Director is the rhapsode. The LLM is both the oral tradition (composer) and the voice (performer). The player is the audience who changes the story by participating in it.
