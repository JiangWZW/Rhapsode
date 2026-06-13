# Rhapsode

Clean-room **AI text RPG engine** — C++ core, Python LLM layer, FastAPI server, Vue 3 frontend.

## Documentation

Design documentation lives in `wiki/` as an **Obsidian vault** (tracked in git).

### Opening the wiki

1. Install Obsidian: `winget install --id Obsidian.Obsidian -e`
2. In Obsidian: **Open folder as vault** → select the `wiki/` subfolder inside this repo.
3. Start from `wiki/index.md` — the master catalog of all pages.

### Quick links (for non-Obsidian users)

- **[Agent / schema contract](AGENTS.md)** — how to maintain the wiki (LLM Wiki pattern)
- **[Raw sources](raw/sources.md)** — reference links and external inputs
- **Inspiration:** [Karpathy — LLM Wiki](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f)

## Repo layout

```
Rhapsode/
├── AGENTS.md          # Wiki + agent schema (tracked)
├── raw/               # Reference sources (tracked)
├── wiki/              # Obsidian vault — curated docs (tracked)
├── core/              # C++17 engine (SceneLoop, Director, WorldGraph, MemorySystem)
├── bindings/          # pybind11 module (_core)
├── server/            # FastAPI WebSocket server + LLM + memory
├── frontend/          # Vue 3 panel-based UI
└── third_party/       # Boost headers, llama.cpp binaries
```

## Note on `wiki/`

The `wiki/` folder is version-controlled so architecture and philosophy stay shared across clones. Obsidian settings under `wiki/.obsidian/` may include machine-specific preferences; commit those you want everyone to share, or ignore subpaths locally if needed.

### Obsidian vs Git

Obsidian stores vault metadata in `wiki/.obsidian/`. If your team prefers not to commit workspace UI state, add selective ignores (e.g. `wiki/.obsidian/workspace`) in `.gitignore` later.
