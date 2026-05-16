---
sources:
  - raw/karpathy-coding-guidelines.md
last_updated: 2026-05-12
confidence: verified
tier: episodic
related:
  - "[[concepts/llm-wiki-pattern]]"
tags:
  - design
---

# Decision: coding guidelines (Karpathy principles)

**Status:** accepted
**Decided:** early development

## Context

Talemate's codebase grew into a monolith with god objects, bloated abstractions, and tightly coupled systems. Rhapsode must avoid repeating this.

## Decision

Adopt the four principles from the Karpathy-inspired coding guidelines:

1. **Think Before Coding** — state assumptions, surface tradeoffs, ask when unclear.
2. **Simplicity First** — minimum code that solves the problem. No speculative features, no abstractions for single-use code.
3. **Surgical Changes** — touch only what you must. Match existing style.
4. **Goal-Driven Execution** — define success criteria, loop until verified.

Full text: `raw/karpathy-coding-guidelines.md`

## Consequences

- Every file should be short, obvious, and justified.
- No "flexibility" or "configurability" that was not requested.
- Push back on scope creep during planning, not after implementation.

## Retrospective

This guideline influenced the decision to consolidate the Python server into a single `app.py` rather than the planned multi-file structure. The `llm/` subpackage with `BaseLLMClient` ABC was dropped — there is only one LLM provider. The abstraction would have been speculative.
