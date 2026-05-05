# LLM Wiki pattern (Karpathy)

Source: [LLM Wiki gist — Andrej Karpathy](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f)

## Core idea

Instead of **only** retrieving chunks at query time (classic RAG), an LLM **maintains a persistent wiki**: structured Markdown that **compounds** — entity pages, synthesis, cross-links, contradiction notes.

Three layers:

1. **Raw sources** — curated inputs (immutable).
2. **Wiki** — Markdown owned by the LLM (with human steering).
3. **Schema** — contract for structure and workflows (`AGENTS.md` / `CLAUDE.md`-style).

## How Rhapsode uses it

This repo applies the **same filesystem layout** to **design documentation** for the engine:

| Karpathy layer | Here |
|----------------|------|
| Raw | [`raw/`](../raw/) — links, external refs |
| Wiki | [`wiki/`](../) — concepts, architecture, ADRs |
| Schema | [`AGENTS.md`](../AGENTS.md) — maintenance rules |

We are **not** (yet) automating ingest from a running game's session logs into this wiki; that could be a later **ops** addition. For now the wiki is the **durable home** for the plan and decisions that would otherwise live only in chat or Cursor.

## Index and log

- [`index.md`](../index.md) — content catalog.
- [`log.md`](../log.md) — chronological append-only log.

This matches the gist's guidance: the index helps humans and agents **route** before reading every page.
