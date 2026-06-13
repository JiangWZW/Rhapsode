---
title: Edge Density Paper Ranking
date: 2026-05-28
tags: [research, graph-sparsification, temporal-knowledge-graphs, narrative-AI, agent-memory]
---

# Edge Density Research — Paper Analysis and Ranking

Structured evaluation of 15 references from the WorldGraph edge density investigation. Each paper is scored on Reliability (venue/citations/affiliation), Code Availability (repo/stars/runnability), and Practicality for Rhapsode (applicability to the `shares_entity` clique problem in a C++17 narrative engine with local Qwen3-8B LLM).

**Weighted composite**: Reliability 30% + Code 20% + Practicality 50%

---

## Ranked Table

| Rank | Paper | Reliability | Code | Practicality | **Composite** |
|------|-------|:-----------:|:----:|:------------:|:-------------:|
| 1 | Generative Agents (Park et al., UIST 2023) | 5 | 5 | 3 | **4.00** |
| 2 | Cortex (MikeSquared-Agency, 2026) | 1 | 4 | 5 | **3.60** |
| 3 | HALO (Ding et al., WWW Companion 2025) | 3 | 3 | 4 | **3.50** |
| 4 | Fog of War Pruning (Senanayake & Ware, AIIDE 2025) | 4 | 3 | 3 | **3.30** |
| 5 | Neighborhood-Preserving Sparsification (Kiouche et al., VLDB 2025) | 5 | 2 | 2 | **2.90** |
| 6t | Multi-Agent Narrative as Graph Pruning (Ware et al., AIIDE 2019) | 4 | 3 | 2 | **2.80** |
| 6t | KEAT (Waghmare et al., AAAI 2026) | 4 | 3 | 2 | **2.80** |
| 6t | Causal Graph from Narrative (Li et al., WNU 2025) | 3 | 2 | 3 | **2.80** |
| 9 | Narrative Trails / MaxST (German et al., CEUR 2025) | 2 | 3 | 3 | **2.70** |
| 10 | "Not All Memories Age the Same" (Karhade, arXiv 2026) | 1 | 1 | 4 | **2.50** |
| 11 | Spectral Sparsification (Spielman-Teng, CACM 2013) | 5 | 2 | 1 | **2.40** |
| 12t | AdaTKG (Lee et al., arXiv 2026) | 2 | 3 | 2 | **2.20** |
| 12t | Fast KNN Graph Construction (Chen et al., JMLR 2009) | 5 | 1 | 1 | **2.20** |
| 14 | Narrative Studio / MCTS (WNU 2025) | 3 | 1 | 2 | **2.10** |
| 15 | Degree-Preserving Sparsification (Chu et al., FOCS 2018) | 5 | 0 | 0 | **1.50** |

---

## Detailed Analysis

### #1 — Generative Agents (Composite: 4.00)

**Park, O'Brien, Cai, Morris, Liang, Bernstein — Stanford + Google — UIST 2023**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 5 | UIST (top HCI venue), ~4,800 citations, Stanford/Google CRFM |
| Code | 5 | [github.com/joonspk-research/generative_agents](https://github.com/joonspk-research/generative_agents) — 21,375 stars, 3,010 forks, Python, Apache 2.0 |
| Practicality | 3 | Implicit forgetting via retrieval scoring (recency × importance × relevance) avoids explicit pruning; but uses flat memory stream, not a graph — doesn't address edge density directly |

**Rationale**: The highest-signal paper in our list by every academic metric. Its memory architecture (observe-reflect-plan with three-factor retrieval) is already partially implemented in Rhapsode's `MemorySystem`. The key extractable insight is that **implicit forgetting via scoring is an alternative to explicit edge deletion** — if edges are weighted by recency/importance, the dense core naturally becomes invisible without removal. However, it doesn't solve the entity-clique generation problem at source.

---

### #2 — Cortex (Composite: 3.60)

**MikeSquared-Agency — Open-source project — 2026**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 1 | No peer review, indie developers, no paper, 8 stars |
| Code | 4 | [github.com/MikeSquared-Agency/cortex](https://github.com/MikeSquared-Agency/cortex) — Rust, MIT, well-documented, 3 releases, actively maintained |
| Practicality | 5 | Directly solves the same problem: entity-based auto-linking with temporal decay, contradiction detection, edge weight maintenance, and configurable briefing synthesis. Single embedded binary (Rust + redb + HNSW) |

**Rationale**: Despite zero academic credibility, Cortex is the most practically relevant reference. Its architecture maps 1:1 to Rhapsode's needs:
- **Auto-linker** (background task discovering relationships via embedding similarity + shared entities + temporal proximity) ≈Rhapsode's `shares_entity()` heuristic, but with configurable rules
- **Temporal decay** (edges lose weight over time, pruned below threshold) ≈what Rhapsode needs but doesn't have
- **Contradiction detection** (conflicting facts surfaced in briefings) ≈Rhapsode's Validator
- **Briefing engine** (structured context documents per agent) ≈Rhapsode's `focus_payload_json()`

The main extractable technique: **configurable edge decay with threshold-based pruning** — edges created by the auto-linker lose weight unless reinforced by new observations, and are garbage-collected when weight drops below a threshold.

---

### #3 — HALO: Half Life-Based Fact Filtering (Composite: 3.50)

**Ding, Wang, Gao, Yu, Ren, Xia — WWW Companion 2025**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 3 | WWW Companion (peer-reviewed, emerging track), Feng Xia is IEEE Fellow, 0 citations (published April 2025) |
| Code | 3 | [github.com/yushuowiki/K-Half](https://github.com/yushuowiki/K-Half) — 0 stars, 2 forks, Python, runnable |
| Practicality | 4 | Half-life theory for fact temporal validity is directly applicable to Rhapsode's supersession timing; three-module design (temporal attention → relation-aware encoder → filtering) maps to Weaver's resolve pipeline |

**Rationale**: The half-life metaphor is powerful for narrative facts: some facts (character traits) have long half-lives, others (current location, mood) have short ones. HALO's contribution is quantifying this via a learned decay function per relation type. For Rhapsode, the extractable technique is: **assign per-edge-type half-lives** (e.g., `is_at` edges decay in 2 turns, `is_ally_of` edges decay in 50 turns), and filter edges whose temporal validity has expired.

---

### #4 — Fog of War Pruning (Composite: 3.30)

**Senanayake & Ware — AAAI AIIDE 2025 (Best Paper)**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 4 | AAAI AIIDE (good specialized venue), Best Paper Award, U. Kentucky Narrative Intelligence Lab |
| Code | 3 | Implemented in Sabre planner (Java, open source at cs.uky.edu), ~1000 LOC |
| Practicality | 3 | Principle of restricting graph operations to protagonist-discovered knowledge is applicable as a retrieval filter; doesn't directly solve edge density but limits the working set |

**Rationale**: Fog of War restricts the planning state space to what the protagonist has discovered — actions involving unknown entities are pruned entirely. For Rhapsode, this maps to: **only create `shares_entity` edges when the entity has been narratively discovered by the active scene participants**. This wouldn't reduce existing edges but would prevent future over-connection by gating edge creation on narrative visibility.

---

### #5 — Neighborhood-Preserving Graph Sparsification (Composite: 2.90)

**Kiouche, Baste, Haddad, Seba, Bonifati — VLDB 2025**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 5 | VLDB (top-tier database venue), strong French academic affiliations (Lyon), ~3 citations (very new) |
| Code | 2 | No public repo found; algorithm described in paper with pseudocode |
| Practicality | 2 | Achieves 40% sparsification while preserving neighborhood structure — theoretically applicable, but designed for graphs with millions of nodes; the neighborhood-preservation guarantee is for classification/reachability, not narrative coherence |

**Rationale**: A well-published method at a top venue, but solves a different problem at a different scale. Rhapsode's 28-node graph doesn't need formal neighborhood-preservation guarantees — it needs *semantic* edge management (which edges represent meaningful narrative connections). The technique of controlling information loss via a user-defined tolerance parameter is a useful abstraction, but the actual algorithm (iterative edge removal with neighborhood distance bounds) is too generic.

---

### #6t — Multi-Agent Narrative as Graph Pruning (Composite: 2.80)

**Ware, Garcia, Shirvani, Farrell — AAAI AIIDE 2019 / IEEE Trans. Games 2022**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 4 | AAAI AIIDE + IEEE journal version, ~40 citations combined, established narrative AI lab |
| Code | 3 | Story Graphs dataset (77GB), Sabre planner; conceptual algorithm |
| Practicality | 2 | Prunes NPC *action possibilities* in a state-space graph, not facts in an entity graph; the framing of "intelligent pruning preserving story properties" is inspirational but not directly transferable |

**Rationale**: The foundational insight — prune edges from a story graph such that desirable narrative properties are preserved — is the correct *framing* for Rhapsode's problem. But the *mechanism* (removing NPC actions until each has at most one per state, preserving finishability) operates on a fundamentally different data structure (state-transition graph vs. entity-fact graph).

---

### #6t — KEAT: Kernelized Edge Attention (Composite: 2.80)

**Waghmare et al. — AAAI 2026 (Mastercard AI + IIT Delhi)**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 4 | AAAI 2026 (main track), Mastercard AI + IIT Delhi, 0 citations (published Jan 2026) |
| Code | 3 | [github.com/waghmaregovind/KEAT-TemporalGNN](https://github.com/waghmaregovind/KEAT-TemporalGNN) — 3 stars, Python, built on TGB |
| Practicality | 2 | Temporal kernels (Laplacian/RBF/MLP) for edge attention are interesting but require a trained GNN; operates on graphs with millions of interactions (Wikipedia edits); not applicable to 28-node graph without training data |

**Rationale**: The idea that **edges should be weighted by temporal kernels** (recent interactions get higher attention than old ones) is valid for Rhapsode. The specific implementation (kernel functions modulating edge features in attention computation) requires too much infrastructure (GNN training, large interaction datasets). The extractable principle: apply a continuous-time decay kernel to edge weights, with different kernel shapes for different relation types.

---

### #6t — Causal Graph from Narrative (Composite: 2.80)

**Li, Pan, Pi — ACL WNU Workshop 2025**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 3 | ACL WNU Workshop (peer-reviewed), NAACL 2025 collocated, 0 citations |
| Code | 2 | Claimed open-source, GitHub repo not found |
| Practicality | 3 | STAC taxonomy (Situation-Task-Action-Consequence) for meaningful edge types could replace `shares_entity` with causal relationships; requires RoBERTa-level NLP at runtime |

**Rationale**: The key insight is that **not all entity co-occurrences are edges** — only *causal* relationships should be connected. If Rhapsode's Weaver classified edges as Situation/Task/Action/Consequence rather than just "shares entity," the graph would naturally be sparser (fewer edges pass the causality threshold). The 7-feature linguistic Expert Index is too expensive for runtime, but the STAC schema itself is a cheap heuristic the LLM could apply during edge creation.

---

### #9 — Narrative Trails / MaxST (Composite: 2.70)

**German, Keith, North — CEUR Text2Story@ECIR 2025**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 2 | CEUR Workshop (low-tier), Virginia Tech, 1 citation |
| Code | 3 | [github.com/faustogerman/narrative-trails](https://github.com/faustogerman/narrative-trails) — 10 stars, Jupyter/Python, MIT |
| Practicality | 3 | MaxST sparsification (keep only strongest n-1 edges) is a trivially implementable heuristic; could be applied to Rhapsode's entity graph as a one-pass pruning operation |

**Rationale**: The simplest extractable technique in our list: **build a maximum spanning tree to reduce a dense graph to its strongest skeleton**. For Rhapsode: compute edge weights (based on recency, usage frequency, narrative importance), build a MaxST, and prune all edges not in the tree. This gives O(n) edges guaranteed. The weakness: a tree topology loses all redundancy (removing one edge disconnects the graph), so in practice you'd keep the MaxST plus some additional high-weight edges.

---

### #10 — "Not All Memories Age the Same" (Composite: 2.50)

**Karhade — arXiv preprint, April 2026**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 1 | arXiv preprint, single author, unknown affiliation ("Citingale"), no peer review, 2 citations |
| Code | 1 | No code published; algorithm requires survival analysis on observed value lifetimes |
| Practicality | 4 | The velocity/volatility decomposition and Lindy effect finding are directly usable for Rhapsode's per-entity decay; "uniform decay is 18× worse than no decay" validates entity-type-aware supersession |

**Rationale**: Low academic credibility but the conceptual contribution is strong. The paper's core claim — that different knowledge types have different temporal dynamics, decomposable into velocity (observation frequency) and volatility (value change rate) — maps directly to Rhapsode's entities. A character's name has zero volatility (permanent), their location has high volatility (transient). The Weibull survival model could inform per-entity-type half-lives without the full survival analysis framework.

---

### #11 — Spectral Sparsification (Composite: 2.40)

**Batson, Spielman, Srivastava, Teng — CACM 2013**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 5 | CACM (top venue), Yale/MIT/Microsoft Research, 124 citations (CACM), underlying work 1000+ |
| Code | 2 | No single official repo; many implementations exist in various linear algebra libraries |
| Practicality | 1 | Preserves Laplacian spectrum — completely irrelevant to narrative graphs; designed for numerical linear algebra applications |

**Rationale**: Foundational theoretical work, but solves a different problem. Spectral sparsification guarantees that the sparsified graph preserves algebraic properties (eigenvalues, effective resistances) — properties that have no meaning in a narrative entity graph. A 28-node graph doesn't need spectral approximation guarantees; it needs semantic awareness.

---

### #12t — AdaTKG (Composite: 2.20)

**Lee et al. — LG AI Research — arXiv May 2026**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 2 | arXiv preprint, 0 citations, but LG AI Research is credible institution |
| Code | 3 | [github.com/seunghan96/AdaTKG](https://github.com/seunghan96/AdaTKG) — new, Python |
| Practicality | 2 | Per-entity adaptive memory via learnable EMA is conceptually relevant; requires supervised training on TKG benchmarks — impractical for runtime narrative with 28 nodes |

**Rationale**: The EMA-based entity memory update (`new_memory = 伪 × old_memory + (1-伪) × new_observation`) is a clean formulation, but requires training on large temporal knowledge graph benchmarks (ICEWS, YAGO). Not applicable to Rhapsode without a training loop.

---

### #12t — Fast KNN Graph Construction (Composite: 2.20)

**Chen, Fang, Saad — JMLR 2009**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 5 | JMLR (top ML journal), U. Minnesota + Argonne National Lab, ~300 citations |
| Code | 1 | No official repo; algorithm well-described but requires Lanczos + spectral bisection implementation |
| Practicality | 1 | Solves graph *construction* from point clouds, not graph maintenance or pruning; would require completely rethinking Rhapsode's architecture |

**Rationale**: A well-cited foundational method for building KNN graphs efficiently. Irrelevant to Rhapsode's problem: we don't need to construct a graph from embedding vectors — we already have a semantically-constructed graph that's too dense. The technique would only be relevant if we abandoned `shares_entity()` and rebuilt the graph from scratch using embedding similarity.

---

### #14 — Narrative Studio / MCTS (Composite: 2.10)

**WNU 2025 (arXiv:2504.02426)**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 3 | ACL WNU Workshop 2025, peer-reviewed, 0 citations |
| Code | 1 | No public repository found |
| Practicality | 2 | Entity graph for narrative grounding is relevant; MCTS for exploration ≈pruning; no edge management mechanism |

**Rationale**: The entity graph construction (actors, environments, relationships) is architecturally similar to Rhapsode's WorldGraph, and the idea of referencing it during generation is exactly what Rhapsode does. But the paper offers no mechanism for managing graph density — it builds the graph once and references it statically.

---

### #15 — Degree-Preserving Sparsification (Composite: 1.50)

**Chu, Gao, Peng, Sachdeva, Sawlani, Wang — FOCS 2018 / SIAM J. Computing 2019**

| Metric | Score | Evidence |
|--------|-------|----------|
| Reliability | 5 | FOCS (top-tier theory venue), strong institutional affiliations, ~50 citations |
| Code | 0 | No code; algorithm requires implementing short cycle decompositions |
| Practicality | 0 | Pure combinatorial theory; preserves vertex degrees and Laplacian spectrum via correlated edge sampling on cycle decompositions; no semantic awareness; requires mathematical infrastructure irrelevant to a 28-node narrative graph |

**Rationale**: Impressive mathematics, zero practical relevance. The algorithm decomposes a graph into short cycles and samples edges in a correlated manner to preserve degrees. This is designed for preprocessing massive graphs (millions of nodes) before running Laplacian solvers. It has nothing to offer a small, semantically-rich narrative entity graph.

---

## Key Takeaways for Rhapsode

The papers cluster into four tiers of practical value:

### Tier 1 — Directly Applicable (implement now)

1. **Cortex** (#2): Reference architecture for the entire WorldGraph maintenance problem. Study its auto-linker rules, decay mechanism, and contradiction detection.
2. **HALO** (#3): Per-relation-type half-lives for fact temporal validity. Implement as a simple lookup: `half_life[edge_type] → turns_until_decay`.
3. **Generative Agents** (#1): Implicit forgetting via retrieval scoring. Already partially implemented — complete the recency × importance × relevance weighting.

### Tier 2 — Useful Principles (design-phase influence)

4. **"Not All Memories Age the Same"** (#10): Velocity/volatility decomposition. Don't treat all entities equally — high-volatility facts (location, mood) should decay faster than low-volatility facts (identity, allegiance).
5. **Fog of War** (#4): Gate edge creation on narrative visibility — only connect entities that have been "discovered" by active participants.
6. **Causal Graph from Narrative** (#6t): Replace `shares_entity` with STAC-classified edges — only create edges for causal/consequential relationships, not mere co-occurrence.
7. **Narrative Trails** (#9): MaxST as a one-pass pruning fallback — when the graph exceeds a density threshold, compute a maximum spanning tree and prune everything else.

### Tier 3 — Background Knowledge (inform mental model)

8. **KEAT** (#6t): Temporal kernels for edge attention — useful principle, impractical infrastructure.
9. **Multi-Agent Narrative Pruning** (#6t): Framing of "pruning preserving narrative properties" — correct problem statement.
10. **NP-Sparsification** (#5): Formal sparsification with controllable quality loss — overpowered for our scale.

### Tier 4 — Not Applicable

11—5. Pure graph theory papers (Spectral, Degree-Preserving), graph construction (KNN), neural TKG methods requiring training (AdaTKG), and tools solving adjacent problems (Narrative Studio).

---

## Recommended Implementation Order

Based on this analysis, the minimum viable edge density solution for Rhapsode combines:

1. **Gated edge creation** (from Fog of War + Causal Graph): Replace the blanket `shares_entity()` heuristic with a stricter criterion requiring either causal connection or narrative visibility.
2. **Per-type temporal decay** (from HALO + Karhade): Assign edge half-lives by relation type; edges past their half-life lose weight geometrically.
3. **Threshold-based pruning** (from Cortex): Edges below a weight threshold are garbage-collected during the existing `drain_resolve_queue()` background loop.
4. **Retrieval scoring** (from Generative Agents): Even if dense edges remain, weight them by recency × importance so stale connections are invisible during retrieval.
