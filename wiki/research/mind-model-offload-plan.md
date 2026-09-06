---
title: Plan — offload per-character mind calls to a trained model
date: 2026-09-06
tags: [plan, role-playing, fine-tuning, minds, latency]
status: proposed
related:
  - "[[research/papers/coser]]"
  - "[[research/papers/her-dual-layer-thinking]]"
  - "[[architecture/monologue-streams]]"
  - "[[episodes/2026-08-10-long-run-storyline-and-character-collapse]]"
---

# Plan — offload per-character mind calls to a trained model

## Goal

Move per-character mind calls to a cheaper model where that costs nothing in quality. The two calls are gated separately:

- **Perception** — short read of the last few turns. Low stakes (one bad line lasts one turn). Already on the cheaper tier; offloading it mainly proves the pipeline.
- **Monologue** — the character's only durable private memory, on pro with thinking, and where the 2026-09-05 study A/B showed the study paying off. Expensive and risky. A flattened monologue compounds.

"Perception offloaded, monologue stays on pro" is an acceptable end state. An automatic in-character judge is a by-product.

## Non-goals

- Replacing or changing the narrator.
- Per-character models or adapters.
- New runtime architecture. The mind callbacks already take a provider; this plan only changes what answers them.

## Gates

Every stage ends in the same test: the existing five-turn narrator A/B, treatment vs control, judged on the downstream monologue lines. Control is always the current frontier setup. One variable per run.

### Stage 0 — Baseline without training (1 day)

1. Export logged mind calls to JSONL: `{character, turn, kind: perception|monologue, prompt, output}`. Drop unparseable outputs. Target 2–5k pairs; run long autoplays if short.
2. Run the A/B twice, one variable each: (a) perception on Flash with thinking off; (b) monologue on Flash with thinking off. Everything else as current.
3. **G0a / G0b:** for each call kind, if Flash-off is indistinguishable on downstream monologue quality, switch that call and drop it from the rest of the plan. Expect (a) to pass and (b) to fail; if (b) passes too, stop here.

### Stage 1 — Judge (2 days, ~$10)

1. Rent one A100 80 GB. Serve `HER-RM-32B` with vLLM (fallback: RoleRM-8B).
2. Feed ~50 line pairs from the 09-05 A/B (same core + scene, control line vs treatment line). Compare its preference with the hand judgments in `judgment.md`.
3. **G1 (soft):** ≥ ~80% agreement → use it as the automatic judge from here on. Otherwise judge by hand with a fixed rubric and revisit later.

### Stage 2 — SFT (3 days, ~$40)

1. Same A100. Base: `HER-32B`. LoRA r=16–32, 1–2 epochs, Unsloth or TRL.
2. Data: Stage 0 pairs, plus 10–20k CoSER turns with inner thoughts reformatted to the perception/monologue JSON schemas. Hold out one character (Wiz or Luna) entirely.
3. Serve with vLLM on the same box; point the mind callbacks at the OpenAI-compatible endpoint.
4. A/B in two steps, trained vs current, one call kind at a time:
   - **G2a (perception):** trained perception, monologue still on pro. Pass = downstream monologue lines not worse, first person intact, < 5 s per call. This is the cheap proof that the pipeline works; low bar.
   - **G2b (monologue):** trained monologue, perception per G2a outcome. Pass = lines at least as good as pro by the judge and a hand read, first person intact, held-out character not worse than in-set, and no drift toward shared sentiment across the five turns. Strict bar; this is the call that matters.
5. If G2a passes and G2b fails: keep perception on the trained model, monologue on pro, and decide whether Stage 3 is worth it.

### Stage 3 — RL, only if G2b fails on quality (1 week, ~$100)

Monologue only. On-policy distillation or GRPO with the Stage 1 judge as reward, on the 32B or a 4B–9B student. Skip if G2b passes, or if the saving from monologue alone doesn't justify a week.

### Stage 4 — Serving decision (no engineering)

Cost per month at the real turn volume for: rented endpoint (H100 ≈ $1.5–3/h), Flash API, local 9B on the 3080 Ti. If the 32B passes G2 but is too costly to keep warm, distill it into Qwen3.5-9B (SFT on the 32B's outputs) and rerun G2 locally.

### Stage 5 — Drift detector (optional, after the above)

Run the judge post-turn on each monologue line against the character core. Log only, do not block. Then replay a long run and check whether it would have flagged the convergence seen in `default-guide-300`.

## Rules

- Keep the frontier control in every A/B.
- Never change perception and monologue in the same run.
- Do not paste study pages onto the narrator (already shown to empty beats).
- Everything under `offline/` or `experiments/`; nothing in `server/` beyond an endpoint setting.
- Stop at the first gate that says the cheaper option is good enough.

## Why these choices

- HER-32B over CoSER-70B or raw Qwen: same judge, CoSER Test avg 53.1 vs 35.9; already trained for first-person character thinking (dual-layer thinking) with public data and reward model.
- CoSER as data, not as checkpoint: largest authentic corpus with inner thoughts; its models are superseded.
- Flash-off first: a trained 4B–7B matches Flash-class models on CoSER Test in published tables, so Flash is the bar a trained model must clear; if Flash alone clears it, nothing else is needed.

## Budget

Under $100 through G2. About two weeks part-time.
