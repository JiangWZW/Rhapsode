# Rhapsode wiki — log

Append-only timeline of wiki and project evolution. Newest entries at the **top**.

## [2026-05-12] wiki | Full wiki rewrite — align docs with implementation

- **Problem**: The entire wiki was deleted from disk (all pages showed as `D` in git). Content had diverged significantly from the actual codebase — the old wiki described planned structures that were never built (Session layer, PlotGraph DAG, llm/ subpackage, ws.py, session.py) while missing implemented features (MemorySystem, Director, save/load, local LLM integration).
- **Rewrote all pages** to accurately reflect the working system:
  - `concepts/rhapsode-overview.md` — updated architecture summary, current state, what is and is not built
  - `concepts/narrative-philosophy.md` — added implementation status notes to each principle
  - `architecture/system-overview.md` — rewritten from aspirational to actual; clear diagram of current architecture; implementation status table separating built from planned
  - `architecture/stack.md` — actual repo layout, actual dependencies, actual build commands
  - `architecture/cpp-data-model.md` — all 10 C++ types documented (was 4)
  - `architecture/scene-loop.md` — Director integration, history windowing, resume support
  - `architecture/plot-graph.md` — split into "implemented" (Node/NodePool) and "planned" (DAG with edges)
  - `architecture/python-server.md` — actual flat structure, not the planned llm/ subpackage
  - `architecture/vue-frontend.md` — actual component tree and store implementation
  - `architecture/mvp-v0.md` — retrospective: what was built, what deviated from plan
  - `architecture/memory-system.md` — **new page** covering the full C++/Python memory pipeline
  - `decisions/ownership-split.md` — updated with MemorySystem callback boundary
  - `decisions/callback-vs-pull.md` — added retrospective
  - `decisions/coding-guidelines.md` — added retrospective
- Restored `research/literature-review.md` and `raw/sources.md` from git.
- Updated `index.md` to reflect the new page set.

## [2026-05-06] arch | Session layer — multi-scene asynchronous architecture (Option A)

- **Decision**: `PlotGraph`, `GitStore`, and `Director` are now owned by a new top-level **`Session`** class, not by `Scene`.
- `Scene` becomes a **local view** — it holds only `History` and `Characters` for one scene context.
- Multiple `SceneLoop` instances share one `Session`; each loop has a **`resolution`** parameter (default 1). Director ticks once every `resolution` turns of that loop.
- Python manages the async scheduling of multiple `SceneLoop`s (asyncio); C++ core has no async knowledge.
- **Note (2026-05-12)**: Session layer was designed but not implemented. Current architecture has Scene owning NodePool directly, with a single SceneLoop per connection.

## [2026-05-06] wiki | Literature review — 8 papers from awesome-llm-story-generation

- Created `research/literature-review.md`: formal literature review of 8 papers.
- Papers: IBSEN (ACL 2024), StoryVerse (FDG 2024), CFPG (ArXiv 2026), RecurrentGPT (2023), Generative Agents (UIST 2023), FACTTRACK (NAACL 2025), Suspenseful Stories (EACL 2024), EvoSpark (ACL 2026).
- 19 concrete ideas adopted. 4 confirmed novelty gaps. 5 key warnings extracted.
- Added Intra (Ian Bicking, 2025) practitioner design log.

## [2026-05-06] wiki | Read/write actions, input mode spectrum, multi-dimensional graph

- Added principle 6 to `narrative-philosophy.md`: read actions vs write actions.
- Added player traversal model and multi-dimensional problem to `plot-graph.md`.
- **Note (2026-05-12)**: Not implemented. Current UI is freeform-only.

## [2026-05-06] wiki | Track `wiki/` in git

- Removed `wiki/` from `.gitignore`; Obsidian vault is now version-controlled with the repo.

## [2026-05-06] wiki | Director's five rules for interesting worlds

- Added five mechanical rules to `system-overview.md` Director section.
- **Note (2026-05-12)**: Design intent only. Rules require the full DAG architecture to implement.

## [2026-05-06] wiki | Plot graph auto-merge + revert, memory importance scoring

- Added auto-merging, revert, and importance scoring designs.
- **Note (2026-05-12)**: Not implemented. Requires DAG edges.

## [2026-05-06] wiki | System overview — engineering synthesis

- Created `architecture/system-overview.md`: four subsystems, control flow, engineering constraints.

## [2026-05-06] wiki | Director as Rhapsode, generation pipeline, fortune tracker rejected

- Reframed the Director as the rhapsode. Added generation pipeline. Rejected fortune tracker.

## [2026-05-06] wiki | Plot graph architecture

- Created `architecture/plot-graph.md`. The Prophet is the graph. VCS analogy. Two loops.

## [2026-05-06] wiki | Narrative philosophy — foundational design beliefs

- Created `concepts/narrative-philosophy.md`: five principles.

## [2026-05-05] wiki | Adopted Karpathy coding guidelines

- Added ADR `coding-guidelines.md`.

## [2026-05-05] wiki | Implementation-ready detail pass

- Fleshed out `stack.md`, `mvp-v0.md`, `scene-loop.md`, `cpp-data-model.md`, `python-server.md`, `vue-frontend.md`.
- Created ADR `callback-vs-pull.md`.

## [2026-05-05] wiki | Renamed to Rhapsode, moved to Rhapsode repo

- Renamed all references from DigitalDream to Rhapsode.

## [2026-05-05] wiki | Initial LLM Wiki scaffold

- Created `AGENTS.md`, `raw/sources.md`, `wiki/index.md`, starter pages.
- Adopted Karpathy LLM Wiki pattern.
