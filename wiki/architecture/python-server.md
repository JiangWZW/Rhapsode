---
sources:
  - server/rhapsode/app.py
  - server/rhapsode/gemini.py
  - server/rhapsode/prompt.py
  - server/rhapsode/memory.py
  - server/rhapsode/validator.py
  - server/rhapsode/lemmatization.py
  - server/rhapsode/spacy_models.py
  - server/pyproject.toml
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/stack]]"
  - "[[architecture/memory-system]]"
  - "[[decisions/ownership-split]]"
tags:
  - python-server
---

# Python server

The server layer lives in `server/rhapsode/`. It is a Python package providing a FastAPI WebSocket endpoint, LLM client, memory system callbacks, and prompt builder.

## Package structure (actual)

```
server/
├── pyproject.toml              # Package metadata + dependencies
├── scenarios/
│   ├── tavern.json             # Default scenario
│   └── konosuba.json           # Alternate scenario
└── rhapsode/
    ├── __init__.py
    ├── app.py                  # FastAPI app + WebSocket endpoint + session wiring
    ├── gemini.py               # Gemini LLM client
    ├── prompt.py               # Prompt builder
    ├── memory.py               # ChromaDB + embedding callbacks for MemorySystem
    ├── validator.py            # Local llama.cpp client for memory pipeline
    ├── lemmatization.py        # spaCy-based BM25 lemmatization
    └── spacy_models.py         # Lazy spaCy model loading
```

There is no separate WebSocket module, no session manager class, and no LLM abstraction layer. Gemini is called directly.

## Application entry point (`app.py`)

`app.py` is the monolithic entry point. It defines the FastAPI app, the WebSocket endpoint, and all session wiring logic.

### Startup (lifespan)

On startup, the app warms up the embedding model (`sentence-transformers`) and loads the spaCy lemmatization model. Both are loaded once and shared across connections.

### WebSocket endpoint (`/ws`)

Single endpoint handling the full session lifecycle:

```
Client connects → Scene loaded from scenario → Memory initialized →
Save loaded if exists → Director created → SceneLoop wired →
Seed/resume messages sent → Main loop: receive input → process turn → send response
```

**Session state is per-connection.** Each WebSocket connection creates its own Scene, Director, MemorySystem, and SceneLoop. There is no shared session store — closing the connection ends the session. Game state persists in the save file.

### Turn flow

1. Client sends `{ "type": "player_message", "content": "..." }`
2. Server sends `{ "type": "status", "state": "processing" }`
3. `SceneLoop.submit_input(text)` runs in a thread executor (avoids blocking asyncio)
4. Inside the loop: Director tick → prompt build → LLM call → response append
5. Post-turn: `memory.process_new_nodes()` stores new facts
6. Post-turn: `scene.save()` persists game state
7. Server sends `{ "type": "assistant_message", "content": "..." }`
8. Server sends `{ "type": "status", "state": "idle" }`

If a turn fails, the server sends an error message and reconstructs the SceneLoop from the existing scene.

### Director LLM callback

The Director's LLM callback is defined inline in `app.py`. It:

1. Retrieves established facts from memory via `retrieve_for_injection()`
2. Formats them as an "ESTABLISHED FACTS" block in the Director system prompt
3. Calls Gemini with the combined prompt
4. Extracts and validates JSON from the response
5. Falls back to empty transitions/nodes if JSON parsing fails

The Director system prompt enforces strict node format: atomic facts, max 15 words, no hedging, named entities required. Node types: `plot`, `scene`, `world`, `relationship`.

## Gemini client (`gemini.py`)

Minimal wrapper around the `google-genai` SDK:

```python
def complete(messages: list[dict]) -> str:
    # Lazily creates client with GOOGLE_API_KEY
    # Model defaults to gemini-2.0-flash (RHAPSODE_MODEL env override)
    # Optional RHAPSODE_API_BASE for custom endpoints
```

The client is a module-level singleton created on first call. No async — called synchronously from the thread executor.

## Prompt builder (`prompt.py`)

`build_prompt()` assembles the narrative prompt from four sources:

1. **System prompt** — from the scenario file
2. **Character names** — NPC names listed for context
3. **Director context blocks** — foreshadow and active context strings from the DirectorOutput
4. **History window** — recent messages from the SceneLoop's history snapshot

Returns a single concatenated string. The format is plain text (role-prefixed lines), not a structured messages array.

## Memory callbacks (`memory.py`)

Registers all seven Python callbacks on the C++ `MemorySystem` instance:

- **embed** — `SentenceTransformer.encode()` → JSON float array
- **lemmatize** — spaCy lemmatization with stop-word removal
- **store** — `ChromaDB.Collection.add()` with embeddings and metadata
- **query** — `ChromaDB.Collection.query()` with cosine similarity
- **update_meta** — `ChromaDB.Collection.update()`
- **get_by_meta** — `ChromaDB.Collection.get()` with where filter

Uses a shared `SentenceTransformer` instance — BAAI/bge-base-en-v1.5, 768 dimensions. ChromaDB is persistent at `./chroma` with per-scene collections.

## Local LLM client (`validator.py`)

Creates a callback for the MemorySystem's local LLM needs:

- Connects to llama.cpp server at `http://localhost:8012`
- Uses the OpenAI-compatible `/v1/chat/completions` endpoint
- Repairs malformed JSON with `json_repair` before returning
- Returns empty string on failure (C++ handles fallback logic)
- 120-second timeout for long quality-assessment prompts

## Lemmatization (`lemmatization.py`)

BM25 preprocessing:

- Loads spaCy `en_core_web_sm` model, lazy-downloaded via `spacy_models.py`
- Removes stop words and punctuation
- Returns lemmatized tokens
- Preserves `-ing` forms alongside lemmas (improves keyword matching)
- Falls back to lowercased input if spaCy is unavailable

## WebSocket protocol

```
Client → Server:
  { "type": "player_message", "content": "I draw my sword." }

Server → Client:
  { "type": "assistant_message", "content": "The barkeep looks alarmed..." }
  { "type": "status", "state": "processing" | "idle" }
  { "type": "error", "detail": "Turn failed: ..." }
```

On connection, the server sends all seed messages (fresh start) or the last 6 messages (resume) as `assistant_message` events so the client renders the conversation history.

## Configuration

| Variable | Default | Purpose |
|----------|---------|---------|
| `GOOGLE_API_KEY` | (required) | Gemini API key |
| `RHAPSODE_API_BASE` | (none) | Custom Gemini endpoint URL |
| `RHAPSODE_MODEL` | `gemini-2.0-flash` | Gemini model name |
| `RHAPSODE_SCENARIO` | `scenarios/tavern.json` | Scenario file path (relative to server/) |

Run with: `uvicorn rhapsode.app:app --reload`
