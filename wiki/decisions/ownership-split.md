---
sources:
  - core/include/rhapsode/scene_loop.h
  - core/include/rhapsode/memory_system.h
  - server/rhapsode/app.py
last_updated: 2026-05-12
confidence: verified
tier: episodic
related:
  - "[[architecture/system-overview]]"
  - "[[decisions/callback-vs-pull]]"
tags:
  - cross-layer
---

# Decision: ownership split (C++ vs Python)

**Status:** accepted

## Context

Rhapsode uses a C++ core with Python for I/O and LLMs. A clear boundary prevents repeating Talemate's tightly coupled orchestration.

## Decision

| Concern | Owner | Rationale |
|---------|-------|-----------|
| Turn-by-turn SceneLoop FSM | C++ | Self-contained, testable, no I/O |
| Scene / History / NodePool (canonical state) | C++ | Serialization, fast access |
| Director control flow + node management | C++ | Deterministic logic, applies transitions |
| Director LLM call | Python (callback) | HTTP to Gemini, response parsing |
| Memory scoring + retrieval logic | C++ | BM25, entity boost, dedup — deterministic |
| Memory storage (Chroma, embeddings) | Python (callbacks) | Python-native libraries |
| Memory quality pipeline (distill, score, extract) | C++ orchestration, Python LLM callback | C++ sequences the steps; Python provides the local LLM HTTP calls |
| Prompt assembly | Python | Template flexibility, evolves independently of C++ |
| LLM HTTP + API keys | Python | Vendor SDKs, streaming, credential management |
| Lemmatization | Python (callback) | spaCy is Python-native |

## Key principle

C++ owns **control flow and logic**. Python provides **I/O services via callbacks**. Business logic is never duplicated — each concern has exactly one owner.

## Consequences

- The pybind API exposes callback registration (`set_*_callback`) rather than data exchange. C++ calls Python; Python does not poll C++.
- Prompt templates can evolve without recompiling C++.
- The callback boundary is consistent: SceneLoop (3 callbacks), Director (2 callbacks), MemorySystem (7 callbacks). All follow the same `std::function` pattern.
- The local LLM (llama.cpp) is accessed through the same callback pattern — C++ does not know about HTTP.

## Alternatives considered

- **Python-owned loop** — faster iteration but risks logic sprawl in handlers. Rejected.
- **Full C++ LLM HTTP** — rejected for MVP (keys, streaming, vendor churn).
- **hnswlib in C++ for vector search** — replaced by ChromaDB in Python for faster prototyping. May revisit for performance.
