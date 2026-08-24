---
sources:
  - core/include/rhapsode/character_memory.h
  - core/src/character_memory_reflection.cpp
last_updated: 2026-08-24
confidence: verified
tier: semantic
related:
  - "[[architecture/monologue-streams]]"
tags:
  - cpp-core
  - character
---

# Decision: linear untagged monologue

**Status:** accepted
**Decided:** 2026-08-24

## Context

Monologue used 1–5 streams, a roster, fork/merge/conclude ops, and a replaced “What just happened” tail. That destroyed prefix cache and asked the model to manage ids.

## Decision

One list of private lines. The user blob after name and Who you are is untagged prose: prior private
lines, then a copied perception string. Sidecar is `{"line":"..."}` or `{"line":null}`. Perception
owns the character-side store (`perception_` overwritten each apply) and optional facts. Scene
fork/merge/conclude is unchanged.

The objective journal (`take` / `seen`) is retired. Narration stays on the scene. Old saves that still
hold `objective_journal`, `observation_consumed_lines`, or monologue `after` are ignored; they do not
seed `perception_`.

## Consequences

Old saves with `streams[]` flatten to monologue lines with `after = 0`. Cores are not revised on this call. Belief decay no longer runs after monologue apply.
