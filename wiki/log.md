# Rhapsode wiki — log

Append-only timeline of wiki and project evolution. Newest entries at the **top**.

## [2026-05-06] arch | Session layer — multi-scene asynchronous architecture (Option A)

- **Decision**: `PlotGraph`, `GitStore`, and `Director` are now owned by a new top-level **`Session`** class, not by `Scene`.
- `Scene` becomes a **local view** — it holds only `History` and `Characters` for one scene context.
- Multiple `SceneLoop` instances share one `Session`; each loop has a **`resolution`** parameter (default 1). Director ticks once every `resolution` turns of that loop.
  - `resolution = 1` → interactive player scene (every turn)
  - `resolution = N` → background world scene (every N turns, async)
- `Session::tick(DirectorInput)` guards graph writes with a mutex so concurrent loops are safe.
- `DirectorInput` now carries `scene_id` so `GitStore` commit messages record which scene triggered each transition.
- Python manages the async scheduling of multiple `SceneLoop`s (asyncio); C++ core has no async knowledge.
- Updated `plan/plotgraph_director_gitstore_37f77032.plan.md`: added `session.h/cpp`, updated `SceneLoop` API, new `tavern_session.json` schema, updated file list.
- Updated `wiki/architecture/system-overview.md`: new architecture diagram showing Session/SceneLoop layers, updated control flow sequence, added constraints 6 & 7, added Session and multi-scene rows to implementation roadmap.
- Updated `wiki/architecture/plot-graph.md`: opening paragraph clarifies Session ownership of PlotGraph.

## [2026-05-06] wiki | Literature review — 8 papers from awesome-llm-story-generation

- Created `research/literature-review.md`: formal literature review of 8 papers selected from [awesome-llm-story-generation](https://github.com/Picrew/awesome-llm-story-generation) for relevance to Rhapsode's architecture.
- Papers reviewed: IBSEN (ACL 2024), StoryVerse (FDG 2024), CFPG (ArXiv 2026), RecurrentGPT (2023), Generative Agents (UIST 2023), FACTTRACK (NAACL 2025), Creating Suspenseful Stories (EACL 2024), EvoSpark (ACL 2026).
- 19 concrete ideas adopted (mapped to Rhapsode subsystems in summary table).
- 4 confirmed novelty gaps identified: no paper implements multi-dimensional plot node DAG, constrained choices at graph nodes + freeform on edges, read/write action distinction, or multi-dimensional arrival.
- 5 key warnings extracted: pure simulation is boring (Generative Agents), LLM objective checking unreliable (IBSEN F1≈0.77), structural suspense cues noisy (55% perception), foreshadowing subtlety is prompt engineering (CFPG), long-form coherence unsolved at scale.
- Updated `plot-graph.md`: added knowledge-state metadata, anti-stall policy, F-T-P triple formalization to plot nodes.
- Updated `system-overview.md`: added adopted literature ideas to memory and Director sections.
- Added all 8 papers + awesome-llm-story-generation + Intra blog post to `raw/sources.md`.
- Added **Intra** (Ian Bicking, 2025) to literature review: practitioner design log validating ground-truth-first architecture, input rewriting as intent parser, guided thinking for structured LLM reasoning, event filtering per NPC. Maps Bicking's "further directions" to existing Rhapsode designs.

## [2026-05-06] wiki | Read/write actions, input mode spectrum, multi-dimensional graph

- Added **principle 6** to `narrative-philosophy.md`: "The interface is part of the dramaturgy — read actions vs. write actions."
  - **Read actions** (observe, talk, explore) are freeform — no graph mutation.
  - **Write actions** (decide, commit) are constrained choices at plot nodes — each maps to a graph edge transition.
  - Input mode spectrum: freeform → guided freeform → constrained choice → forced progression.
  - Talemate comparison: Talemate's "Dynamic Actions" are random-chance suggestion chips that can be ignored; Rhapsode's constrained choices are mandatory at graph nodes.
- Added **player traversal model** to `plot-graph.md`: players are always "on an edge" (freeform) until they arrive "at a node" (constrained choices). Adaptive choice count per node.
- Added **multi-dimensional problem** to `plot-graph.md`: concurrent plot node threads mean the player is simultaneously on edges in multiple dimensions. Open strategies: Director serialization, composite choice points, independent queuing.
- Updated `system-overview.md`:
  - Director now has four operations per turn (added "determine input mode").
  - Control flow sequence diagram updated to show input mode branching (constrained choice vs. freeform) before player input.
- Two new open questions added to `plot-graph.md`: multi-dimensional arrival strategy; freeform edge-breaking.
- Added Talemate story progression analysis reference to `raw/sources.md`.

## [2026-05-06] wiki | Track `wiki/` in git

- Removed `wiki/` from `.gitignore`; Obsidian vault is now version-controlled with the repo.
- README / AGENTS already describe tracked wiki; `architecture/stack.md` updated.

## [2026-05-06] wiki | Director's five rules for interesting worlds

- Added five mechanical rules to `system-overview.md` Director section:
  1. Minimum plot node floor -- generate immediately when active count drops below threshold.
  2. Timescale balance -- maintain plot nodes across immediate, short-term, and long-term horizons.
  3. NPC autonomy -- NPCs act off-screen, player encounters them mid-action.
  4. Disproportionate consequences -- generation pipeline biases toward cascading small actions into unexpected outcomes.
  5. Reputation propagation -- player actions spread through NPC awareness via memory.

## [2026-05-06] wiki | Plot graph auto-merge + revert, memory importance scoring

- Added **auto-merging** to `plot-graph.md`: when two active plot nodes share characters or locations, the Director spawns a merge node with combined context.
- Added **revert** to `plot-graph.md`: transition log enables undo to a previous node state, rolling back history and memory. VCS analogy extended with revert and log rows.
- Added **importance scoring** to `system-overview.md` memory section: float 0.0-1.0, write-once, three sources (plot graph events, structural signals, LLM assessment during generation pipeline). Affects retrieval priority and decay resistance.

## [2026-05-06] wiki | System overview -- engineering synthesis

- Created `architecture/system-overview.md`: translates narrative philosophy into four subsystems (World State, Memory, Plot Graph, Director), data structures, control flow diagrams, engineering constraints, and implementation roadmap.
- Added GRRM references (outlines interview, architects vs gardeners) to `raw/sources.md`.
- Defined concrete node/edge structures for the plot graph (PlotNode, Edge, Trigger types).
- Documented per-turn control flow as a sequence diagram: SceneLoop -> Director traverse -> memory query -> prompt build -> LLM -> post-turn generation check.
- Five engineering constraints codified: no sync LLM in player turns, serializable graph, predicate-based triggers, lossy pipeline, independently testable subsystems.

## [2026-05-06] wiki | Director as Rhapsode, generation pipeline, fortune tracker rejected

- Reframed the Director as the **rhapsode** -- an arranger of LLM-generated fragments, not a controller. The project name is the architectural thesis.
- Added the **generation pipeline** to `plot-graph.md`: LLM composes free text, Director extracts into graph nodes. Three moments: scenario init, periodic world-building, reactive spawning after major events.
- Chose **free text + extraction** (option B) over structured JSON output -- keeps creative output unconstrained.
- **Rejected the fortune tracker.** The arc is not a number. It emerges from accumulated memory + active plot nodes. No explicit tracking.
- Resolved open questions: dynamic plot nodes (yes, via generation pipeline), authoring (authors design worlds, LLM discovers dramatic potential), fortune tracker (dropped).
- Updated `narrative-philosophy.md`, `plot-graph.md`, `rhapsode-overview.md` to reflect all changes.

## [2026-05-06] wiki | Plot graph architecture

- Created `architecture/plot-graph.md`: the core post-MVP narrative data structure.
- Plot nodes are latent world facts (secrets, debts, ticking clocks) organized as a DAG.
- Director traverses the graph deterministically -- no LLM needed for structure.
- Two loops: player-facing (synchronous with SceneLoop) and world-background (off-screen events advance between turns).
- Prophet concept resolved: the Prophet is the graph itself (dormant nodes = sealed predictions, foreshadowed nodes = subtle hints).
- Version control analogy: branches = possible futures, commits = activated plot nodes, merges = colliding plot node lines, HEAD = current world state.
- Scenario authoring is plot graph design: characters with secrets, forces in motion, trigger conditions.
- Updated `narrative-philosophy.md` to link to the plot graph as the concrete realization of principles 1-5.

## [2026-05-06] wiki | Narrative philosophy — foundational design beliefs

- Created `concepts/narrative-philosophy.md`: the five principles that constrain all design.
- Director as compass (rules engine, not LLM agent): fortune tracker, beat counter, constraint set.
- Long-term memory as emotional backbone: weighted memories, not equal-weight chunks.
- The Hamlet quality: ambiguity is depth, not a bug to resolve.
- LLM as world simulator, not storyteller: constraints + state + rendering, not plot decisions.
- Elastic arc: steer consequences, not player actions.
- Lessons extracted from Talemate's Director (god-object, LLM-driven structure) and Memory (ChromaDB, reinforcements, layered context).
- Source: Vonnegut's Shapes of Stories, Talemate codebase analysis.

## [2026-05-05] wiki | Adopted Karpathy coding guidelines

- Added ADR `coding-guidelines.md`: simplicity first, surgical changes, goal-driven execution.
- Saved full guidelines text to `raw/karpathy-coding-guidelines.md`.
- Source: [forrestchang/andrej-karpathy-skills](https://github.com/forrestchang/andrej-karpathy-skills)

## [2026-05-05] wiki | Implementation-ready detail pass

- Renamed all references from DigitalDream to Rhapsode.
- Fleshed out `stack.md` with repo layout, build system, dependency tables.
- Fleshed out `mvp-v0.md` with per-layer task breakdown, end-to-end sequence diagram, scenario JSON schema, definition of done checklist.
- Fleshed out `scene-loop.md` with FSM state diagram, C++ class sketch, pybind11 callback registration pattern.
- Created `cpp-data-model.md`: SceneMessage, History, Character, Scene structs with full JSON serialization contract.
- Created `python-server.md`: FastAPI structure, WebSocket protocol, LLM client abstraction (Gemini + OpenAI), prompt builder, session management.
- Created `vue-frontend.md`: component tree, TypeScript types, Pinia WebSocket store, Vite config.
- Created ADR `callback-vs-pull.md`: chose callback registration over polling for SceneLoop-to-Python invocation.

## [2026-05-05] wiki | Renamed to Rhapsode, moved to Rhapsode repo

- Renamed all references from DigitalDream to Rhapsode.
- Moved wiki into `Rhapsode/wiki/` as git-ignored Obsidian vault.
- Fixed stale links (removed `Start here.md`, renamed overview file).

## [2026-05-05] wiki | Initial LLM Wiki scaffold

- Created `AGENTS.md`, `raw/sources.md`, `wiki/index.md`, starter concept/architecture/decision pages.
- Adopted [Karpathy LLM Wiki](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) pattern: raw / wiki / schema layers.
