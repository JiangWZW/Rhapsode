# Raw sources catalog

Immutable or reference inputs for Rhapsode. Do not edit sources to "fix" the narrative—update derived pages under `wiki/` instead.

| Source | Description |
|--------|-------------|
| [Karpathy — LLM Wiki (gist)](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) | Pattern for a **compounding** markdown wiki (raw vs wiki vs schema); indexing with `index.md` / `log.md`. |
| Cursor plan `digitaldream_engine_plan_423af501.plan.md` | Authoritative planning snapshot in the Cursor plans folder (path varies by machine). |
| [Karpathy coding guidelines](https://github.com/forrestchang/andrej-karpathy-skills) | Four principles for LLM-assisted coding: think first, simplicity, surgical changes, goal-driven. Saved as [`karpathy-coding-guidelines.md`](karpathy-coding-guidelines.md). |
| Talemate (optional) | Prior art only—concepts, not code reuse. See `../talemate` if present in workspace. |
| [Vonnegut — Shapes of Stories](https://storytellingedge.substack.com/p/the-simple-shapes-of-great-stories) | Kurt Vonnegut's framework for universal story shapes, confirmed by NLP research across 2,000 novels. Foundation for the elastic arc and fortune tracker concepts. |
| [GRRM on outlines](https://www.youtube.com/watch?v=XF1PyB5v9jI) | George R.R. Martin on why he doesn't write outlines. "Outlining is like retelling a story you've already told in shorthand." Validates LLM-generated plot graph over hand-authored scripts. |
| [GRRM architects vs gardeners](https://www.youtube.com/watch?v=nK6VoL76r3Q) | Martin's metaphor: architects plan everything, gardeners plant seeds and tend what grows. Rhapsode is a computational gardener. |
| Talemate Director analysis | Internal exploration of Talemate's `DirectorAgent` (mixins, FOCAL actions, `DirectorMessage`). Lessons: separation of structure from prose is good; god-object accumulation is bad. |
| Talemate Memory analysis | Internal exploration of Talemate's `MemoryAgent` + ChromaDB + reinforcements. Lessons: layered context (raw + summary + RAG) is right; equal-weight chunks and over-engineering are wrong. |
