---
last_updated: 2026-05-12
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
- [C++ data model](architecture/cpp-data-model.md) — all C++ types: SceneMessage, History, Character, Node, NodePool, Director, SceneLoop, MemorySystem, Scene
- [Scene loop](architecture/scene-loop.md) — FSM states, Director integration, history windowing, callbacks
- [Plot graph](architecture/plot-graph.md) — Node/NodePool (implemented), DAG vision (planned), F-T-P triples, knowledge state
- [Memory system](architecture/memory-system.md) — hybrid retrieval, quality pipeline, C++/Python callback split
- [Python server](architecture/python-server.md) — FastAPI, WebSocket protocol, Gemini client, memory callbacks
- [Vue frontend](architecture/vue-frontend.md) — component tree, WebSocket store, styling
- [MVP v0](architecture/mvp-v0.md) — original criteria, what was built beyond MVP, deviations from plan

## Research

- [Literature review](research/literature-review.md) — 8 papers from awesome-llm-story-generation, adopted ideas, confirmed gaps
- [Memory systems survey](research/memory-systems-survey.md) — MemGPT, Graphiti, AriGraph, A-Mem, Mem0 v3
- [Memory systems internals](research/memory-systems-internals.md) — Mem0 v3 and A-Mem implementation details

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
