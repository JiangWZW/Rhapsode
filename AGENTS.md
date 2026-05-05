# Rhapsode wiki — agent contract (schema layer)

This file is the **schema** in the [LLM Wiki](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) sense: it tells humans and coding agents how this repository's wiki is structured, when to update it, and what conventions to follow.

## Purpose

- **Compounding documentation** for the Rhapsode project (clean-room C++ text-RPG engine + FastAPI + Vue).
- The wiki lives in `wiki/` and is **tracked in git**. Cursor plans are a source of truth until reflected in the wiki; after that, prefer updating the wiki and linking the plan for history.

## Three layers (per Karpathy)

| Layer | Path | Role |
|-------|------|------|
| **Raw sources** | [`raw/`](raw/) | Immutable or slow-changing inputs: links, pasted specs, exports. Do not "fix" narrative here—summarize in `wiki/`. |
| **Wiki** | `wiki/` | Curated pages: concepts, architecture, decisions, how-to. Cross-link with `[[wikilinks]]` where helpful. Open `wiki/` in Obsidian as the vault root. |
| **Schema** | **This file** | Rules for maintaining the wiki and optional tooling hooks. |

## Conventions

- **Filenames:** `kebab-case.md` for pages; folders by domain (`concepts/`, `architecture/`, `decisions/`).
- **Frontmatter (optional):** `title`, `date`, `tags` for future Dataview-style tooling.
- **Wikilinks:** Use `[[page-name]]` when Obsidian or compatible tooling is used; for portability in tracked files, prefer explicit `[text](path.md)`.
- **Decisions:** Significant reversals or trade-offs get an ADR in `wiki/decisions/` with title `YYYY-MM-DD-short-topic.md` or `topic.md` with a date line inside.

## Operations

### After a planning session

1. Append a line to `wiki/log.md` (`## [YYYY-MM-DD] plan | …`).
2. Update `wiki/index.md` if new pages were added.
3. Update affected concept/architecture pages; add cross-links.

### Ingest (optional pattern)

When dropping a new reference into `raw/`:

1. Add a one-line entry under **Sources** in [`raw/sources.md`](raw/sources.md).
2. Summarize in `wiki/` (new page or section), link back to `raw/`.

### Lint (periodic)

- Broken relative links.
- Orphan pages (nothing links to them)—either link from `index.md` or merge into a parent.
- Stale **MVP** or **deferred** claims vs `AGENTS.md` and current repo reality.

## Relationship to code

- Code will eventually live under `core/`, `server/`, `frontend/` per plan. This wiki documents **intent and design**; implementation status should be noted on relevant pages (e.g. "not implemented yet").
