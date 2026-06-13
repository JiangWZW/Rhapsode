---
last_updated: 2026-05-19
---

# Rhapsode wiki — index

Master catalog of wiki pages. Update when adding or removing pages.

## Overview

- [Rhapsode overview](concepts/rhapsode-overview.md) — what it is, architecture at a glance, current state
- [LLM Wiki pattern](concepts/llm-wiki-pattern.md) — Karpathy gist adapted for this repo

## Concepts

- [Narrative philosophy](concepts/narrative-philosophy.md) — six design principles, lessons from Talemate

## Architecture

- [System overview](architecture/system-overview.md) — subsystems, control flow, engineering constraints, implementation status
- [Stack](architecture/stack.md) — layers, repo layout, build system, dependencies
- [C++ data model](architecture/cpp-data-model.md) — all C++ types: SceneMessage, History, Character, Node, WorldGraph, Director, SceneLoop, MemorySystem, Scene
- [Scene loop](architecture/scene-loop.md) — FSM states, Director integration, history windowing, callbacks
- [Plot graph](architecture/plot-graph.md) — Node/WorldGraph with typed edges (implemented), trigger predicates (planned), F-T-P triples
- [Memory system](architecture/memory-system.md) — hybrid retrieval, quality pipeline, C++/Python callback split
- [Python server](architecture/python-server.md) — FastAPI, WebSocket protocol, Gemini client, memory callbacks
- [Vue frontend](architecture/vue-frontend.md) — component tree, WebSocket store, styling
- [MVP v0](architecture/mvp-v0.md) — original criteria, what was built beyond MVP, deviations from plan
- [Companion system](architecture/companion-system.md) — protagonist companion design: L3 (LoRA identity) + L2 (observe-reflect-plan memory), implementation order, unknowns
- [Character system](architecture/character-system.md) — persona (`Character`) + per-character mind (`CharacterMemory`): memory graph, retrieval, reflection, persistent first-person self-state, and the two prompts that render dialogue
- [Subjective character minds](architecture/subjective-character-minds.md) — design of record (proposed): re-found `CharacterMemory` as a composed subjective `WorldGraph`, fed by narrator-routed perception; three layers truth/perception/interpretation

## Episodes

- [Freedom, tension, and the two axes](episodes/2026-06-08-freedom-tension-and-the-two-axes.md) — design session: coherence vs tension axes, freedom as the complement of the live constraint set, validator `supersede`, the Director-as-tension-layer
- [Pull vs. push, and the three growth directions](episodes/2026-06-09-pull-vs-push-and-three-growth-directions.md) — the engine is 100% pull; adding push via parallel scenes / backfill / ingestion as fact producers through one coherence gate
- [Parallel scenes — shared pool, LOD, lifecycle](episodes/2026-06-10-parallel-scenes-shared-pool-lod-lifecycle.md) — implementation plan: shared pool of facts+minds, scenes as loops, LOD by player-anchored character importance, scene-thread spawn/destroy lifecycle, phased build (0—)

## Research

- [Literature review](research/literature-review.md) — 8 papers from awesome-llm-story-generation, adopted ideas, confirmed gaps
- [Memory systems survey](research/memory-systems-survey.md) — MemGPT, Graphiti, AriGraph, A-Mem, Mem0 v3
- [Memory systems internals](research/memory-systems-internals.md) — Mem0 v3 and A-Mem implementation details
- [Narrator & mind weak points](research/narrator-and-mind-weakpoints.md) — analysis of an 18-turn siege session: the narrator has no dramaturgical spine, off-stage minds can't act, plus the `self_state` leak and reflection bugs
- [Subplot lifecycle — craft research](research/subplot-lifecycle-craft-research.md) — WHEN and HOW to start/advance/end subplots: synthesis of writing craft (McKee, Truby, Snyder, Bell, Sanderson, Weiland, Laws), RPG design (Apocalypse World fronts, DramaSystem, Lazy DM, Alexandrian), and interactive narrative systems (storylets/QBN, Valve dialogue, RimWorld pacing, Façade)
- [LLM role-playing survey](research/llm-roleplay-survey.md) — tuning LLMs for game characters, 19 papers reviewed with quality assessment (three waves: surface imitation, modular approaches, cognitive simulation)

### Foundational and adjacent research

- [Generative Agents](research/papers/generative-agents.md) — observe-reflect-plan NPC memory architecture (**A**, Stanford+Google, UIST 2023, 4,781 citations)
- [Representation Engineering](research/papers/representation-engineering.md) — foundation for all activation steering methods (**A**, CAIS+CMU+Berkeley+Stanford, 988 citations)

### Role-playing papers — fine-tuning approaches

- [Character-LLM](research/papers/character-llm.md) — per-character SFT via Experience Reconstruction (**A**, Fudan, EMNLP 2023)
- [RoleLLM](research/papers/rolellm.md) — 4-stage framework, RoleBench benchmark (**B**, Beihang+HKUST+CAS, ACL 2024)
- [DITTO](research/papers/ditto.md) — self-alignment for role-play, WikiRoleEval (**A**, Alibaba, ACL 2024)
- [CoSER](research/papers/coser.md) — 17K characters from 771 books (**B**, Fudan+JHU, ICML 2025)
- [Neeko](research/papers/neeko.md) — dynamic LoRA for multi-character serving (**C**, BIT, EMNLP 2024)
- [CharacterGLM](research/papers/characterglm.md) — attribute/behavior decomposition (**B**, Tsinghua+Zhipu, EMNLP 2024)
- [Codifying Character Logic](research/papers/codified-profiles.md) — executable character profiles, 1B models can role-play (**B**, UCSD, NeurIPS 2025)
- [Thinking in Character (RAR)](research/papers/rar-thinking-in-character.md) — role-aware reasoning via internal monologue distillation + style DPO (**B**, HIT-SZ+Baidu, NeurIPS 2025)

### Role-playing papers — cognitive simulation (third wave)

- [HER](research/papers/her-dual-layer-thinking.md) — dual-layer thinking (system + role) with RL and trained generative reward model; state-of-the-art (**A**, Fudan+MiniMax, 2026)
- [HumanLLM](research/papers/humanllm.md) — 244 psychological patterns as interacting causal forces; 8B beats 32B on multi-pattern dynamics (**A**, Fudan+JHU, ACL 2026)
- [Character-R1](research/papers/character-r1.md) — 10-dimension cognitive focus with GRPO verifiable rewards; validated on Qwen2.5-7B (**B**, HIT-SZ+Baidu, 2026)

### Role-playing papers — alternative modular approaches

- [Open Character Training](research/papers/open-character-training.md) — Constitutional AI for persona shaping (**A**, Cambridge+AI2+Anthropic, 2025)
- [CAST](research/papers/cast-activation-steering.md) — conditional activation steering, IBM library (**A**, ICLR 2025 Spotlight)
- [ChatHaruhi](research/papers/chatharuhi.md) — RAG-based character system, local Qwen support (**C**, community project, 2K stars)
- [PERSONA](research/papers/persona-steering.md) — training-free personality via activation vector algebra (**D**, HIT, ICLR 2026)
- [RoleRAG](research/papers/rolerag.md) — graph-guided retrieval with cognitive boundaries (**D**, NTU, arXiv 2025)

## External Extensions & Reference Implementations

- [Summaryception analysis](research/summaryception-analysis.md) — recursive layered summarization for SillyTavern; architecture, pipeline, connection routing, layered memory model
- [Generative Agents code analysis](research/generative-agents-code-analysis.md) — Stanford's observe-reflect-plan cognitive architecture; memory stream, three-factor retrieval, importance-triggered reflection

## Talemate analysis

- [Talemate index](talemate/_index.md) — external reference analysis of Talemate's memory subsystems
- [Memory architecture](talemate/memory-architecture.md) — ChromaDB setup, embedding backends, metadata schema
- [Retrieval pipeline](talemate/retrieval-pipeline.md) — multi-query algorithm, RAG mixin, three modes
- [Summarization](talemate/summarization.md) — archive cascade, layered history, context_history budget
- [Reinforcements](talemate/reinforcements.md) — Q&A refresh, context pins, decay
- [Context assembly](talemate/context-assembly.md) — token budgets, section ordering, Jinja templates
- [Comparison](talemate/comparison.md) — Rhapsode vs Talemate feature matrix and roadmap

## Decisions

- [Ownership split (C++ vs Python)](decisions/ownership-split.md) — who owns what, callback boundary
- [Callback vs pull pattern](decisions/callback-vs-pull.md) — how SceneLoop invokes Python
- [Coding guidelines](decisions/coding-guidelines.md) — Karpathy principles: simplicity, surgical changes, goal-driven

## Meta

- [Wiki schema](SCHEMA.md) — conventions, frontmatter, lifecycle, quality standards
- [Agent contract](../AGENTS.md) — how agents maintain the wiki
- [Changelog / log](log.md)
- [Raw sources catalog](../raw/sources.md)
