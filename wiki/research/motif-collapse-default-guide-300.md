---
title: Motif collapse in autonomous sessions — the default-guide-300 post-mortem
date: 2026-08-05
tags: [session-pipeline, evaluation, memory, downsampling, feedback-loop, dynamics]
---

# Motif collapse in autonomous sessions — `default-guide-300` post-mortem

**Run:** `experiments/session_pipeline/runs/default-guide-300` — 300/300 turns, `end_reason=MaxTurns`,
0 pipeline errors, single scene (`konosuba`), no forks/merges, one failed `conclude` at turn 16.

**Symptom:** the session begins as a debt/quest plot (slime-fence job, Gerhardt's toad, guild tab)
and by turn ~100 has collapsed into an endless candle-lit "covenant" scene: toasts, hand-holding,
napkin Articles, paraphrased emotional beats. The quest is never played; the debt is never resolved;
"Saturday" never arrives. The automated report scored this run **0 errors** and sub-threshold
repetition (0.136 < 0.45), flagging only `length_collapse=0.723` and 5 empty beats.

**Thesis of this page:** the collapse is not a model quirk. It is the predictable behavior of a
**closed self-feedback loop with bounded, LLM-controlled memory and no exogenous input**. The
architecture guarantees that long-run dynamics factor through a low-dimensional bottleneck (the
mip summary); the run demonstrates that a salience-based, age-evicting bottleneck locks onto
whatever motif cluster wins early, and then reinforces it monotonically. Below: the code-verified
loop structure, the forensic evidence from the saves, a formal framing that replaces the loose
"fixed-point theorem" intuition, falsifiable predictions, and ranked fixes.

---

## 1. The feedback loop as implemented (code-verified)

All paths verified against the working tree on 2026-08-05.

### 1.1 What the narrator actually sees each beat

Built in `TurnExecutor::build_turn_prompt` → `build_narrator_turn_state`
(`core/src/turn_executor_narrator.cpp`, `core/src/narrator_prompt.cpp`):

| Section | Source | Hard cap |
|---|---|---|
| Cast | roster, on/off-stage | — |
| Live storylines board | one beat, cleared after use | — |
| **Story so far** | `render_text_downsampling` (mip pyramid) | **1500 chars** (`kMaxStoryChars`) |
| **Turn transcript** | `scene.history` window | **last 6 messages** once summaries exist (`kVerbatimTail`), each ≤ **400 chars** (`kMaxMessageChars`) |
| Remember | static rules | — |
| Tools (opt-in) | `query_graph` / `query_mind` / `query_history` / `list_scenes` | bounded results |

Critical structural facts:

- **`scene.history` contains only player input + narrator prose.** NPC speech goes to the separate
  `scene.dialogue` buffer (`emit_dialogue`), which is **never** fed back into the prompt and **never**
  downsampled. Spoken motifs survive only if the narrator's own prose re-mentions them.
- **The scenario's `system_prompt` is loaded and saved but never injected** into the narrator call
  (`build_narrator_instructions()` is fixed boilerplate). **Superseded 2026-08-29:** the beat
  prompt now prepends a subordinated `Scene style` block from `scene.system_prompt`.
- Chroma (`server/chroma/`) is **write-only** in the live loop: graph facts are embedded and stored
  (`MemorySystem::store_node`), but `search_nodes` has no production caller. There is **no vector
  retrieval into any prompt** — the self-RAG hypothesis is false for this codebase.
- History window defaults: `window_size_=8`, `resume_window_size_=12`
  (`core/include/rhapsode/turn_executor.h`); the session never overrides them.

### 1.2 The "Story so far" is recursive self-summarization with age-based eviction

`process_text_downsampling` (`core/src/text_downsampling.cpp`) batches 3 history messages per LLM
summary snippet, cascading into mip levels with budgets **10 / 5 / 3 snippets**. There is no level
above 2 and no importance weighting: when a level overflows, the **oldest** content merges upward,
and above level 2 it is **evicted**. Total long-horizon memory in the automatic prompt: 18 snippets.

### 1.3 The world graph is authoritative but unpruned and dilution-prone

Narrator phase B (`GRAPH_UPDATE`) writes LLM-authored atomic facts into `WorldGraph` via
`Director::apply_planned_turn` (`core/src/director.cpp`). Read-back is opt-in via `query_graph`
(entity timeline trace, `core/src/world_analysis.cpp`). There is no relevance decay in practice —
see §2.3.

### 1.4 The "player" is endogenous

`experiments/session_pipeline/player_agent.py` + `core/src/eval/session_eval.cpp`:

- Player LLM = `deepseek-v4-flash`, `thinking=false`, **zero tools** (the tool loop was removed for
  latency — comment at `player_agent.py:196` — while `protocol.md` still instructs tool use).
- Context = last **12 WS messages** + previous action + Situation block + fixed guide.
- `guides/default.md` explicitly biases against entropy: *"Keep this a slice-of-life story …
  Avoid hard quests; life here is easy, with room for romance … let quiet scenes breathe."*

So the "external input" $u_t$ is a function of the last 12 messages of the same process plus
sampling noise: **the loop is closed**. The only structural entropy sources in the engine —
lifecycle fork/merge/conclude, off-stage scenes, expiry — were all neutralized in this run (§2.4).

### 1.5 Loop diagram

```text
                       ┌────────────────────────────────────────────────┐
                       │                                                │
 player LLM ──text──▶ history ──▶ narrator prompt ──▶ narrator prose ───┤
     ▲                 │  (player + narrator only;      │               │
     │                 │   NPC dialogue EXCLUDED)       ├─▶ dialogue ✂ (dropped from loop)
     │                 │                                │
     │                 └──▶ mip summarizer ──▶ "Story so far" (≤1500 chars,
     │                        (LLM, age-evicting,        18-snippet budget)
     │                         budgets 10/5/3)              │
     │                                                      ▼
     │                                            back into narrator prompt
     │                                                      │
     │                 graph LLM (phase B) ──▶ WorldGraph ──┴─(opt-in query_graph)
     │                                          │
     │                                          └──▶ Chroma (write-only, never read)
     │
     └──── last 12 messages of the same stream (no exogenous input anywhere)
```

---

## 2. Forensic evidence from the run artifacts

### 2.1 Vocabulary lock-in (transcript, `turns.jsonl`)

Motif counts per 50-turn block (regex over full turn records):

| Turns | "candle" | quest terms (slime/toad/fence/Gerhardt) |
|---|---|---|
| 1–50 | 21 | **55** |
| 51–100 | **175** | 7 |
| 101–150 | **199** | 12 |
| 151–200 | 120 | 2 |
| 201–250 | 102 | 10 |
| 251–300 | 114 | **0** |

Plot vocabulary decays to literally zero; the ritual motif saturates by block 2 and holds. The
narrator's simile tic ("the words land the way …") appears at a constant ~5–8 per block across the
whole run — a stylistic invariant of the locked state. The `repetition_score` (adjacent-message
token Jaccard) stays at 0.136 because paraphrase renews tokens while preserving the template:
**the metric measures the wrong thing** for this failure mode.

### 2.2 The summary bottleneck evicted the plot (final save, `saves/konosuba.json`)

The downsampler's terminal state: 18 snippets covering **history messages 507–596 only**.
Messages 1–506 — the quest negotiation, the signed slip, the debt terms, the covenant's founding —
have fallen off the top of the pyramid. Sample of what survived:

> "Toast-clink four-glass covenant, toast-verse names, flat-shield floorboard disarmament,
> staff-garnet third-eye watch, Luna toast-inclusion warmth, bitter-sweet wine, curse-or-blessing
> party joke, not-paying-to-find-out line."

Two independent observations:

1. **Content**: 18/18 snippets are emotional-gesture material; zero plot state (no quest, no debt
   figure, no schedule).
2. **Form**: the summary prose itself degenerated into hyphenated noun-piles — the summarizer's
   output format collapsed under its own iteration, a miniature of the top-level phenomenon.

### 2.3 The graph preserved the plot but diluted it below retrievability (`saves/world.json`)

- Graph grew **18 → 1,007 nodes** over 300 turns (~3.3/turn); **964 remain `active` (96%)**.
- Expiry effectively never ran: **5 expiry LLM hops in the entire run** (vs 2,288 weave hops,
  3,027 narrator hops — from `llm_profile.jsonl`).
- The plot facts are all still there and `active`: *"Slime-residue fence quest pays 20k eris"*,
  *"The party's guild tab could fund rebuilding both the east and west guild walls"*, *"Giant Toad
  mating season is approaching"*. The engine never forgot.
- But the narrator's **6,711 tool calls** query entity timelines, and by endgame each timeline is
  dominated ~50:1 by self-written micro-emotional facts (*"Aqua shows genuine quiet empathy, hand
  drifting near Darkness's gauntlet"*). Authoritative memory was preserved but drowned.

### 2.4 Every entropy mechanism was neutralized

| Mechanism | Status in this run | Cause |
|---|---|---|
| `conclude` | Attempted once (turn 16), **failed**, never retried | Hard-coded refusal: scene holds the Player AND `scenes_.size() <= 1` (`core/src/story.cpp`); no retry queue — a failed op is discarded |
| `fork` | Never proposed | Lifecycle LLM conditioned on covenant content; player guide discouraged quest content that would justify forks |
| Off-stage scenes | None (single scene all run) | Nothing to schedule; `PRIORITY THIS TURN` never fired |
| Expiry | 5 hops / 300 turns | Effectively inert (needs investigation — likely gating bug or starved trigger) |
| Player input | Endogenous | 12-message window of same stream + anti-entropy guide |
| Diegetic clock | Does not exist | `beat_clock_` is a counter, not a calendar; "Saturday" is unfalsifiable prose |

Also documented: turn 182's player line *"I don't have an article about this location or named
people in my archive"* is **meta-leakage** from the player LLM imitating a RAG agent it doesn't
have — `protocol.md` instructs tool use but the tool loop was removed. The weak regex filter
(`_is_bad_action`) let it through, and it drove the next narrator beat.

---

## 3. Formal framing: what kind of dynamical system is this?

### 3.1 Setup

Let $x_t = (W_t, S_t, G_t, M_t)$: the 6-message window, the 18-snippet summary pyramid, the
world graph, and character minds. All prompts, budgets, and temperatures are fixed, so the session
is a **time-homogeneous Markov chain** $x_{t+1} \sim P(\cdot \mid x_t)$. The player input is
$u_t = h(\text{last 12 messages}, \xi_t)$ — a function of state plus sampling noise — so the
closed loop is autonomous: **there is no exogenous term**.

### 3.2 Why "fixed-point theorem" is the wrong formalization

The Banach picture (deterministic contraction ⇒ unique fixed point, insensitive to initial
conditions) fails on two counts:

1. The map is a stochastic kernel, not a deterministic contraction; no metric is available under
   which contractivity could be proven.
2. Decisively, Banach predicts the **same** limit from any start. The reinforcement analysis below
   predicts **path dependence** — different runs lock onto different motif monopolies. This is
   empirically testable (§4) and is the discriminating experiment.

### 3.3 The correct picture: reinforced stochastic process → metastable lock-in

Define a motif's *share* as its representation across the three memory channels. Then:

- **Generation** probability of a motif is increasing in its share (that is what conditioning on
  context means).
- **Write-back** is automatic for the window, and *selected* for the summary and graph — but the
  selectors (summarizer salience, phase-B extraction) are LLMs conditioned on the same
  motif-saturated context, so selection probability is also increasing in share.
- Every channel has a **hard budget** (6 messages / 18 snippets / bounded query returns), so
  shares compete **zero-sum**.

Increasing returns + zero-sum budget competition is the structure of a **generalized Pólya urn /
replicator dynamics**. Under superlinear reinforcement such processes converge almost surely to a
**monopoly**, with the winner selected by early-trajectory fluctuations (lock-in). The observed
timeline matches: the candle-covenant cluster wins between turns ~50–100 (§2.1) and then holds a
quasi-stationary distribution for 200 turns whose residual variance is purely stylistic — new
similes over a fixed scene-template.

Sampling noise (temperature 1.0) does not rescue the system: once $S_t$ and $W_t$ are ~100%
covenant content, all high-probability continuations are covenant paraphrases. Noise in token
space stops translating into noise in motif space; the escape probability of the metastable set
decays as its motifs saturate the conditioning context.

### 3.4 What "goes to a low-dimensional space" rigorously means here

It is **architecturally forced, not emergent**. The long-run dynamics *must* factor through the
1500-char summary bottleneck, because it is the only unbounded-horizon memory in the automatic
prompt. The system was always going to live on a low-dimensional manifold; the open design
question was only **which projection the bottleneck computes**. This run demonstrates that an
LLM-controlled, salience-based projection with **age-based eviction** projects onto mood and
destroys plot — irreversibly, because evicted content never re-enters the prompt except via
keyword `query_history` or a graph lookup that dilution has already defeated.

So the informal claim "F^n(x) goes to a low-dimensional space like a fixed point" is right in
conclusion, wrong in mechanism: it is not convergence to a fixed point of the LLM map; it is
**lock-in of a self-reinforcing selection process inside a mandatory compression bottleneck, in
the absence of any exogenous input**.

### 3.5 Relation to known results

- **Model collapse / self-consuming loops** (Shumailov et al., *The Curse of Recursion*, 2023):
  distributions recursively conditioned on their own output lose tail mass and converge to a
  low-variance core. This run is the inference-time analogue, executed for 300 iterations.
- **Neural text degeneration** (Holtzman et al., 2019): likelihood-seeking decoding self-amplifies
  repetition; here amplified at the session scale through the memory system rather than the
  decoding loop.
- **Pólya-urn lock-in** (Arthur, increasing-returns economics; Pemantle's survey of reinforced
  processes): early random advantage + increasing returns ⇒ monopoly, path dependence.
- Internal precedent: [narrator-and-mind-weakpoints](narrator-and-mind-weakpoints.md) already
  observed "no dramaturgical spine" at 18 turns; this run shows what that becomes at 300.

---

## 4. Falsifiable predictions

1. **Path dependence (the discriminating experiment).** Rerun the identical config with different
   sampling seeds (and/or compare `debt_fishing_dinner` guide runs). Urn/lock-in predicts each run
   collapses onto a *different* motif monopoly; a fixed-point mechanism predicts the same one.
   Cheap, decisive about whether fixes should target the model/prompt (map $F$) or the feedback
   structure (memory budgets).
2. **Dimensionality drop.** Embed the 300 narrator beats (any sentence encoder); compute
   participation ratio / PCA spectrum over sliding windows. Prediction: sharp drop around turns
   50–100 and a plateau, while adjacent-Jaccard stays flat throughout — demonstrating the metric
   gap directly.
3. **Bottleneck intervention.** Exempt structured plot state (open objectives, debt figure,
   schedule commitments) from summary eviction and pin it in the prompt. Prediction: collapse is
   materially delayed with *no other change*, because pinning breaks the zero-sum budget
   competition.
4. **Entropy intervention.** Add a single scheduled exogenous event (e.g. a diegetic-clock-driven
   NPC arrival at turn 150). Prediction: temporary escape from the metastable set, with relapse
   unless the memory fixes also land — entropy and memory fixes are complements, not substitutes.

---

## 5. Recommendations, ranked by leverage

### P0 — the bottleneck itself

1. **Importance-based eviction in the downsampler.** Open threads, commitments, stakes, and
   scheduled events must be un-evictable (structured slots, not prose salience). Today eviction is
   purely by age (`text_downsampling.cpp`, budgets 10/5/3, no level above 2).
2. **Pin authoritative state in the prompt.** Debt figure, open quest objectives, diegetic date —
   as a structured block the narrator must respect, outside the summary's budget competition.

### P1 — restore the entropy channels

3. **Fix expiry.** 5 LLM hops in 300 turns means graph salience-decay is effectively dead; that is
   why 964/1,007 nodes are active and plot facts are diluted 50:1. Investigate the trigger path in
   `run_post_turn` (`turn_executor_post_turn.cpp`) — note the shared `try` block: one thrown
   `type_error` aborts monologues *and* downsampling for that beat (`post-turn failed: type must
   be string` appears in this run's console.log).
4. **Give lifecycle a real move-set.** `conclude` is structurally impossible with `scenes=1` +
   Player; failed ops are silently discarded with no retry. Either force periodic fork proposals,
   or add a scene-age pressure term that escalates until *some* lifecycle op succeeds.
5. **A diegetic clock.** `beat_clock_` is a counter; nothing ever falsifies "Saturday". A world
   calendar that hard-advances and fires scheduled events is the cheapest true entropy source.

### P2 — loop hygiene

6. **Feed dialogue back.** NPC speech is currently outside the recursion entirely (separate
   buffer, not summarized). Half the story's surface never constrains the future.
7. **Inject the scenario `system_prompt`** into the narrator call (currently dead config).
8. **Harness honesty.** Either restore the player tool loop or strip tool instructions from
   `protocol.md` (the mismatch caused the turn-182 meta-leak). Consider periodic objective
   rotation in the guide; the current default brief is actively anti-entropy.

### P3 — measure the right thing

9. **Add collapse-sensitive metrics** to `session_report.cpp`: embedding-window dispersion /
   participation ratio, motif-monopoly share (top-k n-gram concentration over a window), open-thread
   progression (did any `active` graph fact with `type=stake/objective` change state?). The current
   `repetition_score` (adjacent Jaccard, threshold 0.45) scored this run 0.136.

---

## 6. Experiment plan: locating the root cause

Four competing root-cause hypotheses:

| ID | Hypothesis | If true, the fix lives in |
|---|---|---|
| **H1** | Player brief steered into the attractor (anti-entropy guide) | harness config |
| **H2** | Summary bottleneck destroys plot state regardless of player | `text_downsampling.cpp` / prompt |
| **H3** | Absence of exogenous perturbations; system would self-correct if kicked | lifecycle / clock / events |
| **H4** | Narrator model's mode-seeking dominates; harness/memory fixes insufficient | model / decoding |

These are not mutually exclusive; the goal is to rank their causal contribution with the fewest
GPU-hours. Cost basis: ~260 s/turn ⇒ a 120-turn run ≈ 9 h; collapse onset in the reference run was
turns ~50–100, so 120 turns suffices as an endpoint.

### E0 — Build the measurement instrument (offline, no runs, do first)

No experiment is scorable without a collapse metric better than adjacent-Jaccard.

- Embed every narrator beat of existing runs (`default-guide-300`, `20260805-002555`; reuse the
  `bge-base-en-v1.5` infra in `server/rhapsode/memory.py`).
- Compute per sliding window (W=20 beats): **participation ratio** of the PCA spectrum
  $\mathrm{PR} = (\sum_i \lambda_i)^2 / \sum_i \lambda_i^2$, and **motif monopoly share** (share of
  beats within cosine ε of the window centroid; alternatively top-10 content-n-gram concentration).
- Define endpoints used by all experiments below:
  - **onset** $t^*$: first window where PR drops below 50% of its turns-1–30 baseline and stays
    there ≥ 3 windows;
  - **monopoly share** at t=120;
  - **plot pulse**: cumulative count of new graph facts matching open-stake topics per window
    (reconstructable offline — `world.json` nodes carry `created_at`).
- **Validation:** the instrument must show collapse at turns ~50–100 on `default-guide-300` (known
  positive) and ideally not on an early-run prefix (known negative).

**Validation outcome (2026-08-06, instrument built at
`experiments/session_pipeline/analysis/collapse_metrics.py`):** the raw `bge-base-en-v1.5`
embedding version **failed** — PR flat (11.73 early vs 11.78 late), monopoly share saturated
≥0.85 from turn 1, and the quest-era vs candle-era centroids sit at **cosine 0.986**: on this
corpus the sentence embeddings are dominated by the narrator's literary style, not content, so
the geometry cannot see the collapse that lexical counts show unambiguously. This is instrument
blindness, not evidence against collapse. Second iteration (TF-IDF + novelty + anchors),
validated numbers on the reference run:

- **Anchor-term rate** (word-boundary match; tight set `gerhardt,toad,fence,ewes,slime`):
  turns 1–50 **0.056** → turns 251–300 **0.005** — clean decay to an effectively-zero baseline.
  **This is the primary E1 relaxation metric.**
- **Era-centroid cosine (TF-IDF)**: quest era vs candle era **0.699** (vs bge's blind 0.986) —
  usable for post-hoc era comparison.
- **Windowed monopoly share / PR / novelty on TF-IDF: weak.** Monopoly is threshold-brittle
  (saturates at 0.25, flat at 0.35, near-zero at 0.45) because locked-state beats are lexically
  diverse paraphrases of a fixed theme; PR sits at the window ceiling; novelty barely moves
  (0.898 → 0.888). Lexical *tightness* is the wrong proxy for thematic collapse — surface
  variation survives lock-in (consistent with §2.1's constant simile-tic rate).

Anchor choice matters: generic terms (`claim`, `bounty`, `farm`) pick up mundane uses and
inflate the late baseline to ~0.08; probe anchors must be entity-specific.

**Third iteration — primary E1 metric (`analysis/relaxation.py`): baseline-return z-score.**
Anchor terms were rejected as primary (probe-specific, blind to paraphrase/coreference, not
transferable). The general formulation measures **return to the attractor's own distribution**
with no reference to probe content: TF-IDF centroid of the last N pre-injection beats, each
scored beat's cosine to that centroid z-scored against the baseline's self-similarity spread
(trailing 3-beat smoothing — validated as load-bearing), in-band iff z ≥ −2 (one-sided),
relaxation = first k=3 consecutive in-band beats. Validation on the reference run:

| baseline N | quest-era median z | quest-era %out-of-band | held-out %in-band |
|---|---|---|---|
| 40 | −4.87 | 100% | 90% |
| **60 (default)** | **−2.76** | **83%** | **100%** |
| 80 | −2.16 | 53% | 100% |

Operating range N=40–60; N=80 is knife-edge. Jensen–Shannon divergence kept as a secondary
check (corroborates in-band; under-detects out-of-band). Anchors remain as a diagnostic
overlay only — they answer "where did the story go," not "has it left the attractor."

### E1 — Perturbation–relaxation on the existing save (cheapest causal probe, ~2 h)

**Tests H3 vs H2 directly; reuses the turn-300 artifact.**

- **Implemented (2026-08-06)** as a 150-turn continuation with three probes
  (`experiments/e1_perturbation.toml`, injected via `injection.py` + `run.py --inject/--seed-saves`;
  scripted lines bypass the player LLM on those turns):
  - turn 1 `plot_probe` — Gerhardt bursts in: toad broke the east fence, six ewes lost;
  - turn 50 `commitment_probe` — party physically leaves for the farm;
  - turn 100 `style_control` — a novel bard NPC with no plot hook (separates "any novelty" from
    "plot novelty").
- Measure **relaxation time** τ per probe with the baseline-return z-score (E0 third iteration):
  first k=3 consecutive beats back inside the pre-injection attractor's band. Plus save
  forensics: does probe content reach the downsampler snippets / graph facts?
- Decision rule: **τ short (≲10 turns)** ⇒ the basin is deep; entropy injection alone cannot fix
  it; H2 dominates (memory pinning is the lever). **Perturbation sticks** (probe thread persists
  out-of-band, summary snippets pick it up) ⇒ H3 dominates; scheduled events + lifecycle repair
  are sufficient first-line fixes. If the style control sticks as well as the plot probes, the
  system amplifies any recency, weakening the H2-specific reading.

### E2 — Guide swap (tests H1, one 120-turn run, ~9 h)

- Identical config, replace `guides/default.md` with a plot-forward brief (*"pursue the signed
  fence/toad quest to completion; leave the guild hall by turn 10; prefer action over reflection"*).
- **Collapse persists** (onset $t^* \le 120$, monopoly share comparable) ⇒ H1 rejected as root
  cause; the memory system overrides even a cooperative player. **Collapse vanishes** ⇒ H1 is
  load-bearing and the engine is healthier than this post-mortem implies; re-rank fixes.
- Note the confound to avoid: don't simultaneously change player model or protocol.

### E3 — Replicate seeds (tests lock-in vs fixed point, 2 extra 120-turn runs)

- Two additional runs of the *original* config (API sampling at temperature 1.0 makes replicates
  distinct without a seed knob).
- Compare dominant motif clusters across the three runs (centroid cosine similarity).
- **Different monopolies per run** ⇒ path-dependent lock-in confirmed (reinforced-process picture);
  fixes should target feedback structure. **Same monopoly** ⇒ attractor is model/scenario-intrinsic
  (evidence toward H4); test narrator model/decoding changes sooner.

### E4 — Bottleneck pinning (tests H2 causally; small engine change + one run)

- Minimal intervention: append a fixed **"Open threads"** block to the narrator turn_state, built
  from graph facts of stake/objective topics (even a hard-coded 5-line block for the konosuba
  scenario is a valid first probe), exempt from all budgets. No other changes.
- **Onset delayed / plot pulse nonzero at t=120** ⇒ H2 confirmed causally; prioritize P0 fixes.
  No effect ⇒ the bottleneck is not (solely) where collapse is decided; weight shifts to H4.

### E5 — Model swap (tests H4; run only if E1–E4 leave it standing)

- Swap the player model family, then (separately) the narrator model; 120-turn runs.
- Only informative after the cheaper factors are excluded — expensive and confounded otherwise.

### Execution order and decision tree

```text
E0 (instrument, offline)
  └─▶ E1 (perturbation on existing save, ~2 h)
        ├─ τ short  ──▶ E4 (pinning) ──▶ confirms H2 ──▶ implement P0, re-run E1 to verify
        └─ sticks   ──▶ H3: implement clock/lifecycle (P1), then E3 for mechanism
  E2 (guide swap) and E3 (replicates) run overnight in parallel with the above —
  they need no code changes and settle H1 and path dependence independently.
```

Total budget for a decisive first round: E0 offline + E1 (~2 h) + E2/E3 (2–3 overnight runs).

---

## 7. Housekeeping found during the investigation

- `server/rhapsode/scheduler.py` and `server/rhapsode/config.py` are **0 bytes in the working
  tree** (HEAD has content) — looks like accidental truncation, unrelated to this analysis but
  will break sessions if committed.
- `experiments/session_pipeline/player_agent.py` module docstring claims tool use that was removed.
- `config.toml` default guide (`merge_fork_test.md`) disagrees with `autoplay.bat` default
  (`default.md`).

---

## Appendix A — run metrics

| Metric | Value |
|---|---|
| Turns | 300/300, `MaxTurns`, 0 errors, 0 timeouts |
| Turn latency | avg 260s, p50 247s, p95 396s |
| `empty_beats` | 5 (turns 35, 102, 130, 178, 180 — narrator `content=""`) |
| `repetition_score` | 0.136 (threshold 0.45 — not flagged) |
| `length_collapse` | 0.723 (flagged) |
| Graph | 18 → 1,007 nodes; 964 active |
| LLM hops (`llm_profile.jsonl`) | narrator 3,027 (6,711 tool calls) · monologue 1,190 · death 1,075 · scheduler 927 · lifecycle 474 (189 tool calls) · downsample 338 · player 300 (0 tool calls) · weave 2,288 · expiry **5** |
| Lifecycle ops | 1 conclude attempt (turn 16, failed, player-protected + sole scene) |

## Appendix B — narrative timeline (abridged)

```text
T1–7    Debt + slime-fence/toad quest negotiated and SIGNED ("dawn, fence line")
T8–22   Aqua bucket-trial / spa wager ("tomorrow" — never arrives)
T23–40  Luna bar romance (first drink in 8 years, hand-holding)
T41–161 "Covenant of the Guttering Candle": Articles, crane, season debt-wager
        T97 "next Thursday" → T102 retconned to "Saturdays"
        T98 retro-claims the never-played toad fight happened "yesterday"
T162–172 Exit #1 (guild steps) → return ("Alright, I'm back")
T175–203 Exit #2: frost bench outside coexists with candle table inside
        T182 player meta-leak ("no article in my archive")
T204–238 Luna joins the table (fifth seat)
T240–269 Card game → Aqua loses → bent-eris → joint-soak bargain
T270–300 "This table over the world"; debt still open; every promise deferred
```

The quest was never played; the debt was never resolved (explicitly not written off at T124; the
season wager's deadline "first frost" is beyond turn 300); five breakfasts, one spa day, one lake
date, four stanzas, and a weekly candle remain outstanding.

## See Also

- [[research/frontier-llm-long-horizon-orchestration]]
