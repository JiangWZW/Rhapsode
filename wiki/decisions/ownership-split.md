# Decision: ownership split (C++ vs Python)

**Status:** accepted  
**Date:** 2026-05-05  

## Context

Rhapsode uses a C++ core with Python for I/O and LLMs. We needed a clear boundary to avoid repeating Talemate's tightly coupled "god" orchestration in Python.

## Decision

| Concern | Owner |
|---------|--------|
| Turn-by-turn **SceneLoop** | **C++** |
| **Prompt text** assembly | **Python** |
| **LLM HTTP / API keys** | **Python** |
| **Scene / history canonical state** | **C++** |

## Consequences

- pybind API must allow the loop to request **prompt generation** and **completion** without duplicating business rules in both languages.
- Prompt templates can evolve quickly in Python without recompiling C++.

## Alternatives considered

- Python-owned loop — faster iteration but risks logic sprawl in handlers.
- Full C++ LLM HTTP — rejected for MVP (keys, streaming, vendor churn).
