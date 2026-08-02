---
title: DeepSeek V4 — latency, streaming, token caps, context cache
date: 2026-08-02
tags: [deepseek, llm, latency, streaming, cache, thinking-mode]
---

# DeepSeek V4 — latency, streaming, token caps, context cache

Notes from a deep exploration (2026-08-02) while tuning Rhapsode’s DeepSeek narrator path (pro + thinking, tool rounds, non-streaming). Keep this page when switching branches so the conclusions stay reachable.

**Rhapsode context at time of writing:** narrator on `deepseek-v4-pro` with thinking; player / lifecycle / scheduler on flash with thinking off; tool rounds must replay `reasoning_content`; stage-labeled INFO logs in `server/rhapsode/llm.py`.

---

## Sources (primary first)

| Source | Role |
|--------|------|
| [Thinking Mode (official)](https://api-docs.deepseek.com/guides/thinking_mode) | Toggle, effort, streaming sample, tool-loop `reasoning_content` replay |
| [Context Caching / kv_cache (official)](https://api-docs.deepseek.com/guides/kv_cache) | Prefix hit rules, persistence units, usage fields |
| [Context Caching on Disk announcement](https://api-docs.deepseek.com/news/news0802/) | Disk cache = reuse of prompt-prefix work; latency/cost; MLA ↔ KV size |
| [DeepSeek-V3 #1464](https://github.com/deepseek-ai/DeepSeek-V3/issues/1464) | Non-streaming + default thinking ⇒ TTFB ≈ full reasoning time |
| [OpenAI Reasoning guide](https://developers.openai.com/api/docs/guides/reasoning) | Same class of failure: reasoning can consume the whole completion budget |
| [chat-deep.ai Thinking Mode live tests](https://chat-deep.ai/docs/deepseek-thinking-mode/) | Independent probes: shared `max_tokens`, empty `content` on `length` |
| [deepseek-harness](https://github.com/HenryZ838978/deepseek-harness) | Agent contract notes: prefix hygiene, reasoning replay, default thinking cost |

Secondary / community (useful, not gospel): deepseekai.guide streaming/caching writeups, Hermes agent issues on `reasoning_content` 400s and stuck tool loops.

---

## 1) Streaming — how it works and how we would use it

### Mechanism

- **`stream: false` (current):** one HTTP response after the whole generation finishes. With thinking on, that includes the full CoT first. Measured community case (~2.9K-token classifier on pro): ~32s with thinking, ~2.7s with thinking disabled ([#1464](https://github.com/deepseek-ai/DeepSeek-V3/issues/1464)). DeepSeek-side replies agree: document that non-streaming TTFB ≈ full reasoning time; latency-sensitive callers should stream **or** disable thinking.
- **`stream: true`:** Server-Sent Events (OpenAI-compatible chunks). Tokens arrive as they are produced. **Wall-clock compute is not free;** **perceived** latency drops to time-to-first-token. Billing is still by tokens, not by bytes on the wire (streaming guides).

Official thinking + stream pattern ([Thinking Mode](https://api-docs.deepseek.com/guides/thinking_mode)):

```python
response = client.chat.completions.create(
    model="deepseek-v4-pro",
    messages=messages,
    stream=True,
    reasoning_effort="high",
    extra_body={"thinking": {"type": "enabled"}},
)

reasoning_content = ""
content = ""
for chunk in response:
    if chunk.choices[0].delta.reasoning_content:
        reasoning_content += chunk.choices[0].delta.reasoning_content
    else:
        content += chunk.choices[0].delta.content or ""
```

Typical order with thinking: **`delta.reasoning_content` first, then `delta.content`** (and/or `delta.tool_calls`). Live probes report the same sequencing.

### Rhapsode implementation sketch (not done yet)

In `server/rhapsode/llm.py`:

1. Pass `stream=True` on narrator (optional elsewhere).
2. Accumulate `reasoning_content` / `content` / `tool_calls` (aggregate parallel tools by index if needed).
3. Log or push progress while waiting (console and/or WebSocket).
4. After the stream ends, build the same full assistant message already used for tool replay; continue the tool loop.

Constraints:

- A tool round still needs a **complete** assistant message before dispatch — streaming improves visibility *during* a round; it does not remove multi-round waits.
- Never treat streamed `reasoning_content` as narrator prose; engine still gets final `content` only.
- Streaming does **not** replace routing (flash + thinking off for cheap stages).

---

## 2) Does capping `max_tokens` make the model “hurry” its thinking?

### Short answer

**No evidence that `max_tokens` is a soft “finish quickly” signal.** It is a **hard generation stop**. The intentional depth knob is **`reasoning_effort`** (and/or `thinking: disabled`).

### What the docs and probes show

- In thinking mode, **reasoning and final answer share one completion budget** ([Thinking Mode](https://api-docs.deepseek.com/guides/thinking_mode) shape; [live tests](https://chat-deep.ai/docs/deepseek-thinking-mode/); [V4 output planning](https://chat-deep.ai/docs/deepseek-v4-context-output-limits/)).
- If the budget is too small, the call can return HTTP 200 with **`finish_reason: "length"`**, lots of `reasoning_tokens`, and **empty `content`** — truncation by the server, not adaptive short CoT.
- Same failure class on OpenAI reasoning models: reasoning can consume the whole completion budget before any visible answer ([OpenAI Reasoning](https://developers.openai.com/api/docs/guides/reasoning)).

| Mechanism | Effect | Model “plans around it”? |
|-----------|--------|---------------------------|
| `max_tokens` | Hard stop after N completion tokens (incl. reasoning) | **Not documented.** Observed: cut-off / empty answer |
| `reasoning_effort` (`low` / `high` / `max`) | Official thinking intensity ([Thinking Mode](https://api-docs.deepseek.com/guides/thinking_mode)) | **Yes — intended control** |
| `thinking: {type: "disabled"}` | Skip CoT | Fastest / cheapest path |

There is **no public DeepSeek claim** that the model is told a wall-clock timer or a remaining-token countdown as planning context. Time is a side effect of how many tokens it generates.

### Rhapsode takeaway

- Keep `max_tokens` **large enough** that reasoning + prose/JSON can finish (retry/bump on `length` is correct).
- Do **not** lower `max_tokens` hoping for denser thinking.
- To shorten thinking: lower **`reasoning_effort`**, or disable thinking on that call.

---

## 3) Prefix / disk context cache — what it is

### Is it “KV cache”?

**Related, but not your local GPU session KV.**

DeepSeek’s announcement ties disk caching to attention **KV** economics: MLA shrinks the context KV representation enough that reusable prefix state can be **persisted on a disk array** and reloaded instead of recomputed ([news0802](https://api-docs.deepseek.com/news/news0802/)). The guide lives under the `kv_cache` path ([Context Caching](https://api-docs.deepseek.com/guides/kv_cache)).

Accurate picture:

1. Normally each request re-encodes the prompt and builds attention state for all input tokens.
2. **Context Caching on Disk** stores reusable **prompt-prefix** state server-side.
3. Later requests whose **leading tokens match a persisted cache prefix unit** get **cache hits**: cheaper input billing + much lower first-token latency on long reused prefixes (official example: highly reusable 128K prompt, first-token latency **~13s → ~500ms**).

It is **not**:

- a response cache (same question → same answer)
- application memory / RAG store
- something you enable with a special request flag (automatic for all users)

### Official hit rules

From [Context Caching](https://api-docs.deepseek.com/guides/kv_cache) and [news0802](https://api-docs.deepseek.com/news/news0802/):

- Only **identical prefixes from the 0th token** count; a match in the **middle** does **not** hit.
- Hits require a persisted **cache prefix unit** (request-boundary persistence, common-prefix detection, and fixed-interval carve-outs for long I/O).
- Best-effort; not 100% guaranteed. Unused entries expire in hours–days.
- Monitor: `prompt_cache_hit_tokens` / `prompt_cache_miss_tokens` in `usage`.
- Cache is per-account / isolated.
- Older note: storage unit historically **64 tokens** (very short prefixes may not cache).

Output is still freshly generated; cache only accelerates / discounts the **repeated input prefix**.

### Mental model

```text
Request N:   [==== STABLE PREFIX ====][variable]
                    │
                    ▼  persist (KV-related prefix state on disk)
Request N+1: [==== STABLE PREFIX ====][different variable]
                    ▲
                    └── HIT: skip recompute of prefix → cheaper + faster TTFB
                         then generate new output as usual
```

### “Don’t put dates/noise in the cached prefix”

Because matching is **prefix-exact from token 0**, **any change at the beginning** invalidates the shared head.

**Bad** (timestamp in system → every minute busts the cache for the big stable tools/instructions):

```text
System: You are the narrator. Today is 2026-08-02 21:40:11.
Tools:  [query_graph, …]
User:   <beat state>
```

**Good** (volatile material at the end):

```text
System: You are the narrator.          ← stable
Tools:  [query_graph, …]               ← stable
User:   Current time: …                ← volatile
        <beat state>
```

Same for request IDs, “session started at …”, reshuffled tool JSON, regenerating system prompts with different whitespace — all shift the prefix and kill hits.

### Rhapsode-specific cache hygiene

Cache helps most when:

1. Narrator **system / instructions** text is byte-stable across beat / graph / tool rounds.
2. **Tool schemas** keep stable order and wording.
3. Variable **turn state** stays in the trailing user message (already the usual shape: system then user).
4. No clocks / UUIDs / noise injected into the **front** of the prompt.

Across tool rounds of one call, the message list usually **extends** the previous prefix (`… + assistant + tool result`), so later rounds can hit earlier prefixes **if** earlier messages are replayed exactly (including `reasoning_content` when tools were used). Mutating earlier messages breaks the prefix.

---

## Related Rhapsode practice (as of this note)

| Practice | Why |
|----------|-----|
| Narrator: pro + thinking | Quality where it matters |
| Player / lifecycle / scheduler: flash, thinking off | Avoid default-thinking tax on cheap calls |
| Replay full assistant message in tool rounds | Official contract; avoids 400 / forced re-think |
| Cap tool rounds; stage-labeled wait/done logs | Visibility under non-streaming thinking |
| Large `max_tokens` + bump on `length` | Shared reasoning+content budget |

### Still open if turns feel slow

1. **Stream narrator** (console and/or WS progress) — biggest perceived-speed win without disabling thinking.
2. **Audit prompt prefixes** for accidental busts (dates, reshuffled tools, unstable system text).
3. Keep logging `reasoning_tokens` + `elapsed_ms` + `prompt_cache_hit_tokens` as the real budget/hit signals.

---

## Profiling (measure before optimizing)

**Goal:** know whether wall time is dominated by narrator beat, graph, tool rounds, lifecycle, or scheduler — and whether that time correlates with `reasoning_tokens` (decode) or cache misses (prefill).

### Enable

```bash
# PowerShell
$env:RHAPSODE_LLM_PROFILE = "1"
# optional:
$env:RHAPSODE_LLM_PROFILE_PATH = "D:\cursor-workspace\Rhapsode\logs\llm_profile.jsonl"
```

Then run one autoplay turn (or a short session). Each API hop appends one JSONL row (no extra console channel — normal DeepSeek done lines already include `phase=` and `elapsed_ms`).

### What each event measures (reliable)

| Field | Meaning |
|-------|---------|
| `wall_ms` | Client `time.monotonic()` around `chat.completions.create` — full RTT (queue + prefill + decode + network) |
| `stage` | `narrator:<id>`, `lifecycle`, `scheduler`, `player`, `weave`, … |
| `phase` | Narrator: `beat` vs `graph` (via `GRAPH_UPDATE`); lifecycle `verdict`; scheduler `pick` |
| `tool_round` | Tool-loop round index — each row is one **LLM hop** (wait between tool uses) |
| `reasoning_tokens` | From provider `usage` — proxy for thinking decode work |
| `prompt_cache_hit_tokens` / `miss` | DeepSeek disk prefix cache |
| `tools` | Names requested on that round (not local tool runtime) |

Local tool dispatch is not recorded. C++ still logs turn `begin`/`end` and narrator `[2/4]` / `[2b/4]` — correlate by wall clock / log order.

### What we cannot split yet (non-streaming)

**Prefill vs decode** inside one `wall_ms`. Without streaming TTFB, treat:

- high `reasoning_tokens` + high `wall_ms` → mostly **decode/thinking**
- huge `prompt_cache_miss_tokens`, low reasoning → mostly **prefill**
- many tool rounds → sum of round `wall_ms`

Optional next step: stream-profile mode (`ttfb_ms` + `stream_ms`) if we need a hard prefill/decode split.

### How to read one turn

```powershell
Get-Content logs\llm_profile.jsonl | ConvertFrom-Json |
  Group-Object stage, phase |
  ForEach-Object {
    $ms = ($_.Group | Measure-Object wall_ms -Sum).Sum
    "stage=$($_.Name) calls=$($_.Count) wall_ms=$ms"
  }
```

Or inspect `reasoning_tokens` vs `wall_ms` per `phase=beat` row first — that answers “is the bloody narrator call thinking-bound?”

---

## Quick FAQ

**Does streaming make Pro+thinking faster in wall clock?**  
Usually not meaningfully; it makes the wait *observable* and improves UX. Compute still runs reasoning then answer/tools.

**Should we tighten `max_tokens` to force short CoT?**  
No. Use `reasoning_effort` / disable thinking. Tight caps risk empty `content` with `finish_reason=length`.

**Is disk cache the same as OpenAI prompt caching?**  
Same *idea* (reuse prompt prefix work), different product details. DeepSeek’s is automatic prefix matching from token 0 with disk-backed persistence; see official `kv_cache` guide for current hit rules.
