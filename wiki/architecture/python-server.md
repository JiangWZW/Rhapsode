---
sources:
  - server/rhapsode/app.py
  - server/rhapsode/llm.py
  - server/rhapsode/gemini.py
  - server/rhapsode/prompt.py
  - server/rhapsode/memory.py
  - server/rhapsode/character_agent.py
  - server/rhapsode/validator.py
  - server/rhapsode/lemmatization.py
  - server/rhapsode/spacy_models.py
  - server/pyproject.toml
last_updated: 2026-05-17
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

The server layer lives in `server/rhapsode/`. It is a Python package providing a FastAPI WebSocket endpoint, multi-provider LLM abstraction, merged narrator+graph prompt builder, memory system callbacks, character synthesis agent, and supporting utilities.

## Package structure

```
server/
├── pyproject.toml              # Package metadata + dependencies
├── .env.example                # Configuration template
├── scenarios/
│   ├── tavern.json             # Default scenario
→   ├── konosuba.json           # Alternate scenario
→   └── siege.json              # Siege scenario
└── rhapsode/
    ├── __init__.py
    ├── app.py                  # FastAPI app + WebSocket endpoint + session wiring
    ├── llm.py                  # Multi-provider LLM abstraction (Gemini / DeepSeek)
    ├── gemini.py               # Standalone Gemini client (superseded by llm.py)
    ├── prompt.py               # Merged narrator + plot-graph prompt builder
    ├── character_agent.py      # NPC dialogue synthesis via local llama.cpp
    ├── memory.py               # ChromaDB + embedding callbacks for MemorySystem
    ├── validator.py            # Local llama.cpp client for memory pipeline
    ├── lemmatization.py        # spaCy-based BM25 lemmatization
    └── spacy_models.py         # Lazy spaCy model loading
```

## Application entry point (`app.py`)

`app.py` is the monolithic entry point. It defines the FastAPI app, the WebSocket endpoint, and all session wiring logic.

### Startup (lifespan)

On startup, the app warms up the embedding model (`sentence-transformers`) and loads the spaCy lemmatization model. Both are loaded once and shared across connections.

### WebSocket endpoint (`/ws`)

Single endpoint handling the full session lifecycle:

```
Client connects → Scene loaded from scenario → Memory initialized →
Save loaded if exists → Director created on WorldGraph → SceneLoop wired → +Seed/resume messages sent → Main loop: receive input → process turn → send outputs
Save loaded if exists → Director created → SceneLoop wired →
Seed/resume messages sent → Main loop: receive input → process turn → send response
```

**Session state is per-connection.** Each WebSocket connection creates its own Scene, Director, MemorySystem, and SceneLoop. There is no shared session store — closing the connection ends the session. Game state persists in the save file.

### Turn flow

1. Client sends `{ "type": "player_message", "content": "..." }`
2. Server sends `{ "type": "status", "state": "processing" }`
3. `SceneLoop.submit_input(text)` runs in a thread executor (avoids blocking asyncio)
4. Inside the loop (single LLM call): Director focus payload → merged prompt build → LLM call → split prose/JSON → Director applies graph updates → narrator message appended → character synthesis → character messages appended
5. Post-turn: `memory.process_new_nodes()` stores new facts; `memory.sync_resolved()` stores resolved nodes
6. Post-turn: `scene.save()` persists game state
7. Server iterates `loop.take_last_turn_outputs()` and sends each as a `scene_message`
8. Server sends `{ "type": "status", "state": "idle" }`

If a turn fails, the server sends an error message and reconstructs the SceneLoop from the existing scene.

### Session wiring (`_wire_loop`)

```python
loop = SceneLoop()
loop.load_scene(scene)
loop.set_director(director)
loop.set_character_synth_callback(make_character_synth(scene))
loop.set_prompt_callback(
    lambda hist, scene_obj, director_out, focus_json: build_merged_prompt(
        hist, scene_obj, director_out,
        director_focus_json=focus_json,
        established_facts=_established_facts(memory, hist, scene_obj, director_out),
        active_characters=_active_characters(scene_obj),
    )
)
loop.set_llm_callback(_call_llm)
```

The prompt callback retrieves established facts from memory and derives active characters from the WorldGraph's active nodes (matching node entities to scenario character names).

## LLM provider abstraction (`llm.py`)

Multi-provider abstraction selected via `RHAPSODE_PROVIDER` environment variable:

| Provider | Env var | Default model | SDK |
|----------|---------|---------------|-----|
| `gemini` (default) | `GOOGLE_API_KEY` | `gemini-2.0-flash` | `google-genai` |
| `deepseek` | `DEEPSEEK_API_KEY` | `deepseek-chat` | `openai` |

```python
def complete(messages: list[dict]) -> str:
    return _get_provider().complete(messages)
```

Messages use Gemini's format internally (`{role, parts: [{text}]}`). The DeepSeek provider converts to OpenAI format automatically. The provider is a module-level singleton created on first call.

`gemini.py` still exists as a standalone client but is superseded by `llm.py` in `app.py`.

## Merged prompt builder (`prompt.py`)

`build_merged_prompt()` assembles a single prompt that produces both narrative prose and structured graph updates:

```python
def build_merged_prompt(
    history_snapshot, scene, director_out=None, *,
    director_focus_json="{}",
    established_facts=None,
    active_characters=None,
) -> str:
```

The prompt structure:

1. **System prompt** — from the scenario file
2. **Narrative frame** — instructions for 2nd-person present-tense prose, no quoted dialogue
3. **Graph rules** — JSON schema for `transitions` and `new_nodes`
4. **Speech rules** — JSON schema for `speech_turns` (character/cue pairs)
5. **Active characters** — NPC names derived from active plot nodes
6. **Established memories** — facts retrieved from the memory system
7. **Active plot pressures** — context blocks from the Director's previous output
8. **Plot graph snapshot** — the Director's `focus_payload_json` (nodes + 2-hop context)
9. **Conversation backlog** — recent history messages

The LLM responds with prose followed by a `<<<RHAPSODE_JSON>>>` sentinel and a JSON block containing `transitions`, `new_nodes`, and `speech_turns`.

## Character synthesis agent (`character_agent.py`)

NPC dialogue synthesis via a local llama.cpp server:

```python
def make_character_synth(scene, url=LLAMA_URL):
    # Returns a closure matching CharacterSynthCallback
```

For each `(character_name, cue)` pair from the LLM's `speech_turns`:

1. Looks up the character's description from the scenario
2. Constructs a prompt with character sheet, narrator context, and stage direction
3. Calls the local llama.cpp server's OpenAI-compatible endpoint
4. Returns one in-character spoken line (no narration, no quotation marks)

Falls back to a parenthetical placeholder on failure.

## Memory callbacks (`memory.py`)

Registers all seven Python callbacks on the C++ `MemorySystem` instance:

- **embed** — `SentenceTransformer.encode()` → JSON float array
- **lemmatize** — spaCy lemmatization with stop-word removal
- **store** — `ChromaDB.Collection.add()` with embeddings and metadata
- **query** — `ChromaDB.Collection.query()` with cosine similarity
- **update_meta** — `ChromaDB.Collection.update()`
- **get_by_meta** — `ChromaDB.Collection.get()` with where filter

Uses a shared `SentenceTransformer` instance — BAAI/bge-base-en-v1.5, 768 dimensions. ChromaDB is persistent at `./chroma` with per-scene collections (`{scene_id}_facts` and `{scene_id}_entities`).

## Local LLM client (`validator.py`)

Creates a callback for the MemorySystem's local LLM needs:

- Connects to llama.cpp server at `RHAPSODE_LOCAL_LLM_URL` (default `http://localhost:8012`)
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
  { "type": "scene_message", "content": "...", "scene_kind": "narrator" }
  { "type": "scene_message", "content": "...", "scene_kind": "character", "speaker": "Barkeep" }
  { "type": "status", "state": "processing" | "idle" }
  { "type": "error", "detail": "Turn failed: ..." }
```

Each turn produces multiple `scene_message` events: one narrator message followed by zero or more character dialogue messages. The `scene_kind` field distinguishes narrator prose from NPC speech. Character messages include a `speaker` field with the NPC's name.

On connection, the server sends seed messages (fresh start) or the last 8 messages (resume) as `scene_message` events so the client renders the conversation history.

## Configuration

| Variable | Default | Purpose |
|----------|---------|---------|
| `GOOGLE_API_KEY` | (required for Gemini) | Gemini API key |
| `DEEPSEEK_API_KEY` | (required for DeepSeek) | DeepSeek API key |
| `RHAPSODE_PROVIDER` | `gemini` | LLM provider: `gemini` or `deepseek` |
| `RHAPSODE_API_BASE` | (none) | Custom Gemini endpoint URL |
| `RHAPSODE_MODEL` | provider-dependent | LLM model name |
| `RHAPSODE_SCENARIO` | `scenarios/tavern.json` | Scenario file path (relative to server/) |
| `RHAPSODE_LOCAL_LLM_URL` | `http://localhost:8012` | Local llama.cpp server for memory + character synth |
| `RHAPSODE_LOCAL_LLM_TIMEOUT` | `120` | Timeout in seconds for local LLM calls |
| `RHAPSODE_HOST` | (uvicorn default) | Server bind host |
| `RHAPSODE_PORT` | (uvicorn default) | Server bind port |

Run with: `uvicorn rhapsode.app:app --reload`
