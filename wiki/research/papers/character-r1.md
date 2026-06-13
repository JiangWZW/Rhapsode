---
title: "Character-R1: Enhancing Role-Aware Reasoning in Role-Playing Agents via RLVR"
arxiv: "2601.04611"
venue: arXiv 2026
authors: "Yihong Tang, Kehai Chen, Xuefeng Bai, Benyou Wang, Zeming Liu, Haifeng Wang, Min Zhang"
institutions: "Harbin Institute of Technology (Shenzhen), SLAI, CUHK Shenzhen, BUAA, Baidu Inc."
github: "https://github.com/Toyhom/Character-R1"
github_stars: null
pdf: "https://arxiv.org/pdf/2601.04611"
last_updated: 2026-05-19
quality: B
tags:
  - research
  - role-playing
  - reasoning
  - reinforcement-learning
  - verifiable-reward
  - GRPO
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[research/papers/rar-thinking-in-character]]"
  - "[[research/papers/her-dual-layer-thinking]]"
  - "[[architecture/companion-system]]"
---

# Character-R1: Role-Aware Reasoning via Verifiable Rewards

Tang et al., arXiv Jan 2026 | [arXiv](https://arxiv.org/abs/2601.04611) | [GitHub](https://github.com/Toyhom/Character-R1)

## Quality assessment

**Rating: B** (same first author as RAR, solid venue trajectory, but institutional backing is mid-tier)

- **Institutions**: Harbin Institute of Technology Shenzhen (same group as RAR), CUHK Shenzhen, BUAA, Baidu. HIT-SZ is productive in NLP but not a top-tier AI lab. Baidu adds industry credibility. CUHK-SZ (Benyou Wang) is a known NLP group.
- **Venue**: arXiv preprint (Jan 2026). Not yet peer-reviewed. The same first author (Yihong Tang) published RAR at NeurIPS 2025, so there's a track record.
- **Citations**: Too new.
- **Code**: Released on GitHub. Uses the EasyR1 framework, which is well-documented.
- **Relationship to RAR**: This is the same research group's follow-up to RAR. Where RAR used SFT + DPO (distillation), Character-R1 replaces DPO with RL (GRPO) and adds verifiable reward signals. It represents the group's evolution from supervised to reinforcement approaches.

## The core problem

RAR (NeurIPS 2025) showed that structured reasoning helps role-playing, but used contrastive learning (DPO on style preference pairs) as the training signal — an indirect, narrow signal. Other concurrent work (RAIDEN-R1, Echo-N1) tried RL with verifiable rewards, but their reward signals are also limited:

| Prior work | Reward type | Limitation |
|------------|------------|------------|
| RAR (Tang et al.) | DPO on style contrast | Indirect — contrasts reasoning styles, doesn't directly reward character cognition |
| RAIDEN-R1 (Wang et al.) | Keyword matching | Fragile — binary yes/no on keyword presence, easily gamed |
| Echo-N1 (Zhang et al.) | Sentiment classifier | Partial — only captures emotional dimension, misses knowledge/memory/worldview |

Character-R1 argues that effective RL for role-playing needs rewards that are **direct** (measure character cognition explicitly), **comprehensive** (cover multiple dimensions), and **flexible** (adapt to different character types).

## Method: Three reward designs + normalization

Character-R1 uses GRPO (Group Relative Policy Optimization, from DeepSeek-R1) with three complementary reward signals.

### Design 1: Cognitive Focus Reward

Forces the model to explicitly analyze the character before responding, using structured tags for 10 cognitive dimensions:

| Dimension | What it captures | Example |
|-----------|-----------------|---------|
| Knowledge | Static identity, background | "cheerful", "brave", career history |
| Style | Manner of expression | "gentle", "clumsy", "tsundere" |
| Worldview | Context and epistemic boundaries | "18th century Europe", what the character cannot know |
| Emotion | Current internal state | happy, angry, conflicted |
| Empathetic | Stance toward the user | sarcasm, contempt, helplessness |
| Engagement | Interaction design | Encourage user to continue |
| Human_Like | Naturalness | Reduce robotic/formal phrasing |
| Extension | Profile supplements | Specific career details, life events |
| Memory | Continuity and recall | Personal anecdotes, recent dialogue |
| Safety | Content moderation | Sensitive topic identification |

The model's reasoning trace must include `<focus>dimension</focus>` and `<focus_attr>description</focus_attr>` tags. Two sub-rewards measure quality:

- **Focus accuracy** (R_focus): BLEU-1 between the model's `focus_attr` descriptions and reference attributes — measures whether the model correctly identifies relevant character information.
- **Attribute accuracy** (R_attr): Exact match on which cognitive dimension the model selects — measures whether it's attending to the right aspect of the character for this turn.

### Design 2: Reference-Guided Reward

Uses overlap-based metrics (BLEU, ROUGE) between the model's response and a reference response as an optimization anchor. This provides a dense signal that helps RL exploration converge faster — the reference serves as a "good enough" target to approach, while the model is free to surpass it.

### Design 3: Character-Conditioned Reward Normalization

Different characters have very different reward distributions. A "cheerful, outgoing" character naturally scores higher on engagement metrics than a "stoic, withdrawn" character. Standard GRPO normalizes within a batch, but if batches mix character types, the model learns to avoid difficult characters.

Character-R1 groups characters using K-Means clustering on profile embeddings (7 clusters via text-embedding-3-large), then normalizes rewards **within each character group** and **within each reward type**:

```
r虃 = (r - 渭_group_type) / (蟽_group_type + 蔚)
```

This ensures that a stoic character's good response is rewarded as much as a cheerful character's good response, relative to their respective baselines.

### Combined reward

```
R_total = w_focus × R_focus + w_attr × R_attr + w_ref × R_ref
```

With weights: w_focus = 0.4, w_attr = 0.2, w_ref = 0.2. (Remaining 0.2 is reserved for format compliance.)

### Training setup

- **Base models**: LLaMA-3.2-3B-Instruct, Qwen2.5-7B-Instruct
- **RL algorithm**: GRPO
- **Framework**: EasyR1
- **Hardware**: 8x H20 GPUs
- **KL coefficient**: 尾 = 0.02
- **Learning rate**: 1e-6 with cosine schedule

## Experimental results

### CharacterBench results

13-dimensional evaluation (1— scale):

| Method | Avg | MC | FA | BC_K | AC^b | AC^h | BC^b | BC^h | ES | ER | MS | MR | HL | EG |
|--------|-----|----|----|------|------|------|------|------|----|----|----|----|----|----| 
| Vanilla SFT | 3.19 | — | — | — | — | — | — | — | — | — | — | — | — | — |
| RAR (NeurIPS 2025) | 3.69 | — | — | — | — | — | — | — | — | — | — | — | — | — |
| **Character-R1** | **3.81** | — | — | — | — | — | — | — | — | — | — | — | — | — |

Character-R1 outperforms RAR (3.81 vs 3.69) and all other baselines on CharacterBench average. The largest gains are on Knowledge-related dimensions (Fact Accuracy, Boundary Consistency) and Memory Consistency — exactly the dimensions where explicit cognitive focus tags force the model to attend to character-specific information.

### SocialBench results

| Method | Avg | Style | Knowledge | SU | ED | HSD | Memory | Neu | Pos | Neg |
|--------|-----|-------|-----------|----|----|-----|--------|-----|-----|-----|
| RAR | 65.4 | — | — | — | — | — | — | — | — | — |
| **Character-R1** | **67.2** | — | — | — | — | — | — | — | — | — |

Gains are concentrated on Role Knowledge and Memory dimensions.

### Ablation: reward components

| Configuration | CharacterBench Avg |
|---------------|-------------------|
| SFT baseline | 3.19 |
| + GRPO (no structured reward) | 3.45 |
| + Cognitive Focus only | 3.62 |
| + Reference Guidance | 3.71 |
| + Character-Conditioned Normalization | **3.81** |

Each component contributes. Cognitive Focus provides the largest single jump (+0.17 over unstructured GRPO). Character-Conditioned Normalization adds the final +0.10, which is significant for heterogeneous character sets.

## Practical analysis for Rhapsode

### What Character-R1 gives us

Character-R1's main contribution is the **10-dimension cognitive focus framework** — a structured vocabulary for what aspects of a character the model should attend to per turn. This maps naturally to Rhapsode's WorldGraph character nodes:

| Character-R1 dimension | Rhapsode equivalent |
|------------------------|---------------------|
| Knowledge | WorldGraph character attributes |
| Style | Companion system voice/tone config |
| Worldview | Narrative era + epistemic boundaries |
| Emotion | L1 activation steering state |
| Memory | L2 memory retrieval results |
| Engagement | Director-driven interaction goals |
| Safety | Content policy layer |

### Comparison with HER

Both papers train role-play reasoning via RL, but their philosophies differ:

| Dimension | Character-R1 | HER |
|-----------|-------------|-----|
| Thinking model | Single reasoning trace with structured tags | Dual-layer (system thinking + role thinking) |
| Reward model | Rule-based verifiable (BLEU, exact match) | Generative (trained Role-play GRM, 51 principles) |
| Reward calibration | Character-conditioned normalization | Balanced construction + anti-pattern-bias |
| Base model | LLaMA-3.2-3B, Qwen2.5-7B | Qwen3-32B |
| Scale of results | Small models (3B, 7B) | Large model (32B) |
| Benchmark | CharacterBench 3.81, SocialBench 67.2 | CoSER 53.1 (different scale) |

Character-R1's advantage: **works on small models** (3B, 7B), which directly matches Rhapsode's local-deployment constraint. Its rule-based rewards are cheaper to compute than HER's generative reward model.

HER's advantage: **deeper cognitive model** (dual-layer thinking), **more sophisticated reward** (trained GRM with scene-specific principles), and **stronger absolute results** on harder benchmarks.

### Rhapsode integration path

1. **Cognitive focus tags as prompt structure**: Even without RL training, the 10-dimension focus framework could be adopted as a structured prompt template for companion NPCs. Before generating a response, the model explicitly identifies which character dimensions are relevant.

2. **Character-Conditioned Normalization for multi-NPC training**: If Rhapsode trains LoRA adapters across multiple NPCs simultaneously, the per-character-group normalization prevents RL from favoring "easy" personality types.

3. **Combine with HER's dual-layer**: Use Character-R1's cognitive focus as the structure *within* HER's system thinking layer. The system thinking plans by explicitly walking through relevant character dimensions; the role thinking then generates the in-character response.

### Open questions

- **10 dimensions vs. fewer**: The full 10-dimension analysis adds overhead. For fast NPC dialogue, a subset (Knowledge + Emotion + Memory) might suffice.
- **Verifiable vs. generative rewards**: Character-R1 uses rule-based rewards (BLEU, exact match). These are cheaper but may miss nuance that HER's trained GRM captures. An empirical comparison on the same base model would be valuable.
- **Qwen compatibility**: Character-R1 tests on Qwen2.5-7B-Instruct, which directly validates it for Rhapsode's target model family.

## Citation

```bibtex
@article{tang2026character,
  title   = {Character-R1: Enhancing Role-Aware Reasoning in
             Role-Playing Agents via RLVR},
  author  = {Tang, Yihong and Chen, Kehai and Bai, Xuefeng and
             Wang, Benyou and Liu, Zeming and Wang, Haifeng and
             Zhang, Min},
  journal = {arXiv preprint arXiv:2601.04611},
  year    = {2026}
}
```
