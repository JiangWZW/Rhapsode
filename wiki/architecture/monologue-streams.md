---
title: Monologue + character core
date: 2026-08-24
tags: [architecture, character, memory]
---

# Monologue + character core

Design of record for per-character minds.

## Layers

- **Narration** — lives on the scene. `format_narration_window` slices the last three turns of narrator prose and speech. World does not store that string.
- **Perception** — one overwritten first-person string per character. The Perception LLM reads the capped narration window and writes `{"perception":"..."}` plus optional `facts`. Null/empty leaves the previous text and still records the scene turn.
- **Monologue lines** — private first-person lines. The Monologue LLM never reads narration. It reads Who you are, prior private lines, then a copy of `perception_`.
- **Belief graph** — factual long-term knowledge. Perception may write `facts`. The monologue call does not.

Bootstrap fills empty core from scenario `core`, then `description`.

## Writer (actor frame)

After a public beat, one actor per on-stage NPC. Hold the bible. Write one private line, or stay silent. Output is JSON only: `{"line":"..."}` or `{"line":null}`.

## Prompt envelope (prefix cache)

Play session: `process_post_turn` computes the narration window once. If it is empty, perception and monologue skip that scene this turn. Otherwise Perception first, then monologue. HTTP runs off the turn thread; apply happens on the next poll. Eval runners that only set `LLMCallback` stay on the blocking `update_*` path (`update_perceptions` then `update_monologues`).

Python splits on `<<<RHAPSODE_PERCEPTION_USER>>>` / `<<<RHAPSODE_MONOLOGUE_USER>>>` into `role=system` + `role=user`.

Perception:

1. **System** — name, Who you are, first-person craft, `{"perception","facts"}`.
2. **User** — only the capped narration window.

Monologue:

1. **System** — craft + `{"line"}` schema. Identical for every character, every turn.
2. **User** — `You are {name}.` then **Who you are** (core), then private lines oldest first, then the copied perception string if any. No narration window. No `[turn take]`.

`query_mind` returns core + truncated perception + last three monologue lines + compact beliefs. Narrator does not author private lines.

Poll claims by scene turn. Perception dispatches when `perception_turn_ < turn`. Monologue dispatches when `perception_turn_ >= turn` and `monologue_turn_ < turn`. Same-poll harvests a prior in-flight perception before claiming monologue; this turn's new perception does not also claim monologue. HTTP failure releases the slot and does not advance the turn.

## Narrator

Beat narrator owns prose/`speech_turns`/`active_cast`. Graph owns world nodes and does not route them into minds.

As of 2026-08-29 the beat turn state also includes each on-stage NPC's latest
monologue line as `On their mind` (omitted when empty). That string is private
context for what the character does; it is not spoken.

## Stale docs

Prefer this page over countdown-reflection / `self_state_` claims in [character-system.md](character-system.md) and [memory-system.md](memory-system.md). See also [subjective-character-minds.md](subjective-character-minds.md) (partially superseded).
