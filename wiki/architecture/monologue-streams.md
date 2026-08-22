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

The post-turn mind LLM is an **actor for one character** after a public beat: hold the bible, optionally write subtext, optionally commit facts to `knows[]`. Empty appends/`ops`/`knows` are success (listening take). Output is **JSON only** (`appends`, `ops`, `knows`, `core_revision`). There is no freeform acting preamble; inhabitation lives in the prompt and in `appends.text`.

## Prompt envelope (prefix cache)

One native `LLMCallback` string per living on-stage NPC. Python `make_reflection_callback` splits on `<<<RHAPSODE_MONOLOGUE_USER>>>` into `role=system` + `role=user`. No per-character chat store, no tools, no replay of last JSON. Missing sentinel (old `_core.pyd`) keeps the old single user blob.

Order is a prefix tree (stable head, volatile tail). Cloud APIs cache a **byte-identical leading prefix**, not a registered graph.

1. **System** — craft + JSON schema. Identical for every character, every turn.
2. **User head** — `You are {name}`, lived voice (`Character::build_prompt__dialogue_voice()`), CharacterCore as *who I am*.
3. **User middle** — through-line roster (`focus [id]`), then last inner beats, then compact held truths.
4. **User tail** — this take’s given circumstances, then raw perceptions (fact text first, `#id` at the end of the line).

`description` is bootstrap-only when core is empty; it is not duplicated as a “Seed description” field. Stream listing stays insertion order. Other interiors, omniscient graph, and `query_mind` of anyone else stay out of this prompt.

`core_revision` forks the tree at “I am.” Cache hits are **not** guaranteed on the next take (DeepSeek common-prefix persistence often needs two forks of the same head). Expect cheaper input on `stage=monologue` (`prompt_cache_hit_tokens`), not skipped thinking time.

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
