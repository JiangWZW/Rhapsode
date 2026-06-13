---
title: Embedding Similarity Gate — Feasibility Report
date: 2026-05-31
tags: [research, embeddings, RAG, chain-topology, edge-creation]
---

# Embedding Similarity Gate: Theory, Experiment, and Recommendations

## 1. The Question

Should we use cosine similarity (from `BAAI/bge-base-en-v1.5`) as a hard filter
on chain edges in the WorldGraph? Specifically: if a new node shares an entity
with its chain predecessor but the embedding similarity falls below threshold
theta, should we refuse to create that edge?

---

## 2. Theory: Why Cosine Similarity Fails for Causal Links

### 2.1 What Embeddings Actually Measure

Bi-encoder models (BGE, MiniLM, Nomic, GTE) encode each sentence independently
via mean pooling. The resulting vector represents **topical content** — "what is
this sentence about?" Cosine similarity between two vectors measures **topic
overlap**, not logical, causal, or inferential relationships.

This is an architectural constraint, not a training deficiency:
- Mean pooling averages all token representations into one vector
- This erases word order, compositional structure, and negation
- Result: "A causes B" and "B causes A" produce nearly identical embeddings
- Result: "The bridge collapsed" and "Traffic was rerouted" score LOW despite
  being causally linked (they share no vocabulary)

### 2.2 Documented Failure Modes (2026 Literature)

From systematic evaluations across 5+ production embedding models:

| Failure Mode | Mean Cosine | Should Be | Source |
|---|---|---|---|
| Entity swap ("A hit B" vs "B hit A") | 0.987 | ~0.0 | clawRxiv 2604.00986 |
| Temporal inversion ("before" vs "after") | 0.953 | ~0.0 | clawRxiv 2604.00986 |
| Negation ("has X" vs "does not have X") | 0.896 | ~0.0 | clawRxiv 2604.01099 |
| Causal pairs (correct cause-effect) | 0.3—.5 | ~1.0 | arxiv 2504.04700 |
| Semantically similar but irrelevant | 0.7—.9 | ~0.0 | CausalRAG 2503.19878 |

Key insight: embeddings assign HIGH scores to pairs that are topically similar
but logically unrelated, and LOW scores to pairs that are causally linked but
lexically different.

### 2.3 The "Semantic Drift" Problem (CausalRAG)

From the CausalRAG paper (arxiv 2503.19878):
> "Given 'An explosion of sulfides occurred in the factory', DPR selects
> 'a production facility caught fire' (sim=0.82, causally irrelevant) over
> 'workers were injured by eye irritation' (sim=0.41, causally correct)."

44% of retrieval failures in causal tasks come from this drift: the retriever
finds what SOUNDS related rather than what IS related.

### 2.4 Root Causes (Architecture)

1. **Mean pooling erasure** — averaging all tokens destroys positional and
   compositional information (Reimers & Gurevych 2019)
2. **Symmetric metric** — cosine is symmetric; causation is directional
3. **Contrastive training bias** — models trained on paraphrase/NLI data learn
   "same topic = similar" but never learn "causes/enables/prevents = similar"
4. **Anisotropy** — embedding spaces cluster in narrow cones (Ethayarajh 2019),
   making most pairs score 0.4—.7 regardless of actual relatedness

### 2.5 What Production RAG Systems Do

The industry consensus (2026):

| Stage | Method | Purpose |
|---|---|---|
| Initial retrieval | Hybrid: dense vectors + BM25 (keyword) | Cast wide net |
| Reranking | Cross-encoder (full cross-attention) | Resolve compositional failures |
| Causal reasoning | Graph structure or LLM-as-judge | Detect inferential links |

Key takeaway: **no production system relies on cosine similarity alone for
relevance.** It's always combined with keyword matching and reranking.

---

## 3. Experiment: Chain Edge Similarity Analysis

### 3.1 Setup

- Model: `BAAI/bge-base-en-v1.5` (production, 768-dim, cosine space)
- Data: 28 nodes from Siege of Ashenmoor scenario
- Chain topology: 43 edges (one per entity per step)
- Question: how many chain edges would be KILLED at various thresholds?

### 3.2 Results: Threshold Sweep

| Threshold | Edges Survive | Killed | Kill Rate |
|---|---|---|---|
| 0.30 | 43/43 | 0 | 0% |
| 0.35 | 43/43 | 0 | 0% |
| 0.40 | 42/43 | 1 | 2% |
| **0.45** | **39/43** | **4** | **9%** |
| 0.50 | 32/43 | 11 | 26% |
| 0.55 | 27/43 | 16 | 37% |
| 0.60 | 17/43 | 26 | 60% |

### 3.3 Distribution Statistics

```
Mean similarity:   0.572
Median:            0.582
Std dev:           0.091
Min:               0.363
Max:               0.744
```

### 3.4 Per-Entity Chain Quality

| Entity | Links | Mean Sim | Min | Max |
|---|---|---|---|---|
| **ashenmoor** | 13 | **0.474** | **0.363** | 0.582 |
| duke | 2 | 0.523 | 0.496 | 0.551 |
| father aldric | 5 | 0.591 | 0.493 | 0.699 |
| thornfield | 2 | 0.600 | 0.541 | 0.659 |
| lord harren | 6 | 0.600 | 0.567 | 0.635 |
| player | 5 | 0.615 | 0.544 | 0.740 |
| warden elara voss | 7 | 0.643 | 0.574 | 0.700 |
| sergeant maren | 3 | 0.684 | 0.618 | 0.744 |

### 3.5 Edges Killed at theta=0.45

All 4 killed edges are **"Ashenmoor" location links** between unrelated storylines:

| Edge | Sim | From | To | Assessment |
|---|---|---|---|---|
| 10->11 | 0.363 | "Aldric visits catacombs" | "Duke expects Ashenmoor to fall" | Different storylines |
| 11->12 | 0.402 | "Duke expects Ashenmoor to fall" | "Voss ordered water rationing" | Weakly related (both about doom) |
| 12->13 | 0.420 | "Voss ordered water rationing" | "Tunnel beneath east wall passable" | Unrelated events |
| 19->21 | 0.412 | "Three soldiers deserted" | "Harren's vanguard at tree line" | Potentially causal |

### 3.6 Reachability Impact

**Despite killing 4 edges, the graph remains a single connected component.**
No nodes become isolated. This is because every killed edge connects nodes that
share "Ashenmoor" but also have OTHER entity links keeping them connected via
different paths (through Voss, Harren, Aldric chains).

### 3.7 Interpretation

The "Ashenmoor" entity is the **location tag** — everything happens in Ashenmoor.
It's the equivalent of tagging every event with "Earth." Chain links via this
entity are inherently weak because consecutive "Ashenmoor" facts are often about
completely different topics (catacombs, rationing, tunnels, war drums).

However: edge [19->21] "soldiers deserted" -> "Harren's vanguard arrives" is
arguably important (desertion + approaching army = compounding crisis). Killing
it at 0.412 is a genuine loss.

---

## 4. Critical Finding: The Asymmetric Risk

The experiment reveals an **asymmetric risk profile**:

- **Character entities** (Maren, Voss, Aldric) produce strong chain links
  (mean >0.59) because consecutive facts about a person are usually topically
  related. The gate never kills these.

- **Location/setting entities** (Ashenmoor) produce weak chain links (mean 0.47)
  because a location is shared by unrelated events. The gate primarily kills
  these — and some of them ARE important.

The gate's kills are concentrated on the ONE entity type where causal inference
matters most (events at the same location can compound into crisis).

---

## 5. Solutions: Ranking by Viability

### Solution A: No Similarity Gate (pure chain, no filtering)

**Keep all 43 edges.** The chain already reduces edges from 163 to 43 (74%
reduction). The remaining "weak" Ashenmoor links are a minor cost:
- They don't make the graph unreadable
- They provide reachability safety
- The Weaver can later reweight them

| Pro | Con |
|---|---|
| Zero risk of missing links | ~10 unnecessary edges |
| Simplest implementation | Slightly more noise for Weaver |
| No embedding dependency in Director | |

**Recommendation: This is the safest MVP.**

### Solution B: Similarity as Edge WEIGHT, Not Gate

Instead of killing edges below threshold, set their weight proportionally:
```
edge.weight = max(0.1, similarity)
```

All chain edges are created, but weak ones get low weight. The Weaver and
retrieval queries can then prioritize high-weight edges without losing
reachability.

| Pro | Con |
|---|---|
| Preserves all reachability | Requires embed callback in Director |
| Gives Weaver useful signal | Adds latency (~20ms per new node) |
| Gradual, not binary | Weight semantics need definition |

**Recommendation: Elegant middle ground. Worth implementing.**

### Solution C: Soft Gate with Location-Entity Exception

Apply theta=0.45 gate ONLY to non-location entities. Location entities (those
appearing in >N nodes, or explicitly tagged as locations in the scenario) always
get their chain link regardless of similarity.

| Pro | Con |
|---|---|
| Kills genuinely spurious links | Requires entity classification |
| Preserves location reachability | Added complexity |
| Adapts to entity frequency | Threshold still somewhat arbitrary |

**Recommendation: Over-engineered. The chain is already sparse enough.**

### Solution D: Let the Weaver Handle It (Deferred Quality)

Pure chain (Solution A) plus let the Weaver's existing disconnect/reweight ops
clean up weak edges during its background pass. The Weaver already sees the
graph and can reason about which edges are meaningful:
- "Aldric visits catacombs" -> "Duke expects Ashenmoor to fall" — Weaver sees
  these are unrelated threads, disconnects or lowers weight.
- "Three soldiers deserted" -> "Harren's vanguard arrives" — Weaver sees the
  compounding crisis, raises weight.

| Pro | Con |
|---|---|
| LLM reasoning handles causal links | Delayed quality (1-2 turns) |
| Embeddings never touch the hot path | Weaver prompt slightly larger |
| Architecturally clean | |

**Recommendation: This IS the Two-Phase (Method 8) design we already agreed on.**

### Solution E: Hybrid — Chain + Weight from Similarity + Weaver Enrichment

Three layers:
1. Chain creates all edges (Solution A — guaranteed reachability)
2. Edge weight initialized from cosine similarity (Solution B — immediate signal)
3. Weaver asynchronously upgrades/downgrades weights (Solution D — LLM refinement)

This gives the system the best signal at every stage:
- At creation: weight=similarity provides immediate quality ranking
- After 1-2 turns: Weaver overwrites weights with LLM-judged relevance
- No edges are ever killed; weak ones just have low weight

---

## 6. Final Recommendation

**Drop the hard similarity gate from the implementation plan.**

The evidence is clear:
1. Embeddings are architecturally blind to causal links (the literature)
2. The gate kills exactly the links where causal inference matters most (our data)
3. The chain topology already achieves 74% edge reduction without any filtering
4. The Weaver (Phase 2) handles quality with LLM reasoning, not cosine distance

**Revised plan:**
- Phase 1 (Director): Pure chain topology. All chain edges created at weight=1.0.
  No embed callback needed. Simple, fast, zero-risk.
- Phase 1.5 (optional, low priority): Initialize edge weight from cosine sim.
  Gives the Weaver better starting signal but not critical.
- Phase 2 (Weaver): Background enrichment pass. Adds cross-chain causal links.
  Reweights or disconnects weak chain links. Already designed.

This makes the Director change SIMPLER (no embed callback, no threshold, no
similarity computation) while being SAFER (no killed edges, no missed causal
links).

---

## 7. Summary Table

| Approach | Edges | Risk | Complexity | Latency |
|---|---|---|---|---|
| Current (clique) | 163 | None (all linked) | O(n^2) | 0ms |
| Chain + hard gate (original plan) | ~39 | Missing causal links | Medium | +20ms |
| **Chain, no gate (revised plan)** | **43** | **None** | **Low** | **0ms** |
| Chain + soft weight | 43 | None | Medium | +20ms |
| Chain + Weaver (full Method 8) | 43 + Weaver adds | None | Full system | Background |
