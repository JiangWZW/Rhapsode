---
title: "CoSER: Coordinating LLM-Based Persona Simulation of Established Roles"
arxiv: "2502.09082"
venue: ICML 2025
authors: "Xintao Wang, Heng Wang, Yifei Zhang et al."
affiliation: Fudan University
github: "https://github.com/Neph0s/CoSER"
citations: "<30 (too new, as of 2026-05)"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - dataset
  - fine-tuning
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[research/papers/character-llm]]"
---

# CoSER: Coordinating LLM-Based Persona Simulation of Established Roles

Wang et al., ICML 2025 | [arXiv](https://arxiv.org/abs/2502.09082) | [GitHub](https://github.com/Neph0s/CoSER) (194 stars)

**Note:** This paper does not yet meet the 30-citation threshold (published Feb 2025). Included as an honorable mention due to its exceptional dataset scale and top venue (ICML).

## Core idea

The largest and most rigorous dataset for role-playing LLMs, covering 17,966 characters from 771 renowned books. Uses a "given-circumstance acting" methodology borrowed from theater practice to generate and evaluate character dialogues.

## Method

### Given-circumstance acting

Inspired by Stanislavski's acting method: an actor (or LLM) is placed in a specific situation from the source material and must respond authentically as the character, drawing on the character's background, experiences, and inner thoughts.

### Dataset construction

1. **Book selection** -- 771 books spanning diverse genres, eras, and cultures
2. **Character extraction** -- 17,966 characters identified with structured profiles (background, relationships, personality, key events)
3. **Scene reconstruction** -- Key scenes from books are reconstructed as multi-turn dialogues with authentic character voices
4. **Internal thoughts** -- Characters' internal monologues and motivations are annotated alongside their dialogue

### Models

- **CoSER 8B** -- LLaMA-3.1-8B fine-tuned on CoSER data
- **CoSER 70B** -- LLaMA-3.1-70B fine-tuned on CoSER data

### Follow-up: HER

The team also released HER (Human-like Reasoning and Reinforcement Learning for LLM Role-playing), extending CoSER with reasoning-augmented role-playing capabilities.

## Key results

| Benchmark | CoSER 70B | GPT-4o |
|-----------|-----------|--------|
| InCharacter | 75.80% | ~75% |
| LifeChoice | 93.47% | ~90% |

CoSER 70B matches or surpasses GPT-4o on multiple role-playing benchmarks.

## Code completeness

The repo provides dataset access, model weights on HuggingFace, and training/evaluation code. More complete than RoleLLM or DITTO for the actual training pipeline.

## Limitations

- Very new paper -- not yet widely validated by the community
- Large-scale training (70B) requires significant compute resources
- Dataset is book-focused; may not directly transfer to game character archetypes
- Character quality varies with the literary source quality

## Relevance to Rhapsode

**Rank: #3 -- best available pre-trained base model for local deployment.**

CoSER-8B (LLaMA-3.1-8B) is already fine-tuned on 17,966 characters and runs on consumer hardware. It can serve as Rhapsode's base model, providing stronger out-of-the-box role-playing than a generic instruction-following model, even before any LoRA fine-tuning.

| Aspect | Applicability |
|--------|--------------|
| CoSER-8B as base model | **Very High** -- drop-in replacement for generic LLaMA-3.1-8B with better role-play quality |
| Dataset scale | High -- 17K characters provides the largest training corpus available |
| Given-circumstance acting | High -- the theatrical methodology aligns with interactive narrative design |
| Internal thoughts annotation | High -- directly useful for generating NPC inner monologue / motivation in Rhapsode's Director |
| LLaMA-3.1 base | High -- compatible with Neeko-style LoRA adapters |
| Book-to-game transfer | Medium -- literary characters differ from game NPCs in interactivity patterns |

## Citation

```bibtex
@InProceedings{pmlr-v267-wang25dk,
  title     = {{C}o{SER}: Coordinating {LLM}-Based Persona Simulation
               of Established Roles},
  author    = {Wang, Xintao and Wang, Heng and Zhang, Yifei and others},
  booktitle = {Proceedings of the 42nd International Conference on
               Machine Learning},
  pages     = {64822--64858},
  year      = {2025},
  volume    = {267}
}
```
