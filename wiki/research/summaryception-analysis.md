---
title: Summaryception Extension Analysis
date: 2026-05-22
tags: [summarization, sillytavern, extension, layered-memory]
---

# Summaryception v5.5.2 — Architecture Analysis

Summaryception is a **SillyTavern third-party extension** that implements recursive layered summarization for long-running chat sessions. It keeps an LLM's context window manageable by progressively compressing older conversation turns into hierarchical summaries — the "ception" in the name refers to summaries-of-summaries, recursively.

Source: `e:\Extension-Summaryception` ([GitHub](https://github.com/Lodactio/Extension-Summaryception))

---

## File Structure

```
Extension-Summaryception/
├── manifest.json        (12 lines)   ST extension registration
├── index.js             (2,746 lines) Core pipeline, UI, events
├── connectionutil.js    (664 lines)   LLM backend routing
├── settings.html        (425 lines)   Settings panel UI
├── style.css            (472 lines)   Styling
├── README.md            (306 lines)   User documentation
└── LICENSE                            AGPL-3.0
```

Not a browser extension — no service workers, content scripts, or `package.json`. Everything runs in SillyTavern's page context via `SillyTavern.getContext()`.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────────→ +→                        SillyTavern Host                              → +→                                                                     → +→  eventSource ──────────────→                                        → +→  (MESSAGE_RECEIVED,        →          setExtensionPrompt()          → +→   CHAT_CHANGED,            →               ▼                       → +→   GENERATION_STARTED)      →               →                        → +→                            ▼              →                        → +→  ┌──────────────────── index.js ────────────────────────────────→   → +→  →                                                              →   → +→  →  onMessageReceived() ─── 500ms delay ──▼maybeSummarizeTurns→   → +→  →                                                →             →   → +→  →         ┌──────────────────────────────────────→             →   → +→  →         ▼                                                   →   → +→  →  ┌─────────────────→     overflow > threshold?               →   → +→  →  → Count visible   │ ├── yes ──▼showCatchupDialog()          →   → +→  →  → assistant turns →                  →                      →   → +→  →  └─────────────────→         ┌────────└────────→            →   → +→  →           →                  ▼       ▼       ▼           →   → +→  →    count > limit?      runCatchup  skip   oneBatch          →   → +→  →           →                                                  →   → +→  →           ▼                                                 →   → +→  →  summarizeOneBatch() ────────────────────────────────────→   →   → +→  →         →                                                →   →   → +→  →         鈹溾攢 buildPassageFromRange(chat, start, end)       →   →   → +→  →         鈹溾攢 buildFullContext(layer 0)                      →   →   → +→  →         鈹溾攢 callSummarizer(passage, context) ─────────→   →   →   → +→  →         →                                            →   →   →   → +→  →         →    ┌───────── connectionutil.js ────────→  →   →   →   → +→  →         →    →                                    →  →   →   →   → +→  →         →    →  sendSummarizerRequest(settings,   │ └─→   →   →   → +→  →         →    →    systemPrompt, userPrompt)       →      →   →   → +→  →         →    →         →                          →      →   →   → +→  →         →    →    ┌────└──────────────────────→   →      →   →   → +→  →         →    →    ▼        ▼       ▼       ▼  →      →   →   → +→  →         →    → default   profile   ollama   openai→      →   →   → +→  →         →    → (genRaw)  (CMRS)   (/api/   (SSE) →      →   →   → +→  →         →    →                     chat)          →      →   →   → +→  →         →    └────────────────────────────────────→      →   →   → +→  →         →                                                →   →   → +→  →         鈹溾攢 store snippet → layers[0]                     →   →   → +→  →         鈹溾攢 ghostMessagesUpTo(endIdx)                     →   →   → +→  →         鈹溾攢 maybePromoteLayer(0) ──▼recursive            →   →   → +→  →         鈹斺攢 updateInjection() ──▼assembleSummaryBlock()  →   →   → +→  →                                         →                →   →   → +→  └─────────────────────────────────────────├────────────────→   →   → +→                                            →                    →   → +→                                            ▼                   →   → +→                               setExtensionPrompt(block, depth)  →   → +└─────────────────────────────────────────────────────────────────────→ +```

---

## Layered Memory Model

```
                    ┌─────────────────────────────────────→ +                    →         LLM Context Window           → +                    →                                     → +                    →  ┌───────────────────────────────→  → +   deepest layer    →  →  Layer N  (meta^N summaries)  →  →  → oldest, most compressed
                    →  ├───────────────────────────────→  → +                    →  →  Layer 2  (meta-meta)         →  → +                    →  ├───────────────────────────────→  → +                    →  →  Layer 1  (meta summaries)    →  → +                    →  ├───────────────────────────────→  → +                    →  →  Layer 0  (turn summaries)    →  →  → direct summaries of batches
                    →  ├───────────────────────────────→  → +  injection depth   →  →  ─── injection point ───      →  →  depth = verbatimTurns × 2
                    →  ├───────────────────────────────→  → +                    →  →  Verbatim Messages (last N)   →  →  → full text, no compression
                    →  └───────────────────────────────→  → +                    └─────────────────────────────────────→ +
    ────────── Chat History (not in context) ──────────
    →  Ghosted messages (sc_ghosted=true, /hide'd)    → +    →  Still visible to user, excluded from LLM       → +    └─────────────────────────────────────────────────→ +```

---

## Core Pipeline: Step-by-Step

### 1. Trigger — `onMessageReceived`

Every new assistant message triggers the pipeline after a 500ms debounce:

```js
function onMessageReceived(messageIndex) {
    const { chat } = SillyTavern.getContext();
    const msg = chat[messageIndex];
    if (msg && !msg.is_user && !msg.is_system) {
        setTimeout(async () => {
            await maybeSummarizeTurns();
            updateInjection();
            updateUI();
        }, 500);
    }
}
```

### 2. Threshold Check — `maybeSummarizeTurns`

Counts visible (non-ghosted) assistant turns. If count exceeds `verbatimTurns` (default 10), the oldest overflow gets summarized:

```js
async function maybeSummarizeTurns() {
    const s = getSettings();
    if (!s.enabled || s.pauseSummarization || isSummarizing) return;

    const { chat } = SillyTavern.getContext();
    const allAssistantTurns = getAssistantTurns(chat);
    const visibleTurns = allAssistantTurns.filter(t => !chat[t.index].extra?.sc_ghosted);

    if (visibleTurns.length <= s.verbatimTurns) return;

    const overflow = visibleTurns.length - s.verbatimTurns;
    const backlogThreshold = s.turnsPerSummary * 2;

    if (overflow > backlogThreshold && !catchupDismissed) {
        // Large backlog → show modal: Process All / Skip / Just One Batch
        const choice = await showCatchupDialog(overflow, batchesNeeded);
        // ...
    }

    // Normal: single batch
    await summarizeOneBatch(visibleTurns);
}
```

### 3. Batch Summarization — `summarizeOneBatch`

Takes up to `turnsPerSummary` (default 3) eligible turns, builds a passage, calls the LLM:

```js
const startIdx = batch[0].index;
const endIdx = batch[batch.length - 1].index;

const passageStart = store.summarizedUpTo < 0 ? 0 : store.summarizedUpTo + 1;
const storyTxt = buildPassageFromRange(chat, passageStart, endIdx);
const contextStr = buildFullContext(0);

const summary = await callSummarizer(storyTxt, contextStr);

store.layers[0].push({
    text: summary,
    turnRange: [passageStart, endIdx],
    timestamp: Date.now(),
});

store.summarizedUpTo = Math.max(store.summarizedUpTo, endIdx);
await ghostMessagesUpTo(endIdx);
await maybePromoteLayer(0);
```

### 4. Passage Construction — `buildPassageFromRange`

Formats chat messages as labeled lines, skipping user-hidden and empty messages:

```js
function buildPassageFromRange(chat, startIdx, endIdx) {
    const lines = [];
    for (let i = startIdx; i <= endIdx; i++) {
        const m = chat[i];
        if (!m || !m.mes?.trim()) continue;

        const isUserHidden = (m.is_system || m.is_hidden) && !m.extra?.sc_ghosted;
        if (isUserHidden) continue;

        const speaker = m.is_user ? 'Player' : 'Assistant';
        lines.push(`${speaker}: ${m.mes.trim()}`);
    }
    return lines.join('\n');
}
```

### 5. Layer Promotion — `maybePromoteLayer`

When Layer N exceeds `snippetsPerLayer` (default 30), overflow promotes to Layer N+1:

```js
async function maybePromoteLayer(layerIndex) {
    const layer = store.layers[layerIndex];
    if (!layer || layer.length <= s.snippetsPerLayer) return;
    if (layerIndex >= s.maxLayers - 1) return;

    if (!store.layers[layerIndex + 1]) store.layers[layerIndex + 1] = [];
    const destLayer = store.layers[layerIndex + 1];

    // First promotion: FREE SEED (no LLM call)
    if (destLayer.length === 0) {
        const seed = layer.shift();
        seed.promoted = true;
        destLayer.push(seed);
        return;
    }

    // Later: merge N snippets via LLM
    const toMerge = layer.splice(0, s.snippetsPerPromotion);
    const storyTxt = toMerge.map(sn => sn.text).join(' ');
    const contextStr = buildFullContext(layerIndex + 1);
    const metaSummary = await callSummarizer(storyTxt, contextStr);

    destLayer.push({
        text: metaSummary,
        fromLayer: layerIndex,
        mergedCount: toMerge.length,
        timestamp: Date.now(),
    });

    // Recursive check
    if (destLayer.length > s.snippetsPerLayer) {
        await maybePromoteLayer(layerIndex + 1);
    }
}
```

**Promotion flow:**

```
Layer 0 exceeds snippetsPerLayer (30)?
         → +         ├── Destination empty? ──▼FREE SEED (shift oldest, no LLM call)
         → +         └── Destination has data? ──▼MERGE:
                  → +                  鈹溾攢 Splice snippetsPerPromotion (3) oldest from Layer N
                  鈹溾攢 Concatenate their text
                  鈹溾攢 Call LLM with deeper-layer context
                  鈹斺攢 Push meta-summary to Layer N+1
                           → +                           └── Recurse if Layer N+1 also overflowed
```

### 6. Injection — `assembleSummaryBlock`

Assembles all layers (deepest-first) into a single block, inserts into context at `depth = verbatimTurns × 2`:

```js
function assembleSummaryBlock() {
    const snippets = [];

    // Deepest layers first (most compressed, oldest)
    for (let i = store.layers.length - 1; i >= 1; i--) {
        const layer = store.layers[i];
        if (!layer || layer.length === 0) continue;
        for (const sn of layer) snippets.push(sn.text);
    }

    // Layer 0 last (most recent turn summaries)
    if (store.layers[0]?.length > 0) {
        for (const sn of store.layers[0]) snippets.push(sn.text);
    }

    return s.injectionTemplate.replace('{{summary}}', snippets.join(' '));
}

function updateInjection() {
    const summaryBlock = assembleSummaryBlock();
    const depth = s.verbatimTurns * 2;
    setExtensionPrompt(MODULE_NAME, summaryBlock, 1, depth, false, 0);
}
```

---

## Connection Routing (`connectionutil.js`)

Four backends via `connectionSource` setting:

```
┌─────────────── sendSummarizerRequest() ───────────────────→ +→                                                           → +→  switch(connectionSource)                                 → +→     →                                                     → +→     鈹溾攢 'default' ──▼generateRaw({systemPrompt, prompt})  → +→     →                 Uses ST's active connection          → +→     →                                                     → +→     鈹溾攢 'profile' ──▼ConnectionManagerRequestService      → +→     →                 .sendRequest(profileId, messages)    → +→     →                                                     → +→     鈹溾攢 'ollama'  ──▼fetch(/proxy/localhost:11434/api/chat)→ +→     →                 Non-streaming, temp=0.3             → +→     →                                                     → +→     鈹斺攢 'openai'  ──▼fetch(/v1/chat/completions)          → +→                       Streaming SSE, temp=0.8             → +└───────────────────────────────────────────────────────────→ +```

| Mode | Mechanism | Notes |
|------|-----------|-------|
| `default` | `generateRaw()` | Active ST connection; **prompt isolation** disables all RP prompts during call |
| `profile` | `ConnectionManagerRequestService.sendRequest()` | ST Connection Profile (March 2025+) |
| `ollama` | `fetch` → `/api/chat` | Via CORS proxy; non-streaming; temp 0.3 |
| `openai` | `fetch` streaming SSE | `/v1/chat/completions`; handles local/cloud routing; temp 0.8 |

### Prompt Isolation (Default Mode)

Before calling `generateRaw`, all prompt manager toggles are disabled and restored in `finally`:

```js
const isDefaultMode = !s.connectionSource || s.connectionSource === 'default';
const snapshot = isDefaultMode ? snapshotPromptToggles() : null;
if (isDefaultMode) disableAllPromptToggles();

try {
    // ... LLM call ...
} finally {
    if (isDefaultMode && snapshot) restorePromptToggles(snapshot);
}
```

This prevents RP system prompts from contaminating the summarization task.

### CORS Proxy

All local requests route through SillyTavern's `/proxy/` endpoint with fallback to direct fetch:

```js
function proxiedUrl(url, useProxy = true) {
    if (!useProxy) return url;
    return `/proxy/${url}`;
}
```

---

## Retry System

```js
const RETRY_CONFIG = {
    maxRetries: 5,
    baseDelay: 2000,      // 2s initial
    maxDelay: 60000,      // 60s cap
    backoffMultiplier: 2,
    retryableStatuses: [429, 500, 502, 503, 504],
};
```

- Exponential backoff with jitter: 2s → 4s → 8s → 16s → 32s (capped at 60s)
- Respects `Retry-After` headers from server
- Non-retryable: missing config, 401, deleted profile, user abort
- Retryable: rate limits, 5xx, network errors, timeouts, empty responses
- 120s hard timeout per request via `Promise.race`
- `ConnectionError` class carries explicit `.retryable` flag

---

## Ghosting (Non-Destructive Hiding)

```js
async function ghostMessage(messageIndex) {
    const msg = chat[messageIndex];
    if (!msg || msg.extra?.sc_ghosted) return;

    msg.extra.sc_ghosted = true;

    // Track which messages WE hid
    if (!store.ghostedIndices.includes(messageIndex)) {
        store.ghostedIndices.push(messageIndex);
    }

    // Only visually hide if ghosting enabled
    if (!s.disableGhosting) {
        await SillyTavern.getContext().executeSlashCommandsWithOptions(
            `/hide ${messageIndex}`, { showOutput: false }
        );
    }
}
```

Key principles:
- **Non-destructive** — messages remain in chat file, just flagged with `extra.sc_ghosted`
- **Tracked** — `store.ghostedIndices` records which messages Summaryception hid (vs user-hidden)
- **Reversible** — `/sc-clear` or "Clear Memory" unghost everything via `/unhide`
- **Optional** — `disableGhosting` mode marks metadata only (for compatibility with other extensions)

---

## Output Cleaning

Strips reasoning model artifacts before storing summaries:

```js
function cleanSummarizerOutput(raw) {
    let text = raw;

    // Configurable literal patterns
    for (const pattern of s.stripPatterns) {
        while (text.includes(pattern)) text = text.replace(pattern, '');
    }

    // Regex blocks for reasoning models
    const blockPatterns = [
        /<\|channel>thought[\s\S]*?<channel\|>/gi,
        /<thinking>[\s\S]*?<\/thinking>/gi,
        /<output>([\s\S]*?)<\/output>/gi,       // keeps content inside
        /<reasoning>[\s\S]*?<\/reasoning>/gi,
        /<thought>[\s\S]*?<\/thought>/gi,
        /<reflect>[\s\S]*?<\/reflect>/gi,
        /<inner_monologue>[\s\S]*?<\/inner_monologue>/gi,
    ];

    for (const regex of blockPatterns) {
        if (regex.source.includes('output')) {
            text = text.replace(regex, '$1');  // Keep <output> content
        } else {
            text = text.replace(regex, '');     // Strip reasoning blocks
        }
    }

    return text.replace(/\n{3,}/g, '\n').trim();
}
```

---

## Data Persistence

### Global Settings (`extensionSettings.summaryception`)

Persisted via `saveSettingsDebounced()`:

| Key | Default | Purpose |
|-----|---------|---------|
| `verbatimTurns` | 10 | How many recent turns stay uncompressed |
| `turnsPerSummary` | 3 | Batch size per summarization call |
| `snippetsPerLayer` | 30 | Max snippets before promotion |
| `snippetsPerPromotion` | 3 | How many snippets merge per promotion |
| `maxLayers` | 5 | Maximum recursion depth |
| `connectionSource` | `'default'` | Which backend to use |
| `promptPreset` | `'narrative'` | `'narrative'` / `'gamestate'` / `'custom'` |

### Per-Chat Store (`chatMetadata.summaryception`)

```js
{
    layers: [
        // Layer 0: [{ text, turnRange: [start, end], timestamp }]
        // Layer 1: [{ text, fromLayer, mergedCount, timestamp }]
        // ...
    ],
    summarizedUpTo: number,    // last summarized message index
    ghostedIndices: number[],  // messages we hid (not user-hidden)
}
```

---

## LLM Prompt Template

Two presets with XML-structured prompts:

```xml
<player_name>{{player_name}}</player_name>

<prior_context>{{context_str}}</prior_context>

<passage_in_question>{{story_txt}}</passage_in_question>

Summarize only the necessary elements from the passage_in_question
to coherently continue the prior_context.
```

| Preset | Focus |
|--------|-------|
| **narrative** | Character interactions, dialogue tone, emotional beats, atmosphere, themes |
| **gamestate** | Plot points, quests, location changes, interactables, world state |

System prompt: `"You are a precise narrative-state tracker. You output only the summary line — no preamble, no commentary, no markdown."`

---

## Branch Repair

Handles SillyTavern's chat branching where metadata is copied but chat is truncated:

```js
async function repairIfBranched() {
    if (store.summarizedUpTo >= chatLength) {
        // Trim snippets referencing turns beyond branch point
        store.layers[0] = store.layers[0].filter(sn =>
            !sn.turnRange || sn.turnRange[1] < chatLength
        );

        // Recalculate summarizedUpTo
        const maxEnd = Math.max(...store.layers[0]
            .filter(sn => sn.turnRange)
            .map(sn => sn.turnRange[1]));
        store.summarizedUpTo = maxEnd;

        // Trim ghost tracking
        store.ghostedIndices = store.ghostedIndices.filter(idx => idx < chatLength);
    }
}
```

---

## Slash Commands

| Command | Effect |
|---------|--------|
| `/sc-status` | Show layer stats (snippet counts, summarizedUpTo) |
| `/sc-clear` | Clear all memory + unghost all messages |
| `/sc-preview` | Preview the assembled injection block |

---

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| Non-destructive ghosting | Messages stay in chat file; uses ST's native `/hide`/`/unhide` |
| Prompt isolation in default mode | Disables all preset prompts so RP system prompts don't contaminate summarizer |
| Free seed on first promotion | Avoids an LLM call when bootstrapping a new layer |
| Recursive promotion | If Layer N+1 also exceeds capacity, promotes again (up to `maxLayers`) |
| Exponential backoff + abort | Robust against rate limits; user can cancel anytime |
| Streaming for OpenAI mode | Avoids non-streaming token ceiling (4096 on many providers) |
| CORS proxy fallback | Tries ST's `/proxy/` route first, falls back to direct fetch |
| Per-chat metadata | Each chat has independent summary state; branching is handled |

---

## Data Flow Summary

```
New message arrives
     → +     ▼+Count visible turns > verbatimTurns?
     → yes
     ▼+Take oldest N eligible turns ──▼Build "Player:/Assistant:" passage
     →                                      → +     ▼                                     ▼+Gather all existing summaries       Send to LLM (system + user prompt)
as "prior context"                          → +                                            ▼+                                   Clean output (strip reasoning tags)
                                            → +                                            ▼+                              Store snippet in layers[0]
                                            → +                              ┌─────────────├──────────────→ +                              ▼            ▼             ▼+                       Ghost messages   Promote if    Update injection
                       (/hide 0..N)    layer full     (setExtensionPrompt)
```

---

## Theoretical Framing: Recursive Filter / Running Accumulator

Summaryception's core operation is a **recursive (IIR) filter** applied to narrative state. The output of all previous summarization steps is fed back as input context to the next step.

### The Recurrence Relation

```
S(n) = LLM( x(n), S(n-1) )

where:
  x(n)   = buildPassageFromRange(passageStart, endIdx)   — new raw input (chat text)
  S(n-1) = buildFullContext(0)                           — ALL prior output concatenated
  LLM    = callSummarizer with prompt instructing "exclude what's already captured"
  S(n)   = new snippet pushed to layers[0]
```

The prompt instruction *"Exclude anything already covered in Prior Context"* is the mechanism that makes the LLM produce the **residual (delta)** rather than a full re-summary. Without it, the system would be a sliding-window re-summarizer instead of a recursive accumulator.

### Verified Signal Path

**Layer 0 recurrence** (`summarizeOneBatch`, lines 1042—062):
```js
const contextStr = buildFullContext(0);       // ALL existing snippets from ALL layers
const summary    = callSummarizer(storyTxt, contextStr);
store.layers[0].push({ text: summary });      // output appended to accumulator
```

`buildFullContext(0)` (lines 622—35) iterates from the deepest layer down to Layer 0, concatenating every snippet. The full accumulator state is the feedback signal.

**Promotion recurrence** (`maybePromoteLayer`, lines 1425—446):
```js
const contextStr = buildFullContext(layerIndex + 1);  // only DEEPER layers as context
const metaSummary = callSummarizer(mergedSnippets, contextStr);
destLayer.push({ text: metaSummary });
```

At promotion, context is restricted to layers *above* the current one. Layer 1 promotion sees Layer 2+ as context, not Layer 0. The promotion kernel conditions only on the slower-changing state.

### IIR Filter Mapping

| IIR filter concept | Summaryception equivalent |
|---|---|
| Accumulator state `y[n-1]` | `buildFullContext(0)` — all prior output concatenated |
| New input sample `x[n]` | `buildPassageFromRange(start, end)` — raw chat text |
| Filter kernel `H` | The LLM + prompt (produces delta conditioned on state) |
| Output `y[n] = H(x[n], y[n-1])` | New snippet pushed to `layers[0]` |
| Decimation / downsampling | `snippetsPerLayer` threshold triggers promotion |
| Multi-rate / cascaded stages | Layers 0, 1, 2, ..., N with increasing time constants |
| Polyphase decimation buffer | Accumulate at fast rate, drain N → emit 1 at threshold |

### Multi-Rate Decomposition (Frequency Bands)

```
Layer 0:  high-freq    — individual scenes, dialogue beats, immediate actions
Layer 1:  mid-freq     — relationship shifts, plot threads, character arcs
Layer N:  low-freq     — overall story shape, world state, thematic structure
```

The `snippetsPerLayer` (default 30) acts as the decimation factor. Once 30 samples accumulate at one rate, `snippetsPerPromotion` (default 3) oldest are drained, compressed, and emitted as 1 sample at the next slower rate:

```
Layer 0:  append, append, append, ... [30 reached] → drain 3, emit 1 to Layer 1
Layer 1:  append, append, append, ... [30 reached] → drain 3, emit 1 to Layer 2
```

### Where the Analogy Breaks

1. **Non-linear, non-deterministic kernel** — the LLM is not a fixed transfer function. Same input can produce different output across runs.
2. **Irreversible lossy compression** — a true IIR filter preserves all information (reshapes spectrum); this permanently discards detail. Information lost at any stage is unrecoverable.
3. **Variable "sample rate"** — batches are fixed in turn count (`turnsPerSummary`) but variable in content/token density.
4. **Context window ceiling** — as the accumulator grows, `contextStr` can eventually exceed the LLM's context window. There is no explicit truncation of the feedback signal, risking quality degradation in very long sessions.
5. **Prompt-as-filter-design** — the "kernel" behavior depends entirely on natural language instruction. If the prompt fails to enforce delta-only output, the system degenerates into redundant re-summarization.

### Implications

- **Error accumulation**: Like any recursive system, errors compound. A bad summary at step N pollutes all subsequent context, biasing future summaries.
- **Information half-life**: Each promotion level roughly cubes the compression ratio (3 snippets → 1). After Layer 2, original dialogue is ~27:1 compressed. Specific details have a finite "half-life" in the system.
- **No error correction**: Unlike digital filters with feedback quantization noise shaping, there's no mechanism to detect or correct accumulated drift. The snippet browser (manual edit) is the only human-in-the-loop correction path.

---

## Relevance to Rhapsode

Summaryception's layered recursive summarization is directly comparable to Rhapsode's [[memory-system]] design. Key parallels and differences:

- Both use hierarchical compression to manage long conversations
- Summaryception operates purely at the prompt level (injection); Rhapsode integrates memory into C++ data structures
- Summaryception's "ghosting" is analogous to Rhapsode's history windowing in the scene loop
- The prompt isolation pattern (disabling RP prompts during summarization) is a lesson worth adopting
- Summaryception's branch repair handles a real-world edge case that Rhapsode's save/load system should also address
- The recursive filter framing suggests Rhapsode should consider **error correction mechanisms** (e.g., periodic re-grounding against raw history) that Summaryception lacks
