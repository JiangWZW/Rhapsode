---
title: "RoleLLM: Benchmarking, Eliciting, and Enhancing Role-Playing Abilities of LLMs"
arxiv: "2310.00746"
venue: ACL 2024 Findings
authors: "Zekun Moore Wang et al. (17 authors)"
github: "https://github.com/InteractiveNLP-Team/RoleLLM-public"
citations: "54 (Semantic Scholar) / ~106 (Google Scholar, as of 2026-05)"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - benchmark
  - fine-tuning
related:
  - "[[research/llm-roleplay-survey]]"
---

# RoleLLM: Benchmarking, Eliciting, and Enhancing Role-Playing Abilities of LLMs

Wang et al., ACL 2024 Findings | [arXiv](https://arxiv.org/abs/2310.00746) | [GitHub](https://github.com/InteractiveNLP-Team/RoleLLM-public) (522 stars)

## Core idea

A four-stage framework that constructs role profiles, extracts role-specific knowledge from scripts and books, imitates speaking styles, and fine-tunes open-source models via Role-Conditioned Instruction Tuning (RoCIT). Produces the first systematic character-level benchmark (RoleBench) with 168,093 samples across 100 characters.

## Method

### Stage 1: Role Profile Construction

For 100 roles (mix of English literary/film characters and Chinese historical figures), the authors collect structured profiles including name, background story, personality traits, and representative dialogues from scripts.

### Stage 2: Context-Instruct

A novel method for extracting role-specific knowledge from long-form source material (scripts, novels). Given a character's dialogue lines in context, GPT-4 generates QA pairs that capture the knowledge embedded in the character's speech patterns and story context.

### Stage 3: RoleGPT

GPT-4 is prompted with role profiles and example dialogues to generate role-play instruction-response pairs that imitate each character's speaking style. This captures stylistic elements (vocabulary, sentence structure, catchphrases) beyond factual knowledge.

### Stage 4: Role-Conditioned Instruction Tuning (RoCIT)

The combined Context-Instruct and RoleGPT data (RoleBench) is used to fine-tune open-source LLMs with role-conditioning -- the model receives a role identifier as part of the instruction and learns to modulate its output accordingly. This produces:

- **RoleLLaMA** -- English role-playing model (LLaMA-based)
- **RoleGLM** -- Chinese role-playing model (ChatGLM-based)

## RoleBench dataset

| Metric | Value |
|--------|-------|
| Total samples | 168,093 |
| Number of roles | 100 |
| English roles | ~80 |
| Chinese roles | ~20 |
| Source types | Film scripts, novels, historical records |

RoleBench is integrated into the [OpenCompass](https://github.com/open-compass/opencompass) evaluation framework.

## Key results

- RoleLLaMA and RoleGLM achieve comparable performance to RoleGPT (GPT-4 with role prompting)
- Context-Instruct is more effective than naive profile injection for knowledge-intensive characters
- Speaking style transfer is easier to achieve than deep knowledge transfer

## Code completeness

**Partial** -- the repo is primarily a data and benchmark release:

- RoleBench dataset available on HuggingFace (`ZenMoore/RoleBench`)
- Role profile data and Context-Instruct prompts included
- Framework documentation and evaluation scripts
- The actual RoCIT training scripts and full fine-tuning pipeline are less documented than Character-LLM's repo

## Limitations

- Heavy reliance on GPT-4 for both data generation stages (Context-Instruct and RoleGPT)
- RoleBench is static -- no mechanism for extending to new characters without repeating the full pipeline
- Evaluation primarily through automated metrics; limited human evaluation
- Training code not fully self-contained in the repo

## Relevance to Rhapsode

**Rank: #4 -- fallback multi-role approach if LoRA routing proves too complex; evaluation benchmark.**

RoCIT trains a single model for multiple roles, which is simpler than Neeko's LoRA routing but less modular (adding a new character means retraining). It serves as a fallback if the dynamic LoRA approach encounters scaling issues. RoleBench is useful for evaluation.

| Aspect | Applicability |
|--------|--------------|
| Context-Instruct | High -- extracting character knowledge from game lore documents maps to WorldGraph population |
| RoleBench as evaluation | High -- 100-character benchmark usable for testing Rhapsode's character quality |
| RoCIT (single model, multiple roles) | Medium -- fallback if LoRA gating doesn't scale; less modular than Neeko |
| Speaking style separation | Medium -- DITTO's cross-supervision finding confirms: style is cheap (LoRA), knowledge is expensive (base model) |
| GPT-4 data dependency | **Negative** -- Context-Instruct and RoleGPT require GPT-4, incompatible with Rhapsode's local-only constraint for data generation |

## Citation

```bibtex
@article{wang2023rolellm,
  title   = {RoleLLM: Benchmarking, Eliciting, and Enhancing Role-Playing Abilities of Large Language Models},
  author  = {Zekun Moore Wang and Zhongyuan Peng and Haoran Que and others},
  year    = {2023},
  journal = {arXiv preprint arXiv: 2310.00746}
}
```
