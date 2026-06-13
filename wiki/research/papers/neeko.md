---
title: "Neeko: Leveraging Dynamic LoRA for Efficient Multi-Character Role-Playing Agent"
arxiv: "2402.13717"
venue: EMNLP 2024
authors: "Xiaoyan Yu, Tongxu Luo, Yifan Wei, Fangyu Lei, Yiming Huang, Hao Peng, Liehuang Zhu"
github: "https://github.com/weiyifan1023/Neeko"
citations: "~25 (Semantic Scholar, as of 2026-05; borderline)"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - lora
  - multi-character
related:
  - "[[research/llm-roleplay-survey]]"
---

# Neeko: Leveraging Dynamic LoRA for Efficient Multi-Character Role-Playing Agent

Yu et al., EMNLP 2024 | [arXiv](https://arxiv.org/abs/2402.13717) | [GitHub](https://github.com/weiyifan1023/Neeko) (137 stars)

**Note:** Citation count is borderline (~25, may have reached 30 by now). Included because its architecture is the most directly relevant for multi-character game engines.

## Core idea

A dynamic LoRA framework that assigns separate low-rank adapters to individual characters and uses a learned gating network for role selection. This allows a single base model to efficiently switch between many characters without full fine-tuning for each.

## Method

### Three-stage framework

1. **Agent Pre-training** -- The base LLM is pre-trained on a general role-playing corpus to acquire foundational character simulation capabilities.

2. **Multiple Character Playing** -- Individual LoRA blocks are trained for each character. A gating function learns to route inputs to the appropriate LoRA based on the specified role. During inference, the gating network activates the correct adapter given a role identifier.

3. **Character Incremental Learning** -- New characters can be added by training additional LoRA blocks without retraining existing ones. The gating network is updated to incorporate the new character.

### Dynamic LoRA routing

The key technical contribution: rather than merging all character knowledge into a single model (which causes interference between characters), Neeko keeps character-specific knowledge in separate LoRA modules. The gating network acts as a character selector, enabling efficient switching.

## Key results

- Superior performance in multi-character role-playing compared to single-model approaches
- Effective handling of both seen characters (trained LoRA exists) and unseen characters (nearest-neighbor routing)
- Character incremental learning works without catastrophic forgetting of existing characters

## Code completeness

The GitHub repo provides code and data for the three-stage framework, including LoRA training scripts.

## Limitations

- Requires one LoRA adapter per character -- storage scales linearly with character count
- Gating network accuracy for unseen characters depends on similarity to seen characters
- Not tested at very large character counts (hundreds+)
- LoRA adapters are static after training; no online adaptation

## Relevance to Rhapsode

**Rank: #1 -- primary architectural reference for Rhapsode's character system.**

Rhapsode requires local small LLMs (Qwen/LLaMA 7-8B) serving multiple NPCs from a shared base. Neeko's dynamic LoRA is the direct answer to this constraint.

| Aspect | Applicability |
|--------|--------------|
| Dynamic LoRA architecture | **Very High** -- the serving backbone for Rhapsode's Tier 2 characters |
| Incremental character addition | **Very High** -- new game characters added without retraining existing ones |
| Gating network for role selection | High -- maps to Rhapsode selecting which NPC is speaking via `character_agent.py` |
| Unseen character handling | High -- minor NPCs without trained LoRAs fall back to nearest archetype |
| Storage efficiency | High -- ~10-50MB per adapter vs. ~14GB per full model copy |

**Open question:** Neeko was validated on a small character set. Whether the gating network degrades with 50+ adapters is untested and relevant to Rhapsode's scaling needs. Archetype-level LoRAs (e.g., "warrior," "scholar," "trickster") may be more practical than per-individual LoRAs at scale.

## Citation

```bibtex
@inproceedings{yu-etal-2024-neeko,
  title     = "Neeko: Leveraging Dynamic LoRA for Efficient Multi-Character
               Role-Playing Agent",
  author    = "Yu, Xiaoyan and Luo, Tongxu and Wei, Yifan and Lei, Fangyu
               and Huang, Yiming and Peng, Hao and Zhu, Liehuang",
  booktitle = "Proceedings of the 2024 Conference on Empirical Methods
               in Natural Language Processing",
  year      = "2024",
  pages     = "12540--12557"
}
```
