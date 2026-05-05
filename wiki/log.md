# Rhapsode wiki — log

Append-only timeline of wiki and project evolution. Newest entries at the **top**.

## [2026-05-06] wiki | Track `wiki/` in git

- Removed `wiki/` from `.gitignore`; Obsidian vault is now version-controlled with the repo.
- README / AGENTS already describe tracked wiki; `architecture/stack.md` updated.

## [2026-05-06] wiki | Director's five rules for interesting worlds

- Added five mechanical rules to `system-overview.md` Director section:
  1. Minimum tension floor -- generate immediately when active count drops below threshold.
  2. Timescale balance -- maintain tensions across immediate, short-term, and long-term horizons.
  3. NPC autonomy -- NPCs act off-screen, player encounters them mid-action.
  4. Disproportionate consequences -- generation pipeline biases toward cascading small actions into unexpected outcomes.
  5. Reputation propagation -- player actions spread through NPC awareness via memory.

## [2026-05-06] wiki | Plot graph auto-merge + revert, memory importance scoring

- Added **auto-merging** to `plot-graph.md`: when two active tensions share characters or locations, the Director spawns a merge node with combined context.
- Added **revert** to `plot-graph.md`: transition log enables undo to a previous node state, rolling back history and memory. VCS analogy extended with revert and log rows.
- Added **importance scoring** to `system-overview.md` memory section: float 0.0-1.0, write-once, three sources (plot graph events, structural signals, LLM assessment during generation pipeline). Affects retrieval priority and decay resistance.

## [2026-05-06] wiki | System overview -- engineering synthesis

- Created `architecture/system-overview.md`: translates narrative philosophy into four subsystems (World State, Memory, Plot Graph, Director), data structures, control flow diagrams, engineering constraints, and implementation roadmap.
- Added GRRM references (outlines interview, architects vs gardeners) to `raw/sources.md`.
- Defined concrete node/edge structures for the plot graph (Tension, Edge, Trigger types).
- Documented per-turn control flow as a sequence diagram: SceneLoop -> Director traverse -> memory query -> prompt build -> LLM -> post-turn generation check.
- Five engineering constraints codified: no sync LLM in player turns, serializable graph, predicate-based triggers, lossy pipeline, independently testable subsystems.

## [2026-05-06] wiki | Director as Rhapsode, generation pipeline, fortune tracker rejected

- Reframed the Director as the **rhapsode** -- an arranger of LLM-generated fragments, not a controller. The project name is the architectural thesis.
- Added the **generation pipeline** to `plot-graph.md`: LLM composes free text, Director extracts into graph nodes. Three moments: scenario init, periodic world-building, reactive spawning after major events.
- Chose **free text + extraction** (option B) over structured JSON output -- keeps creative output unconstrained.
- **Rejected the fortune tracker.** The arc is not a number. It emerges from accumulated memory + active tensions. No explicit tracking.
- Resolved open questions: dynamic tensions (yes, via generation pipeline), authoring (authors design worlds, LLM discovers dramatic potential), fortune tracker (dropped).
- Updated `narrative-philosophy.md`, `plot-graph.md`, `rhapsode-overview.md` to reflect all changes.

## [2026-05-06] wiki | Plot graph architecture

- Created `architecture/plot-graph.md`: the core post-MVP narrative data structure.
- Tensions are latent world facts (secrets, debts, ticking clocks) organized as a DAG.
- Director traverses the graph deterministically -- no LLM needed for structure.
- Two loops: player-facing (synchronous with SceneLoop) and world-background (off-screen events advance between turns).
- Prophet concept resolved: the Prophet is the graph itself (dormant nodes = sealed predictions, foreshadowed nodes = subtle hints).
- Version control analogy: branches = possible futures, commits = activated tensions, merges = colliding tension lines, HEAD = current world state.
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
