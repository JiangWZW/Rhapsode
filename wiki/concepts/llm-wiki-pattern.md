---
sources:
  - AGENTS.md
  - wiki/SCHEMA.md
last_updated: 2026-05-12
confidence: verified
tier: procedural
related:
  - "[SCHEMA](../SCHEMA.md)"
tags:
  - design
---

# LLM Wiki pattern (Karpathy)

Source: [LLM Wiki gist — Andrej Karpathy][1]

[1]: https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f

## Core idea

Classic RAG retrieves chunks at query time. An LLM wiki goes further: structured Markdown that compounds — entity pages, synthesis, cross-links, contradiction notes.

Three layers:

1. **Raw sources** — curated inputs (immutable).
2. **Wiki** — Markdown owned by the LLM with human steering.
3. **Schema** — contract for structure and workflows (`AGENTS.md`-style).

## How Rhapsode uses it

This repo applies the same filesystem layout to **design documentation** for the engine:

| Karpathy layer | Here |
|----------------|------|
| Raw | [`raw/`](../../raw/) — links, external refs, guidelines |
| Wiki | [`wiki/`](../) — concepts, architecture, decisions, research |
| Schema | [`AGENTS.md`](../../AGENTS.md) — maintenance rules |

The wiki is the durable home for plans and decisions that would otherwise live only in chat transcripts or Cursor sessions.

## Maintenance contract

Per `AGENTS.md`, after any planning session:

1. Append a line to [`log.md`](../log.md).
2. Update [`index.md`](../index.md) if new pages were added.
3. Update affected concept/architecture pages; add cross-links.

## Index and log

- [`index.md`](../index.md) — content catalog; helps humans and agents route before reading every page.
- [`log.md`](../log.md) — chronological append-only log of wiki and project evolution.
