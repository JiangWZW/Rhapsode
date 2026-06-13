---
title: "Character-LLM: A Trainable Agent for Role-Playing"
arxiv: "2310.10158"
venue: EMNLP 2023
authors: "Yunfan Shao, Linyang Li, Junqi Dai, Xipeng Qiu"
affiliation: Fudan University
github: "https://github.com/choosewhatulike/trainable-agents"
citations: "100+ (Google Scholar, as of 2026-05)"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - fine-tuning
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[architecture/memory-system]]"
---

# Character-LLM: A Trainable Agent for Role-Playing

Shao et al., EMNLP 2023 | [arXiv](https://arxiv.org/abs/2310.10158) | [GitHub](https://github.com/choosewhatulike/trainable-agents) (628 stars)

## Core idea

Train LLMs (LLaMA-7B) to act as specific historical/fictional characters via supervised fine-tuning on synthetic biographical data, rather than relying on system-prompt engineering.

## Method: Experience Reconstruction

The pipeline has three stages:

1. **Scene Extraction** -- Given a character profile (e.g. a Wikipedia biography), GPT-3.5 extracts discrete "scenes" representing key life events, relationships, and interactions.
2. **Experience Completion** -- Each scene is expanded into multi-turn dialogues where the character interacts with others in period-appropriate settings. This produces the supervised fine-tuning data.
3. **Protective Experiences** -- Additional training data where the character correctly refuses knowledge outside their temporal/contextual scope (e.g. Beethoven refusing to discuss modern technology). This reduces "character hallucination."

The model is then SFT-trained on this data using FastChat with FSDP, producing one fine-tuned model per character.

## Characters trained

Nine characters: Beethoven, Cleopatra VII, Julius Caesar, Socrates, Isaac Newton, Martin Luther King Jr., Hermione Granger, Lord Voldemort, Spartacus.

Training data per character: ~1,600 scenes, ~750K words, ~13 dialogue turns per scene.

## Evaluation

The authors built an interview-based evaluation playground that probes:

- **Memorization** -- Does the agent recall character-specific facts?
- **Values** -- Does it exhibit the character's known beliefs and attitudes?
- **Personality** -- Does it maintain consistent personality traits?
- **Hallucination avoidance** -- Does it refuse anachronistic or out-of-scope questions?
- **Stability** -- Is behavior consistent across multiple interactions?

Character-LLMs outperform prompted baselines (Alpaca, Vicuna) on all dimensions. Protective experiences significantly reduce hallucination.

## Code completeness

**Excellent** -- the most complete repo among all surveyed papers:

- Full data generation pipeline (scene extraction, experience completion, protective scenes via GPT-3.5 API)
- Training scripts (FastChat-based SFT, FSDP on 8xA100, ~30-45 min per character)
- Inference server (FastChat model workers with OpenAI-compatible API)
- Single-turn and multi-turn interview evaluation scripts
- Pre-trained model weights for all 9 characters on HuggingFace (`fnlp/`)
- Training dataset on HuggingFace (`fnlp/character-llm-data`)

## Limitations

- **One model per character** -- no multi-character support. Each character requires a separate fine-tune, making this expensive at scale.
- **LLaMA-1 base** -- the released models use LLaMA-1 7B, which is outdated. The method itself is model-agnostic but the provided code targets the LLaMA-1 ecosystem.
- **GPT-3.5 dependency** -- data generation requires OpenAI API calls. The quality of training data is bounded by GPT-3.5's capabilities.
- **Static characters** -- no mechanism for characters to learn or evolve during gameplay.

## Relevance to Rhapsode

**Rank: #6 -- data generation methodology reference only. Per-character SFT is ruled out by Rhapsode's modular constraint.**

The per-character fine-tuning approach is too expensive for a game with many NPCs on local hardware. However, two ideas from this paper transfer directly:

| Aspect | Applicability |
|--------|--------------|
| Experience Reconstruction pipeline | High -- the data generation method (profile to scenes to dialogues) applies to LoRA training data, not just full SFT |
| Protective experiences | **Very High** -- essential for preventing NPCs from breaking the fourth wall; directly adopted into Rhapsode's training data generation |
| Per-character SFT | **Ruled out** -- too expensive; Rhapsode uses LoRA adapters (Neeko-style) instead |
| Interview evaluation | Medium -- the 5 probe dimensions (memorization, values, personality, hallucination, stability) inform Rhapsode's evaluation criteria |

## Citation

```bibtex
@inproceedings{shao2023character,
    title={Character-LLM: A Trainable Agent for Role-Playing},
    author={Yunfan Shao and Linyang Li and Junqi Dai and Xipeng Qiu},
    booktitle={EMNLP},
    year={2023}
}
```
