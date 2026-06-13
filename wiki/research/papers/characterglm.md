---
title: "CharacterGLM: Customizing Social Characters with Large Language Models"
arxiv: "2311.16832"
venue: EMNLP 2024 (Industry Track)
authors: "Jinfeng Zhou, Zhuang Chen, Dazhen Wan et al. (17 authors)"
affiliation: Tsinghua University
github: "https://github.com/thu-coai/CharacterGLM-6B"
citations: "~19 (Semantic Scholar, as of 2026-05; below threshold)"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - character-design
  - production
related:
  - "[[research/llm-roleplay-survey]]"
---

# CharacterGLM: Customizing Social Characters with Large Language Models

Zhou et al., EMNLP 2024 Industry Track | [arXiv](https://arxiv.org/abs/2311.16832) | [GitHub](https://github.com/thu-coai/CharacterGLM-6B) (499 stars)

**Note:** Does not meet the 30-citation threshold (~19 citations). Included because its attribute/behavior decomposition is the most production-oriented character design framework and its model is fully released.

## Core idea

A series of models (6B to 66B) built on ChatGLM, designed for character-based dialogue (CharacterDial). Characters are defined through a clean separation of **attributes** (static identity) and **behaviors** (dynamic interaction patterns), which is the most practical decomposition for game character design.

## Character design framework

### Attributes (static)

- Identity (name, age, gender, occupation)
- Interests and hobbies
- Viewpoints and beliefs
- Life experiences and backstory
- Achievements and milestones
- Social relationships

### Behaviors (dynamic)

- Linguistic features (vocabulary, sentence patterns, catchphrases)
- Emotional expressions (how the character shows joy, anger, sadness)
- Interaction patterns (proactive vs reactive, formal vs casual)

This attribute/behavior separation allows game designers to configure characters declaratively without writing dialogue data.

## Training pipeline

1. **Dialogue data collection** -- Combination of human role-play, GPT-4 synthesis, and literary extraction. Categories include celebrity conversations, daily life, and emotional dialogue.
2. **Supervised fine-tuning** -- ChatGLM base model fine-tuned with character prompts from diverse profiles. Summarization and paraphrasing techniques increase generalizability.
3. **Self-refinement** -- Post-deployment iterative improvement using user feedback from human-prototype interactions (inspired by LaMDA).

## Key results

- Outperforms GPT-3.5 in consistency, human-likeness, and engagement (human evaluation)
- Performs comparably to GPT-4 across the model size range
- Superior long-dialogue coherence compared to baseline LLMs

## Code completeness

The 6B model is fully released with inference code and a subset of training data. The repo provides:

- CharacterGLM-6B model weights
- Inference scripts for character dialogue
- Character configuration examples
- Apache-2.0 license

Training code for the full pipeline is not included, but the released model is directly usable.

## Limitations

- ChatGLM base -- primarily targets Chinese language; English performance less validated
- Self-refinement loop requires deployment and user interaction (not reproducible offline)
- Citation count is low relative to its GitHub popularity (499 stars)
- Larger models (66B) not publicly released

## Relevance to Rhapsode

**Rank: #5 -- design influence only (character profile schema). Not adopted as a model or training approach.**

CharacterGLM is tied to the ChatGLM family and primarily targets Chinese. Its contribution to Rhapsode is purely conceptual: the attribute/behavior decomposition is the cleanest character profile schema and directly informs how WorldGraph character nodes should be structured.

| Aspect | Applicability |
|--------|--------------|
| Attribute/behavior decomposition | **Very High** -- adopted as the schema for Rhapsode's character profiles in WorldGraph |
| Declarative character configuration | **Very High** -- game designers configure attributes and behaviors; these feed into prompt context (Tier 1) and training data generation (Tier 2) |
| Long-dialogue coherence | High -- essential for extended game sessions |
| Self-refinement loop | Low -- requires deployment and user feedback; not practical for offline game development |
| ChatGLM base model | **Not applicable** -- Rhapsode uses Qwen/LLaMA, not ChatGLM |

## Citation

```bibtex
@inproceedings{zhou-etal-2024-characterglm,
  title     = "{C}haracter{GLM}: Customizing Social Characters with
               Large Language Models",
  author    = "Zhou, Jinfeng and Chen, Zhuang and Wan, Dazhen and others",
  booktitle = "Proceedings of the 2024 Conference on Empirical Methods
               in Natural Language Processing: Industry Track",
  year      = "2024",
  pages     = "1457--1476"
}
```
