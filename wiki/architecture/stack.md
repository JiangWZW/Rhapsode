---
sources:
  - CMakeLists.txt
  - core/CMakeLists.txt
  - server/pyproject.toml
  - frontend/package.json
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/cpp-data-model]]"
  - "[[architecture/python-server]]"
  - "[[architecture/vue-frontend]]"
tags:
  - cross-layer
---

# Stack

## Layer diagram

```
┌────────────────────────┐
│  Vue 3 SPA (frontend/) │  TypeScript, Pinia, Vite
└───────────┬────────────┘
            │ WebSocket /ws
┌───────────▼────────────┐
│  FastAPI (server/)     │  Python 3.12+, Gemini, Chroma
│   + pybind11 bridge    │
└───────────┬────────────┘
            │ _core.pyd
┌───────────▼────────────┐
│  librhapsode (core/)   │  C++17, nlohmann/json
└────────────────────────┘
     Optional:
┌────────────────────────┐
│  llama.cpp (local LLM) │  GGUF model on port 8012
└────────────────────────┘
```

## Layer responsibilities

| Layer | Technology | Responsibility |
|-------|------------|----------------|
| UI | Vue 3 + TypeScript + Pinia | Chat view, message rendering, WebSocket connectivity |
| API | FastAPI + WebSocket | Session lifecycle, message ingress/egress, LLM orchestration |
| Bridge | pybind11 | Expose all C++ types and callbacks to Python |
| Core | C++17 | Scene state, turn loop, Director logic, node pool, memory scoring/retrieval |
| LLM (cloud) | Gemini via google-genai | Narrative generation and Director node management |
| LLM (local) | llama.cpp (optional) | Memory quality pipeline (distill, score, entity extract, conflict check) |
| Storage | ChromaDB + sentence-transformers | Vector embeddings, fact persistence, semantic retrieval |

## Repository layout

```
Rhapsode/
├── AGENTS.md                  # Wiki maintenance contract
├── README.md
├── CMakeLists.txt             # Top-level CMake (core + bindings)
├── .gitmodules                # llama.cpp submodule
├── build_llama.bat            # Downloads prebuilt llama.cpp Windows CUDA binary
├── start_llm.bat              # Launches llama-server on port 8012
│
├── raw/                       # Reference sources
│   └── karpathy-coding-guidelines.md
│
├── wiki/                      # Obsidian vault (tracked in git)
│   ├── index.md
│   ├── log.md
│   ├── concepts/
│   ├── architecture/
│   ├── decisions/
│   └── research/
│
├── core/                      # C++17 static library: librhapsode
│   ├── CMakeLists.txt
│   ├── include/rhapsode/
│   │   ├── character.h
│   │   ├── director.h
│   │   ├── history.h
│   │   ├── md5.h
│   │   ├── memory_system.h
│   │   ├── node.h
│   │   ├── node_pool.h
│   │   ├── scene.h
│   │   ├── scene_loop.h
│   │   └── scene_message.h
│   ├── src/                   # Corresponding .cpp files
│   └── tests/
│       ├── test_scene.cpp
│       └── test_node_pool.cpp
│
├── bindings/                  # pybind11 module: rhapsode._core
│   ├── CMakeLists.txt
│   └── bind_rhapsode.cpp
│
├── server/                    # Python package: rhapsode
│   ├── pyproject.toml
│   ├── scenarios/
│   │   ├── tavern.json
│   │   └── konosuba.json
│   └── rhapsode/
│       ├── app.py             # FastAPI app + WebSocket endpoint
│       ├── gemini.py          # Gemini LLM client
│       ├── prompt.py          # Prompt builder
│       ├── memory.py          # Chroma + embedding callbacks
│       ├── validator.py       # Local llama.cpp client for memory pipeline
│       ├── lemmatization.py   # spaCy BM25 lemmatization
│       └── spacy_models.py    # Lazy spaCy model loading
│
├── offline/                   # Offline tools (not the game runtime)
│   └── character_study/       # Novel walk → living study; own venv; not session eval
│
├── frontend/                  # Vue 3 SPA
│   ├── package.json
│   ├── vite.config.ts
│   ├── tsconfig.json
│   └── src/
│       ├── App.vue
│       ├── main.ts
│       ├── style.css
│       ├── components/
│       │   ├── ChatView.vue
│       │   ├── MessageList.vue
│       │   └── InputBar.vue
│       └── stores/
│           └── websocket.ts
│
└── third_party/
    └── llama.cpp/             # Git submodule (optional, for local LLM)
```

## Build system

| Component | Tool | Command |
|-----------|------|---------|
| C++ core + bindings | CMake 3.20+ | `cmake -B build` then `cmake --build build` |
| Python package | pip / uv | `pip install -e ./server` |
| Vue frontend | Vite | `cd frontend && npm install && npm run dev` |
| Local LLM (optional) | llama.cpp | `build_llama.bat` then `start_llm.bat` |

The top-level CMakeLists.txt builds both `librhapsode` (static library) and the pybind11 module (`_core.pyd`). A post-build step copies `_core.pyd` into `server/rhapsode/` so Python can import it directly.

## Dependencies

### C++ (fetched via CMake FetchContent)

| Library | Version | Purpose |
|---------|---------|---------|
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | JSON serialization for all C++ types |
| [pybind11](https://github.com/pybind/pybind11) | v2.13.6 | C++ ↔ Python bindings |
| [Catch2](https://github.com/catchorg/Catch2) | v3.7.1 | C++ unit tests |

### Python (`server/pyproject.toml`)

| Package | Purpose |
|---------|---------|
| fastapi | HTTP/WebSocket framework |
| uvicorn[standard] | ASGI server |
| google-genai | Gemini API client |
| python-dotenv | Environment variable loading |
| chromadb | Vector database for memory storage |
| sentence-transformers | BAAI/bge-base-en-v1.5 embedding model |
| spacy | BM25 lemmatization (en_core_web_sm) |
| httpx | HTTP client for local llama.cpp |
| json-repair | Fixes malformed JSON from LLMs |
| jinja2 | Template rendering (FastAPI dependency) |

Requires Python 3.12+.

### Frontend (`frontend/package.json`)

| Package | Version | Purpose |
|---------|---------|---------|
| vue | ^3.5.32 | UI framework |
| pinia | ^3.0.4 | State management (WebSocket store) |
| typescript | ~6.0.2 | Type safety |
| vite | ^8.0.10 | Dev server + bundler |

### External services

| Service | Required | Configuration |
|---------|----------|---------------|
| Gemini API | Yes | `GOOGLE_API_KEY` env var. Optional `RHAPSODE_API_BASE` and `RHAPSODE_MODEL` (default: `gemini-2.0-flash`) |
| llama.cpp server | Optional | `start_llm.bat` on port 8012. Used for memory quality pipeline. Falls back gracefully if unavailable. |
