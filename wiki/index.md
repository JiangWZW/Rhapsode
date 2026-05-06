# Rhapsode wiki — index

Master catalog of wiki pages. Update when adding or removing pages.

## Overview

- [[concepts/rhapsode-overview|Rhapsode overview]]
- [[concepts/llm-wiki-pattern|LLM Wiki pattern]] — Karpathy gist adapted for this repo

## Architecture

- [[architecture/stack|Stack]] — layers, repo layout, build system, dependencies
- [[architecture/mvp-v0|MVP v0]] — acceptance criteria, task breakdown, sequence diagram, scenario schema
- [[architecture/scene-loop|SceneLoop]] — FSM states, C++ class sketch, callback interface
- [[architecture/cpp-data-model|C++ data model]] — SceneMessage, History, Character, Scene structs + JSON contract
- [[architecture/python-server|Python server]] — FastAPI, WebSocket protocol, LLM clients, prompt builder
- [[architecture/vue-frontend|Vue frontend]] — component tree, WebSocket store, styling

- [[architecture/system-overview|System overview]] — four subsystems, control flow, engineering constraints
- [[architecture/plot-graph|Plot graph]] — plot node DAG, F-T-P triples, knowledge state, generation pipeline

## Concepts

- [[concepts/narrative-philosophy|Narrative philosophy]] — six design principles, lessons from Talemate

## Research

- [[research/literature-review|Literature review]] — 8 papers from awesome-llm-story-generation, adopted ideas, confirmed gaps

## Decisions

- [[decisions/ownership-split|Ownership split (C++ vs Python)]]
- [[decisions/callback-vs-pull|Callback vs pull pattern]] — how SceneLoop invokes Python
- [[decisions/coding-guidelines|Coding guidelines]] — Karpathy principles: simplicity, surgical changes, goal-driven

## Meta

- [Agent contract / schema](../AGENTS.md)
- [[log|Changelog / log]]
- [Raw sources catalog](../raw/sources.md)
