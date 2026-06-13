---
title: "HER: Human-like Reasoning and Reinforcement Learning for LLM Role-playing"
arxiv: "2601.21459"
venue: arXiv 2026
authors: "Chengyu Du, Xintao Wang, Aili Chen, Weiyuan Li, Rui Xu, Junteng Liu, Zishan Huang, Rong Tian, Zijun Sun, Yuhao Li, Liheng Feng, Deming Ding, Pengyu Zhao, Yanghua Xiao"
institutions: "Fudan University, MiniMax"
github: "https://github.com/cydu24/HER"
github_stars: null
pdf: "https://arxiv.org/pdf/2601.21459"
last_updated: 2026-05-19
quality: A
tags:
  - research
  - role-playing
  - reasoning
  - reinforcement-learning
  - reward-modeling
  - cognitive-simulation
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[research/papers/rar-thinking-in-character]]"
  - "[[research/papers/coser]]"
  - "[[research/papers/open-character-training]]"
  - "[[architecture/companion-system]]"
---

# HER: Human-like Reasoning and Reinforcement Learning for LLM Role-playing

Du et al., arXiv Feb 2026 | [arXiv](https://arxiv.org/abs/2601.21459) | [GitHub](https://github.com/cydu24/HER) | [HER-32B model](https://huggingface.co/ChengyuDu0123/HER-32B) | [HER-RM-32B reward model](https://huggingface.co/ChengyuDu0123/HER-RM-32B) | [Dataset](https://huggingface.co/datasets/ChengyuDu0123/HER-Dataset)

## Quality assessment

**Rating: A** (top lab + strong results + full open release)

- **Institutions**: Fudan University (same lab as CoSER, InCharacter — the leading role-play research group) + MiniMax (major Chinese AI company with production role-play products).
- **Venue**: arXiv preprint (Feb 2026). Not yet peer-reviewed, but the Fudan role-play group has a strong publication track record (CoSER at ICML 2025, InCharacter at ACL 2024).
- **Citations**: Too new for citation counts.
- **Code**: Full pipeline released — training code, data synthesis pipeline, reward model, final model weights (32B), and dataset on HuggingFace. This is the most complete release of any cognitive role-play paper.
- **Production deployment**: MiniMax-M2-HER is a deployed commercial variant, ranking 6th on both CoSER and MiniMax benchmarks ahead of DeepSeek-v3.1 and approaching Claude-level scores.

## The core problem

All prior role-play training methods focus on either (a) what a character *says* (dialogue SFT — CoSER, Character-LLM, RoleLLM), (b) what rules a character *follows* (constitutional DPO — OCT), or (c) how a character's response *sounds* (reasoning style — RAR). None teach the model to *think* like the character at a cognitive level.

When reasoning models (DeepSeek-R1, QwQ) are applied to role-play, they generate internal thoughts that read like a helpful assistant planning how to be helpful ("I need to carefully consider the persona constraints and plan my response accordingly...") rather than a character with genuine inner life ("*Why does he look at me so?* The grounds are unexpectedly pleasant...").

HER identifies two separate deficiencies:

1. **No high-quality reasoning traces in existing data.** CoSER dialogues include short bracketed thoughts, but these are shallow ("I'm nervous"). The underlying motivations, plans, and emotional reasoning are implicit — human readers can infer them, but there's no explicit supervision signal.

2. **No reliable reward signals for role-play.** Role-play responses are open-ended and non-verifiable. General reward models get gamed by superficial cues (verbosity, sentiment words). Fixed evaluation rubrics miss what matters for a specific character in a specific scene.

## Method: Dual-layer Thinking + RL

### Dual-layer Thinking

HER's key conceptual contribution: two distinct types of thinking serve fundamentally different functions.

**System thinking** (3rd person, hidden from user and reward model):
```
I need to play Elizabeth Bennet. My persona is quick-witted, independent,
despises false pride. The scene is a tense reunion at Pemberley. Previous
conflicts are unresolved. Plan: stay polite, deflect with irony...
```

This is the LLM reasoning about *how to portray the character* — understanding constraints, planning trajectory, tracking continuity. Generated once per turn, then discarded from conversation history.

**Role thinking** (1st person, visible, evaluated by the reward model):
```
[Why does he look at me so?] (raises eyebrow) "The grounds are
unexpectedly pleasant, Mr. Darcy."
```

This is the *character's* inner monologue — emotions, intentions, decisions — interleaved with actions and speech. This is what users care about, and what the reward model judges.

The ordering and composition of role-level elements (thoughts, actions, speech) are flexible — the model decides based on context rather than following a fixed template. This prevents pattern collapse.

**Why this distinction matters vs. RAR:** RAR (Tang et al., NeurIPS 2025) puts everything into a single `<think>` block. HER argues this conflation causes two problems: (1) no dedicated planner for constraint-following, and (2) the reward model can't properly supervise role thinking when it's mixed with system thinking. HER explicitly calls out RAR's design as flawed (Section 3.1: "Existing methods often fail to distinguish system reasoning from role thinking").

### Three-stage reverse synthesis pipeline

High-quality role-play dialogues exist in novels (via CoSER's 771-book corpus), but their underlying reasoning is implicit. HER reverse-engineers both thinking layers from surface dialogues.

**Stage 1 — Role thinking augmentation**: A strong teacher model generates first-person role thinking to explain each turn's emotions and intentions. The paired action and speech are revised for within-turn consistency. Then each turn is rewritten into multiple layouts by varying the interleaving of thoughts, actions, and speech (661 → 939 unique patterns), preventing template collapse during training.

**Stage 2 — System thinking construction**: Forward generation produces a draft plan from prompt and history. Backward rewrite revises this draft using the ground-truth continuation, removing first-person content that belongs in role thinking. This ensures system thinking stays third-person and planning-oriented.

**Stage 3 — Integration & context augmentation**: The original role-play prompt is cross-checked against the source novel and current dialogue. Missing facts are added, unsupported details removed. This provides explicit constraints for the reward model to learn valid scene-specific principles.

### Role-play Generative Reward Model (GRM)

Rather than scoring with a single number, the GRM follows a structured evaluation process:

1. **Generate by-case principles** — scene-specific preference constraints derived from the dialogue context (not a fixed rubric)
2. **Analyze candidates** — compare two responses against these principles with concrete pros and cons
3. **Output preference** — binary win/lose/tie decision

**Principle distillation**: A teacher LLM analyzed 300K preference pairs and generated 3— principles each, producing 36,373 unique raw principles. Clustered into 15 categories, distilled to 107 candidates, then expert-refined to 51 finalized principles across 12 dimensions. These include Emotional Consistency, Status Consistency, Openness & Diversity, Reader Experience, and others.

**Anti-shortcut measures**: Unbalanced preference data causes the GRM to collapse — e.g., always claiming one side wins on every dimension ("pattern bias"). HER tracks a "Mixed Selection %" metric (dimension-wise winners are diverse, not uniform). Balanced construction maintains this at ~70%, while unbalanced training collapses to ~6%. Balanced GRM achieves 73.99% accuracy vs. 69.91% unbalanced.

### RL for the generator

- Initialize policy from SFT checkpoint
- Keep frozen SFT model to produce baseline responses
- For each context: sample policy response, pair with frozen baseline, GRM judges (win/lose/tie), map to scalar reward (+1 / -1 / 0)
- Optimize with clipped policy objective (GRPO variant)

## Experimental results

### Main leaderboard (Table 2 in paper)

CoSER benchmark (0-100, higher is better):

| Rank | Model | Avg | SC | AN | CF | SQ |
|------|-------|-----|----|----|----|----|
| 1 | Claude-4.5-Opus | 62.4 | 63.7 | 64.3 | 58.5 | 63.2 |
| 2 | Gemini-3-Pro | 61.8 | 66.0 | 60.4 | 58.3 | 62.5 |
| 3 | GPT-5.1 | 61.1 | 65.0 | 54.0 | 60.1 | 65.4 |
| 6 | MiniMax-M2-HER | 57.3 | 60.0 | 50.1 | 49.3 | 69.8 |
| 7 | DeepSeek-v3.1 | 53.5 | 50.2 | 53.2 | 53.9 | 56.7 |
| **8** | **HER-RL** | **53.1** | **54.3** | **47.3** | **52.8** | **58.1** |
| 9 | HER-SFT | 50.9 | 50.5 | 46.0 | 49.8 | 57.4 |
| 13 | CoSER-70B | 36.0 | 35.1 | 31.2 | 32.3 | 45.3 |
| 16 | GPT-4o | 27.7 | 34.0 | 14.9 | 22.9 | 38.9 |
| 17 | Qwen3-32B | 22.9 | 30.6 | 19.6 | 15.5 | 30.6 |

HER-RL (32B) scores 53.1, a **+30.2 improvement over its Qwen3-32B base** (22.9). For context, this puts an open-source 32B model competitive with DeepSeek-v3.1 and ahead of GPT-4o. RL adds +2.2 over SFT alone, with the largest gain in Storyline Quality (+0.75).

MiniMax Role-Play Bench (100-turn self-chat):

| Model | Avg | Preference |
|-------|-----|------------|
| MiniMax-M2-HER | 84.7 | 97.5 |
| HER-RL | 65.7 | 86.9 |
| HER-SFT | 58.4 | 86.4 |
| Qwen3-32B | 50.8 | 89.5 |
| CoSER-70B | 45.4 | 82.6 |

### Ablation: system thinking matters (Figure 5 in paper)

| Variant | CoSER Avg |
|---------|-----------|
| Base model (no SFT) | ~22.9 |
| SFT without system thinking | 48.6 |
| SFT with system thinking | 50.9 |
| + RL | 53.1 |

System thinking adds +2.3 on top of SFT, with the largest gains on Character Fidelity (+3.21) and Story Consistency (+2.60). The model adaptively adjusts thinking length: mean 580 tokens, range 77—,443 tokens — shorter for casual moments, longer for complex scenes.

### GRM design ablation (Table 3 in paper)

| GRM format | Agreement with human experts |
|------------|------------------------------|
| General principles + point-wise | 60.0% |
| By-case principles + point-wise | 86.0% |
| By-case principles + pairwise | 88.0% |
| By-case principles + pairwise + CoT | **93.0%** |

By-case principles are critical — jumping from 60% to 86% agreement. The full pipeline reaches 93% agreement with human expert consensus.

### Diversity preservation (Table 6 in paper)

Without explicit diversity measures, RL training causes pattern collapse — 96.1% of responses follow a single interleaving template by step 50. HER's diversified SFT data (939 unique patterns) maintains diversity: Top-1 pattern at 49.2%, Shannon entropy at 2.15 (vs. 0.28 collapsed), Self-BLEU at 0.0013 (vs. 0.0140 collapsed at 4-gram).

## Practical analysis for Rhapsode

### What HER gives us

HER represents a generation beyond RAR. Where RAR adds a structured thought block (emotion/experience/stance/motivation), HER adds:

1. **A hidden planning layer** (system thinking) that tracks persona constraints and plans trajectory — Rhapsode's Director prompt could serve a similar function
2. **A visible character cognition layer** (role thinking) with flexible interleaving of thoughts, actions, and speech — directly maps to companion system output
3. **A principled reward model** trained on scene-specific criteria — could serve as a quality gate for generated NPC dialogue

### How HER relates to the four-layer architecture

HER's dual-layer thinking doesn't replace our L1-3 stack; it adds an **L+ reasoning layer** that sits between memory retrieval (L2) and response generation:

```
L2 retrieves: "I met this merchant in Thornwall. He cheated the town."
System thinking: "I'm playing Kael, a suspicious guard. The merchant is here
  again. My constitution says I protect townsfolk fiercely. Plan: confront but
  stay controlled — Kael doesn't do rash things."
Role thinking: [That face. Thornwall. The missing grain.] (hand moves to hilt)
  "You've some nerve showing your face here, merchant."
```

### Scale considerations

HER trains on Qwen3-32B, which is too large for Rhapsode's local-deployment constraint (7—B target). However:

- The dual-layer thinking format is model-agnostic — distillation to 7—B is the obvious next step (HER's own data synthesis pipeline uses a teacher鈫抯tudent approach)
- The GRM is a separate 32B model used only during training, not inference
- HER's SFT data (from 760 books, 17,966 characters, 383,654 utterances) is released and could be used to train smaller models

### Comparison with RAR and OCT

| Dimension | RAR | OCT | HER |
|-----------|-----|-----|-----|
| What it trains | How to structure thoughts | What rules to follow | How to think + plan + act |
| Thinking model | Single `<think>` block | No explicit thinking | Dual-layer (system + role) |
| Training signal | SFT + DPO (style contrast) | DPO (constitution) | SFT + RL (generative RM) |
| Reward model | None (supervised only) | None | Dedicated Role-play GRM with 51 principles |
| Anti-hacking | None | None | Balanced construction, pattern bias monitoring |
| Base model | LLaMA-3-8B | LLaMA/Qwen/Gemma 4—B | Qwen3-32B |
| Best benchmark | CharacterBench 3.69 | Adversarial F1 0.86—.95 | CoSER 53.1 |
| Open release | Data only (no adapter) | 33 LoRA adapters | Full: model + RM + data + code |

### Open questions for Rhapsode

- **Distillation to 7—B**: HER's 32B results are strong, but do they survive distillation? The dual-layer format adds ~580 tokens of system thinking overhead per turn — on a 7B model at 30 tok/s, that's ~19 seconds of hidden computation before the character speaks. May need aggressive length constraints.
- **Conditional system thinking**: For routine NPC dialogue ("Welcome to the shop"), the full dual-layer pipeline is overkill. Rhapsode could enable system thinking only for important NPCs in emotionally-charged scenes.
- **GRM as quality gate**: HER's GRM could serve as an offline evaluator for Rhapsode's NPC dialogue quality, even if we don't use RL for training.
- **Compatibility with LoRA**: HER trains a full model. Whether the dual-layer format works well with per-character LoRA adapters (our L3 strategy) is untested.

## Citation

```bibtex
@article{du2026her,
  title   = {HER: Human-like Reasoning and Reinforcement Learning
             for LLM Role-playing},
  author  = {Du, Chengyu and Wang, Xintao and Chen, Aili and Li, Weiyuan
             and Xu, Rui and Liu, Junteng and Huang, Zishan and Tian, Rong
             and Sun, Zijun and Li, Yuhao and Feng, Liheng and Ding, Deming
             and Zhao, Pengyu and Xiao, Yanghua},
  journal = {arXiv preprint arXiv:2601.21459},
  year    = {2026}
}
```
