---
title: "Codifying Character Logic in Role-Playing"
arxiv: "2505.07705"
venue: NeurIPS 2025
authors: "Letian Peng, Jingbo Shang"
affiliation: UC San Diego
github: "https://github.com/KomeijiForce/Codified_Profile_Koishiday_2025"
last_updated: 2026-05-17
tags:
  - research
  - role-playing
  - structured-profiles
  - small-models
  - modular
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[research/papers/character-llm]]"
  - "[[architecture/world-graph]]"
---

# Codifying Character Logic in Role-Playing

Peng & Shang, NeurIPS 2025 | [arXiv](https://arxiv.org/abs/2505.07705) | [OpenReview](https://openreview.net/forum?id=RoVS9vmpd2) | [GitHub](https://github.com/KomeijiForce/Codified_Profile_Koishiday_2025)

## Institutional credibility

**UC San Diego**, Department of Computer Science. Jingbo Shang is an Associate Professor with h-index 37, Google PhD Fellowship (2017), 6,170 citations, PhD from UIUC under Jiawei Han. Published at **NeurIPS 2025** (top ML venue).

## Why this matters for Rhapsode

This paper offers a radically different approach to character control: instead of steering vectors or LoRA adapters, convert character descriptions into **executable Python functions** with if-then-else logic. The model executes the character's behavioral rules as code, not as natural-language interpretation. The headline result: **even 1B-parameter models can do high-quality role-playing** with codified profiles. This could eliminate the need for large models for minor NPCs entirely.

## Core idea

Traditional character profiles are natural-language descriptions ("She is brave but cautious"). The model must interpret these descriptions, which introduces ambiguity, inconsistency, and hallucination. Codified profiles convert the same descriptions into structured, executable functions:

**Natural-language profile:**
> Elizabeth is a medieval healer who distrusts authority. She will help anyone in need but refuses to serve the king's court. She speaks formally but warmly.

**Codified profile (simplified):**
```python
def parse_by_scene(scene):
    assertions = []
    if check_condition(scene, "someone needs medical help"):
        assertions.append("offer to help regardless of their status")
    if check_condition(scene, "asked to serve the king or court"):
        assertions.append("firmly but politely refuse")
    if check_condition(scene, "authority figure gives orders"):
        assertions.append("express distrust, question their motives")
    assertions.append("speak formally but with warmth")
    return assertions
```

The `check_condition(scene, question)` function lets the LLM evaluate whether a condition is true, false, or unknown given the current scene context. The codified profile then returns a list of behavioral assertions the model must follow.

## Three advantages over natural-language profiles

**1. Persistence**: Every relevant rule is checked and enforced on every response. Natural-language profiles rely on the model "remembering" all traits, which degrades with context length. Codified profiles execute completely every time.

**2. Updatability**: To change a character's behavior, edit a specific function branch. To add a new plot point ("Elizabeth now serves the court reluctantly"), add an if-branch. Changes are precise and auditable, unlike rewriting paragraphs of natural-language description.

**3. Controllable randomness**: The profile can include stochastic elements:
```python
if random.random() < 0.3:
    assertions.append("mention a past trauma obliquely")
```
This gives characters natural variation without being unpredictable.

## Evaluation

The authors created a benchmark from 83 characters and 5,141 scenes curated from Fandom wikis. Evaluation uses NLI-based scoring (does the response entail the character's known traits?).

Key finding: codified profiles enable **1B-parameter models** to achieve role-playing performance comparable to much larger models using natural-language profiles. The structure compensates for the model's limited capacity.

## How it maps to Rhapsode

This approach maps naturally to Rhapsode's existing WorldGraph:

| Codified Profiles concept | Rhapsode equivalent |
|---------------------------|-------------------|
| Character profile functions | Could be generated from WorldGraph character nodes |
| `check_condition(scene, question)` | WorldGraph edge queries + scene state |
| Behavioral assertions | Injected into prompt as structured rules |
| Profile updates | WorldGraph node mutations trigger profile regeneration |

### Integration path

1. For each character in WorldGraph, generate a codified profile (Python function) from their attributes, relationships, and plot state
2. At inference time, execute the profile against the current scene to produce behavioral assertions
3. Inject assertions into the prompt alongside RAG-retrieved memories
4. The model generates a response constrained by both factual memories (RAG) and behavioral rules (codified profile)

This could replace or augment the current natural-language character descriptions in the prompt, especially for minor NPCs where a LoRA adapter is too expensive and steering vectors are unreliable on Qwen.

### For Rhapsode's character evolution problem

Codified profiles handle evolution naturally: when a game event changes a character, update the profile function. Add a new branch, change a condition, modify an assertion. The change takes effect immediately -- no retraining, no vector re-extraction. This is the most lightweight evolution mechanism of any method in our survey.

## Limitations

- The paper doesn't address adversarial robustness. Since the profile is injected via prompt, it may be overridable.
- `check_condition()` requires an LLM call for each condition check, which adds latency proportional to the number of profile rules.
- Profile generation from natural-language descriptions may itself require a capable LLM (the paper uses GPT-4 for profile codification).
- NeurIPS 2025 (new) -- limited community validation so far, but strong venue and researcher credentials.

## Citation

```bibtex
@inproceedings{peng2025codifying,
  title     = {Codifying Character Logic in Role-Playing},
  author    = {Peng, Letian and Shang, Jingbo},
  booktitle = {NeurIPS},
  year      = {2025}
}
```
