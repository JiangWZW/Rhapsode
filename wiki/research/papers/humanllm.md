---
title: "HumanLLM: Benchmarking and Reinforcing LLM Anthropomorphism via Human Cognitive Patterns"
arxiv: "2601.10198"
venue: ACL 2026 Main
authors: "Xintao Wang, Jian Yang, Weiyuan Li, Rui Xie, Jen-tse Huang, Jun Gao, Shuai Huang, Yueping Kang, Liyuan Gou, Hongwei Feng, Yanghua Xiao"
institutions: "Fudan University, Hello Group, Johns Hopkins University"
github: null
pdf: "https://arxiv.org/pdf/2601.10198"
last_updated: 2026-05-19
quality: A
tags:
  - research
  - role-playing
  - cognitive-modeling
  - psychology
  - anthropomorphism
  - evaluation
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[research/papers/her-dual-layer-thinking]]"
  - "[[research/papers/coser]]"
  - "[[architecture/companion-system]]"
---

# HumanLLM: Cognitive Patterns for Authentic Character Simulation

Wang, Yang et al., ACL 2026 Main | [arXiv](https://arxiv.org/abs/2601.10198) | [OpenReview](https://openreview.net/forum?id=5J8y7gTrCs)

## Quality assessment

**Rating: A** (top institution + top venue + novel paradigm)

- **Institutions**: Fudan University (Yanghua Xiao's lab — same group behind CoSER, InCharacter, HER) + Johns Hopkins University (Jen-tse Huang). The Fudan role-play research group is the most prolific and highest-impact lab in this space.
- **Venue**: ACL 2026 Main Conference — the top NLP venue. Peer-reviewed and accepted.
- **Citations**: Too new for counts, but ACL 2026 main track acceptance is strong validation.
- **Code**: Not yet released at time of writing, but the dataset and evaluation framework are described in sufficient detail for replication. The data construction pipeline uses Gemini 2.5 Pro and Claude Sonnet 4.5 as generators.
- **Novelty**: This is the first paper to treat LLM anthropomorphism as a problem of *cognitive process simulation* rather than behavior imitation. It's philosophically different from all other role-play papers.

## The core insight

Every other role-play paper models personality as **isolated label-to-behavior mappings**: "extroverted" maps to "talkative," "agreeable" maps to "cooperative." But real human behavior doesn't work this way:

- A talkative person may fall *silent* when the spotlight effect is activated
- An assertive individual may *yield* under conformity pressure
- Someone high in loss aversion may *double down* on a bad decision when sunk cost fallacy is also active

Human behavior emerges from the **dynamic interplay of multiple cognitive patterns**, not from any single trait in isolation. Current methods — prompting, SFT, activation steering — all treat traits independently. This leads to "personality illusion": models report traits on questionnaires but behave inconsistently in context.

HumanLLM's thesis: **to make characters that truly think like humans, you must model the psychological processes generating behavior, not just the behavioral outcomes.**

## Method

### Pattern taxonomy (244 patterns)

HumanLLM decomposes human cognition into two dimensions (following Lewin's field theory: B = f(P, E)):

**Person dimension — 100 Personality Traits** (Goldberg's Big Five Unipolar Markers):
- 20 traits per Big Five dimension (Extraversion, Agreeableness, Conscientiousness, Emotional Stability, Intellect)
- Each with 10 positive-pole and 10 negative-pole descriptors
- Examples: talkative, assertive, sympathetic, organized, creative, anxious, cold, careless

**Environment dimension — 144 Social-Cognitive Patterns** curated from four theoretical traditions:

| Source | Count | Examples |
|--------|-------|---------|
| Cognitive biases & heuristics | 76 | Sunk cost fallacy, spotlight effect, confirmation bias, anchoring, framing effect |
| Social influence mechanisms | 27 | Authority bias, conformity, groupthink, bystander effect, reciprocity principle |
| Evolutionary adaptations | 11 | Kin selection, territoriality, jealousy, dominance hierarchies |
| Motivational processes | 30 | Self-determination theory, cognitive dissonance, terror management, flow principle |

Each pattern is backed by ~50 academic papers (~12,000 total across all patterns). For each pattern, the framework constructs a structured representation with three components:

1. **Definition** — precise characterization from authoritative sources
2. **Core Mechanisms** — underlying cognitive, emotional, and behavioral processes
3. **Real-World Manifestations** — how the pattern expresses across diverse contexts

### Scenario synthesis (11,359 scenarios)

Each scenario involves 2— characters and 2— patterns that may:

- **Align** (reinforce each other): "self-serving bias" + "overconfidence effect" → character who credits all successes to personal talent
- **Conflict** (compete): "assertive" + "conformity" → internal tension when the character's natural boldness collides with group pressure
- **Modulate** (one constrains another): "talkative" suppressed by "spotlight effect" → normally chatty character goes quiet when all eyes are on them

Character profiles contain self-perception (identity, personality, background, motivations) and other-perception (knowledge and attitudes toward other characters), enabling information asymmetry.

Scenarios are diversified using the DIAMONDS model (Rauthmann et al., 2014): Duty, Intellect, Adversity, Mating, Positivity, Negativity, Deception, and Sociality.

### Conversation generation

Multi-turn conversations (12—0 turns) per scenario, generated by Claude Sonnet 4.5. Each turn has three dimensions:

```
[Inner thoughts — enclosed in brackets]
(Physical actions — enclosed in parentheses)
Verbal expressions — unenclosed dialogue
```

Target patterns are naturally embedded across all three dimensions, guided by the scenario's expected behavioral tendencies.

### Dual-level checklists

Two complementary evaluation instruments:

**Pattern-level checklist** (15 items per pattern): Universal behavioral indicators derived from the pattern's definition. Context-independent — they apply whenever a character exhibits the pattern. Example for spotlight effect: "Shows heightened awareness of being observed."

**Scenario-level checklist** (2— items per character): Context-specific behavioral tendencies from the particular pattern combination. Example: "Projects outward confidence in expressing opinions, yet harbors internal anxiety regarding audience scrutiny" (assertive + spotlight effect in a public speaking scenario).

These checklists are the key evaluation innovation. Scoring uses GPT-5-mini as judge with ternary scoring: +1 (satisfied), 0 (not exhibited), -1 (violated).

### Training

SFT on Qwen3-8B and Qwen3-32B. Training mixture:

| Dataset | Samples | Ratio | Purpose |
|---------|---------|-------|---------|
| HumanLLM scenarios | 30,543 | 4 | Psychological pattern simulation |
| OpenThoughts-114k | 30,543 | 4 | General instruction-following |
| CoSER | 15,272 | 2 | Role-play dialogue |

Total: 76,358 samples. The 4:4:2 ratio was empirically determined.

## Experimental results

### Main results (Table 2 in paper)

Two metrics: IPE (Individual Pattern Expression) and MPD (Multi-Pattern Dynamics).

| Model | IPE (%) | MPD (%) |
|-------|---------|---------|
| **Closed-source** | | |
| Gemini 3 Pro | 41.1 | 85.3 |
| Claude Sonnet 4.5 | 34.6 | 79.7 |
| GPT-5 | 15.7 | 43.2 |
| **Open-source** | | |
| Qwen3-235B | 34.1 | 72.7 |
| Qwen3-32B | 26.2 | 66.0 |
| DeepSeek-R1 | 23.5 | 68.8 |
| DeepSeek-V3.2 | 22.0 | 65.3 |
| Qwen3-8B | 18.8 | 54.2 |
| **Ours** | | |
| **HumanLLM-32B** | **32.6** | **73.8** |
| **HumanLLM-8B** | **25.5** | **70.1** |

The headline result: **HumanLLM-8B (25.5 IPE, 70.1 MPD) outperforms Qwen3-32B (26.2 IPE, 66.0 MPD) on multi-pattern dynamics despite having 4x fewer parameters.** The 8B model also beats DeepSeek-V3.2 (a much larger model) on MPD.

IPE vs. MPD gap: all models score much higher on MPD than IPE. HumanLLM argues this is because models can produce superficially coherent multi-pattern behavior without deeply understanding individual pattern mechanisms — MPD is easier to "approximate," while IPE requires genuine pattern fidelity.

GPT-5's surprisingly low scores (15.7 IPE, 43.2 MPD — below most open-source models) suggest that strong instruction-following actively *hurts* pattern simulation. The model is too "helpful" to faithfully express cognitive biases and negative traits.

### Critical ablation: negative transfer (Table 3)

| Variant | IPE | MPD |
|---------|-----|-----|
| Qwen3-8B (base) | 18.8 | 54.2 |
| Qwen3-8B (OT + CoSER, no HumanLLM) | **8.9** | **31.1** |
| HumanLLM-8B (all three) | **25.5** | **70.1** |

Training on generic instruction data (OpenThoughts) and conventional role-play data (CoSER) *without* HumanLLM data causes **catastrophic degradation**: IPE drops 53%, MPD drops 43% vs. the base model. This suggests standard SFT actively suppresses the base model's latent ability to simulate psychological patterns by reinforcing "helpful assistant" behaviors.

The full mixture (with HumanLLM data) not only recovers but creates a synergistic effect — the psychological pattern data serves as an "anchor" that prevents the model from collapsing toward generic prosocial behavior.

### Evaluation framework validation (Table 5)

Comparison of HumanLLM's checklist metrics vs. CoSER's holistic metrics on 20 scenarios scored by both GPT-5-mini and three human experts:

| Metric | Human | LLM | 螖 | Pearson r |
|--------|-------|-----|---|-----------|
| **Holistic (CoSER)** | | | | |
| Anthropomorphism | 85.2 | 53.0 | -32.2 | 0.41 |
| Character Fidelity | 83.5 | 66.8 | -16.7 | 0.62 |
| **Checklist (HumanLLM)** | | | | |
| IPE | 39.0 | 38.6 | -0.4 | **0.91** |
| MPD | 71.7 | 75.0 | +3.3 | **0.88** |

Holistic metrics show weak human-LLM alignment (r=0.41—.62) with large systematic bias. HumanLLM's checklist metrics achieve r=0.91 and r=0.88 with near-zero bias. The paper calls this problem **normative confounding**: holistic LLM judges conflate "good anthropomorphism" with "prosocial behavior," penalizing realistic but negative human traits (defensiveness, cognitive biases).

Case study: a scenario featuring "ultimate attribution error" (defensively blaming out-groups) received a holistic score of 5/100 ("lack of empathy") but passed all checklist items. The behavior was *psychologically accurate* — just socially undesirable.

### External benchmarks (Table 4)

| Model | LifeChoice | CroSS-MR |
|-------|-----------|----------|
| GPT-5 | 85.53 | 62.25 |
| Claude Sonnet 4.5 | 85.49 | 68.24 |
| Qwen3-32B | 47.71 | 63.37 |
| HumanLLM-32B | **50.64** | **64.27** |
| HumanLLM-8B | 47.19 | 54.23 |

Moderate improvements on external benchmarks. The paper argues this is expected — LifeChoice and CroSS-MR evaluate behavioral *outcomes* (decisions, motivations) rather than cognitive *processes*. These benchmarks don't capture pattern-specific indicators, so they underestimate HumanLLM's contribution.

## Practical analysis for Rhapsode

### Why this matters for game NPCs

HumanLLM addresses a problem Rhapsode will inevitably face: **NPC behavioral realism under conflicting pressures.** Game characters routinely encounter situations where multiple psychological forces compete:

- A loyal guard who discovers corruption (loyalty vs. justice — cognitive dissonance)
- A cowardly merchant forced to defend their shop (self-preservation vs. loss aversion)
- A prideful warrior asked for help (assertiveness suppressed by spotlight effect of admitting weakness)

Standard role-play training produces characters that express one trait consistently. HumanLLM produces characters that exhibit *tension between traits* — the guard visibly struggling with their conscience, the merchant finding courage through panic about losing everything.

### Integration with Rhapsode's four-layer architecture

HumanLLM operates at a different level than the L1-3 stack. It's not about personality steering (L1) or knowledge retrieval (L2) or constitutional identity (L3) — it's about the **cognitive processes** underlying all three.

Potential integration:

1. **Pattern-annotated WorldGraph profiles**: Augment character nodes with explicit pattern assignments. A guard character gets: personality traits (conscientious, dutiful, assertive) + social-cognitive patterns (authority bias, in-group bias, moral licensing effect). The combination specification tells the system *how* these patterns interact in context.

2. **Scenario-level behavioral tendencies as Director prompts**: When the Director sets up a scene, it generates expected behavioral tendencies for each NPC based on their active patterns and the situation. These serve as soft constraints on NPC behavior.

3. **Dual-level checklists as evaluation**: Use pattern-level checklists to validate NPC behavior during development. "Does this guard exhibit in-group bias toward fellow guards? Does their authority bias manifest when speaking to officials?"

4. **Training data for companion LoRAs**: The HumanLLM dataset (or the construction pipeline) could generate psychologically-grounded training data for major NPC LoRA adapters. Rather than training on "what Kael would say," train on "how cognitive dissonance manifests in someone with Kael's pattern profile."

### The negative transfer finding is critical

The ablation showing that standard SFT *degrades* pattern simulation has direct implications for Rhapsode: **if we fine-tune a base model on conventional role-play data without psychological grounding, we may actively destroy its ability to simulate cognitively complex characters.** The HumanLLM data (or something similar) may be necessary as a training "anchor" whenever we do SFT.

### Scale advantage

HumanLLM-8B beating Qwen3-32B on multi-pattern dynamics means that **cognitive process simulation is more parameter-efficient than behavior memorization.** A 7—B model with the right training data can produce more psychologically realistic NPCs than a model 4x its size trained on conventional dialogue. This is extremely favorable for Rhapsode's local-deployment constraint.

### Limitations for Rhapsode

- **WEIRD psychology**: The 244 patterns come from Western academic psychology. Rhapsode's fantasy characters may exhibit patterns that don't map neatly to Big Five or standard cognitive biases.
- **Static patterns**: Patterns are assigned at scenario creation time. How patterns evolve (a character developing new cognitive biases through trauma) is not modeled.
- **No code released yet**: The pipeline depends on Gemini 2.5 Pro and Claude Sonnet 4.5 for generation, which conflicts with Rhapsode's local-only constraint for inference. However, the training data could be generated once with cloud models and then used locally.
- **SFT only**: No RL training. HER shows that RL adds meaningful gains on top of SFT. Combining HumanLLM's psychological grounding with HER's RL pipeline could yield the best of both.

### Potential synthesis: HumanLLM + HER

The two papers from the same lab address complementary problems:

| | HumanLLM | HER |
|---|----------|-----|
| **Focus** | *What* cognitive processes to simulate | *How* to think about those processes |
| **Key contribution** | 244-pattern taxonomy + multi-pattern dynamics | Dual-layer thinking + RL with trained GRM |
| **Training** | SFT only | SFT + RL |
| **Evaluation** | Dual-level checklists (r=0.91 with human experts) | By-case principles + GRM |

A combined approach: use HumanLLM's pattern taxonomy and training data to define *which* cognitive processes a character should exhibit, then use HER's dual-layer thinking and RL to train the model to *execute* those processes with genuine character voice. The HumanLLM checklists could augment HER's GRM as additional reward signals.

## Citation

```bibtex
@inproceedings{wang2026humanllm,
  title     = {HumanLLM: Benchmarking and Reinforcing LLM Anthropomorphism
               via Human Cognitive Patterns},
  author    = {Wang, Xintao and Yang, Jian and Li, Weiyuan and Xie, Rui
               and Huang, Jen-tse and Gao, Jun and Huang, Shuai
               and Kang, Yueping and Gou, Liyuan and Feng, Hongwei
               and Xiao, Yanghua},
  booktitle = {Proceedings of the 64th Annual Meeting of the Association
               for Computational Linguistics (ACL 2026)},
  year      = {2026}
}
```
