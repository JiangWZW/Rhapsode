# Rhapsode

Clean-room **AI text RPG engine** — C++ core, Python LLM layer, FastAPI server, Vue 3 frontend.

## Documentation

Design documentation lives in `wiki/` as an **Obsidian vault** (git-ignored, local-only).

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
├── wiki/              # Obsidian vault — curated docs (git-ignored, local-only)
├── core/              # C++ (future)
├── server/            # FastAPI (future)
└── frontend/          # Vue 3 (future)
```

## Note on `wiki/`

The `wiki/` folder is in `.gitignore` — it is a local Obsidian vault and not tracked by git. Each contributor maintains their own copy. Shared decisions and references that must be tracked go in `AGENTS.md` and `raw/`.
