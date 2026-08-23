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

The take stays on the **user** track (DeepSeek concatenates system-role messages at the front of the template). Order:

1. **System** — craft + JSON schema. Identical for every character, every turn. Ids under **On your mind** are the ones to cite; **What you've been thinking** is the private journal, oldest first.
2. **User** — `You are {name}.` then **Who you are** (CharacterCore), **On your mind** (active `id: focus`, insertion order), **What you've been thinking** (every stored line from every stream, including closed, tagged `[{id}]`, global `seq` order, append-only), **What just happened** (this turn's capped `take`, then this turn's `seen`). The only replaced tail.

Voice, compact beliefs, and last-N windows stay out of this prompt. `description` is bootstrap-only when core is empty. Other interiors, omniscient graph, and `query_mind` of anyone else stay out. Weave still uses a short `format_graph_seed`; monologue does not.

A stream `focus` is a topic, worry, relationship, or want — not wants-only. Roster bytes change on fork/merge/conclude (intentional miss). The inner journal never drops or rewrites a line. Full public history stays off this prompt. `core_revision` rebuilds the bible head. Expect cheaper input on `stage=monologue` (`prompt_cache_hit_tokens`), not skipped thinking time.

## Belief graph writes (no reflect LLM)

- Interpreted beliefs: only `knows[]` from the monologue call (C++ applies).
- Objective journal: engine copies this turn's narrator + speech as `take`; a per-character observation call may append `seen` (`stage=observation`, flash, thinking on). Monologue tail is this turn's `take` (capped) plus `seen` (`stage=monologue`, pro, thinking on).
- Seeds / intentions / decay: CPU only.

## Stream lifecycle

Fork / merge / conclude (min 1 active, max 5). Parent stays on fork. Pattern inspired by storylines; **not** `SceneData`.

## Narrator

Beat narrator owns prose/`speech_turns`/`active_cast`. Graph owns world nodes and does not route them into minds. `query_mind` returns core + stream tails + compact beliefs. Narrator does not author streams.

## Stale docs

Prefer this page over countdown-reflection / `self_state_` claims in [character-system.md](character-system.md) and [memory-system.md](memory-system.md). See also [subjective-character-minds.md](subjective-character-minds.md) (partially superseded).
