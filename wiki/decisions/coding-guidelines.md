# Decision: coding guidelines (Karpathy principles)

**Status:** accepted
**Date:** 2026-05-05

## Context

Talemate's codebase grew into a monolith with god objects, bloated abstractions, and tightly coupled systems. Rhapsode must avoid repeating this.

## Decision

Adopt the four principles from [Karpathy-inspired coding guidelines](https://github.com/forrestchang/andrej-karpathy-skills) as the project's coding standard:

1. **Think Before Coding** -- state assumptions, surface tradeoffs, ask when unclear.
2. **Simplicity First** -- minimum code that solves the problem. No speculative features, no abstractions for single-use code. If 200 lines could be 50, rewrite it.
3. **Surgical Changes** -- touch only what you must. Match existing style.
4. **Goal-Driven Execution** -- define success criteria, loop until verified.

Full text: [`raw/karpathy-coding-guidelines.md`](../raw/karpathy-coding-guidelines.md)

## Consequences

- Every file should be short, obvious, and justified.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- Push back on scope creep during planning, not after implementation.
- Code review question: "Would a senior engineer say this is overcomplicated?"
