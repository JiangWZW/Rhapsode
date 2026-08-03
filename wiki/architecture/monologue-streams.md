---
title: Monologue streams + character core
date: 2026-08-02
tags: [architecture, character, memory]
---

# Monologue streams + character core

Design of record for per-character minds after dropping forced perception→Thought reflection.

## Layers

| Layer | Role | LLM? |
|-------|------|------|
| **CharacterCore** | Deep continuity sheet / soul-level identity analysis. Authored in scenario `core` (Cast keeps short `description`). **Not** a thought stream. | Rare `core_revision` |
| **Monologue streams** (1–5 active) | Rendered interiority / subtext / objectives | On-stage updater |
| **Belief graph** (`beliefs_`) | Factual long-term subjective knowledge | Via optional `knows[]` on same call |

Bootstrap prefers scenario `core`, then falls back to `description`.

## Writer agent (actor frame)

The post-turn mind LLM is an **actor for one character** after a public beat: hold the bible, optionally write subtext, optionally commit facts to `knows[]`. Empty appends/`ops`/`knows` are success (listening take).

## Belief graph writes (no reflect LLM)

- Interpreted beliefs: only `knows[]` from the monologue call (C++ applies).
- Narrow-routed perceptions: prompt stimulus; consumed after the call; **not** auto-promoted.
- Seeds / intentions / decay: CPU only.

## Stream lifecycle

Fork / merge / conclude (min 1 active, max 5). Parent stays on fork. Pattern inspired by storylines; **not** `SceneData`.

## Narrator

Beat narrator owns prose/`speech_turns`/`active_cast`. Graph owns world nodes. `query_mind` returns core + stream tails + compact beliefs. Narrator does not author streams.

## Stale docs

Prefer this page over countdown-reflection / `self_state_` claims in [character-system.md](character-system.md) and [memory-system.md](memory-system.md). See also [subjective-character-minds.md](subjective-character-minds.md) (partially superseded).
