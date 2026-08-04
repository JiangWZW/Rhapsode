---
title: "LLM role-play literature — comprehensive report"
date: 2026-08-04
tags: [research, role-play, scaffolding, survey]
related:
  - "[[research/llm-roleplay-survey]]"
  - "[[architecture/monologue-streams]]"
  - "[[architecture/companion-system]]"
---

# LLM role-play literature — comprehensive report

**Corpus:** 39 papers archived as PDFs under [`raw/papers/llm-roleplay/`](../../raw/papers/llm-roleplay/) (extracted text in `txt/`).  
**Lens:** Rhapsode’s bet — one **frontier monolithic LLM** (API) + **scaffolding** (CharacterCore, monologue streams, narrator/actor split, memory) rather than per-character fine-tunes.

## Bottom line

The field’s **center of mass** is “make one chatbot more like one character,” usually by **training weights**. The systems that look like a **game engine** are almost all **frozen LLM + director/memory/RAG**.

That supports the Rhapsode bet **if scaffolding stays thick**. It does **not** support “API + thin `You are Megumin` prompt beats LoRA.” On weak baselines and small open models, weight training wins. On multi-cast interactive fiction with a frontier model, the literature’s best friends are director–actor loops, thought-tagged literary schemas, persona-aware memory, and staged style/content pipelines — not N LoRAs.

---

## What’s in the archive

| Group | Count (approx) | Examples |
|-------|----------------|----------|
| Weight identity (SFT / LoRA / RL) | ~14 | Character-LLM, CoSER, HER, Neeko, Character-R1, HumanLLM |
| Frozen scaffolding (prompt / director / stages) | ~10 | IBSEN, BookWorld, TTM, PersonaForge, ChatHaruhi, Amadeus |
| Memory / RAG governance | ~8 | ARPM, DualMem, RoleRAG, MemGPT, MemoryBank, RMM |
| Steering / decoding (needs open weights) | ~4 | CAST, PERSONA, RepE, PDD |
| Surveys / eval / datasets | ~8 | Two Tales, Oscars, RoleBench/RoleLLM, CharacterBox, PersonaGym |

PDFs: `raw/papers/llm-roleplay/*.pdf` · Manifest: `manifest.json` · Text extracts: `txt/`.

---

## How to read the field (one map)

```
                    ┌─────────────────────────────┐
                    │   Who owns the character?   │
                    └──────────────┬──────────────┘
           ┌───────────────────────┼───────────────────────┐
           ▼                       ▼                       ▼
    PARAMETERS               CONTEXT STACK            ACTIVATIONS
    (SFT/LoRA/RL)            (prompt+RAG+director)    (steering)
           │                       │                       │
    Character-LLM            IBSEN / BookWorld        CAST / PERSONA
    CoSER / HER / Neeko      TTM / ChatHaruhi         RepE / PDD
    Character-R1             Amadeus / DualMem
                             Codified Profiles
```

**Rhapsode lives in the middle column**, with optional later distillation of scaffolding outputs into weights (not the other way around).

Two surveys keep the jargon straight:

- **Two Tales of Persona** — *role-playing* (model plays a character) vs *personalization* (model adapts to a user). Don’t mix those metrics.
- **From Persona to Personalization** / **Oscars of AI Theater** — taxonomies of data, methods, and eval dimensions; warn that fanfic pollutes style.

---

## What supports the monolith + scaffolding bet

| Claim from papers | Why it helps Rhapsode |
|-------------------|------------------------|
| Director + actors beat one blob prompt for controllable drama (**IBSEN**) | Matches narrator vs cast |
| Book-world multi-agent + world agent (**BookWorld**) | Closest “fictional society” cousin |
| Script/line memory without FT (**ChatHaruhi**) | Anime voice via retrieval |
| Decouple personality / memory / style at test time (**TTM**) | Staged actor pipeline |
| Inner monologue under stress, not every turn (**PersonaForge**) | Monologue stream gating |
| RAG + attribute inference when canon is silent (**Amadeus**) | Core + lore retrieval |
| Persona-agnostic summaries kill fidelity (**DualMem / RoleMemo**) | Fact vs character insight |
| Knowledge-scope / OOK refusal (**RoleRAG**) | Isekai ignorance boundaries |
| Executable character laws (**Codified Profiles**) | Hard invariants in cores |
| Base model strength dominates SFT volume (**DITTO**, various) | Buy frontier; engineer structure |
| Literary speech + **thoughts** (**CoSER**, **HER** schema) | Monologue + speech contract |

## What contradicts or complicates it

| Counter-evidence | Honest take |
|------------------|-------------|
| Character-LLM / CharacterGLM / CoSER-FT / HER / Character-R1 beat naive prompts on their benches | True — vs thin prompts on smaller models. Not vs thick scaffolding on DeepSeek-class APIs. |
| Neeko multi-LoRA serves many cast members from one runtime | Real alternative **if** you leave API-monolith for local weights. |
| Open Character Training: constitution + DPO beats prompt/steering for *assistant* persona | More relevant to **narrator** identity than to Megumin. |
| Activation steering matches SFT on Big Five benches | Measures traits, not literary idiolect; needs open weights. |
| HumanLLM: isolated trait knobs ≠ human cognitive patterns | Don’t replace cores with OCEAN sliders. |

**Net formulation that survives the literature:**

> Frontier model + thick literary scaffolding ≥ per-character FT for multi-cast interactive fiction.  
> FT remains optional later as distillation — not as the identity store.

---

## Closest cousins to Rhapsode’s design

| Rhapsode piece | Closest papers | Still missing in literature |
|----------------|----------------|------------------------------|
| Narrator / director | **IBSEN**, BookWorld world agent | Live party RPG + graph validation |
| CharacterCore | Codified Profiles, PersonaForge layers, ChatHaruhi profiles | Versioned engine state, rare revision |
| Monologue streams | HER dual-layer, CoSER thoughts, PersonaForge inner monologue | Persistent forked streams across turns |
| Belief / memory | DualMem, RoleRAG, ARPM, Generative Agents | Shared world + private false beliefs as first-class |
| Multi-cast party | BookWorld, IBSEN actors, Neeko (weights) | Ensemble comedy timing / banter choreography |

---

## Steal list (ranked for Rhapsode)

### Adopt in scaffolding (now)

1. **IBSEN director loop** — narrator owns objectives/beats; actors get constrained instructions; character DB ≠ episodic memory.
2. **CoSER / HER message schema** — thought → speech/action; keep planner diction out of character voice.
3. **DualMem writes** — every long-term note is `fact` vs `insight_from(character)`; never neutral-only RP summaries.
4. **RoleRAG knowledge boundary** — explicit in-character ignorance for out-of-scope entities.
5. **TTM staging** — intention/content → memory bind → linguistic style (catchphrase bank).
6. **Amadeus retrieval** — hierarchical lore + infer traits when facts are missing; don’t dump irrelevant canon.
7. **Codified invariants** — hard laws as pre/post checks (“won’t apologize,” “explosion first”).
8. **PersonaForge gating** — deep monologue updates on stress/OOC risk, not every trivial beat.
9. **ARPM-style logging** — white-box “why was this retrieved / said?” for debugging.

### Adopt for eval / data hygiene

10. Trajectory eval (**CharacterBox**) + env-shift suites (**PersonaGym**) + style-vs-knowledge split (**DITTO / RoleBench**).
11. Prefer primary text / CoSER-like authentic excerpts over fanfic (**Oscars** warning).

### Avoid / defer

- Per-character full finetunes as the identity store.
- Multi-LoRA casts unless you abandon API-monolith.
- Big Five / activation steering as a substitute for literary cores.
- Fanfiction-heavy SFT; toy forgetting that erases canon.
- Economic multi-agent routing papers (e.g. AgentSociety) — wrong problem.

### Watch

- HER / Character-R1 / RAR open models as optional **actor backends** under your narrator.
- Open Character Training for **narrator** constitutions if narrator drift hurts.
- PDD / PERSONA only if you self-host with logits/activations.
- Memento-style case memory for **engine/narrator skills**, not cast voice.

---

## Gaps for a Konosuba-style literary party RPG

Almost nothing in this corpus optimizes the actual product surface:

1. **Ensemble comedy timing** — interrupts, escalation, deadpan vs loud; evals are mostly single-agent or pairwise story wins.
2. **Idiolect under plot pressure** — trademark rhythm without becoming a parrot.
3. **Asymmetric competence comedy** — useless goddess / explosion maniac / crusader masochist; OCEAN flattens this.
4. **Live literary narrator + diegetic banter** — BookWorld rephrases after sim; few co-generate stage prose and banter together.
5. **Shared world graph + private minds** — lies, dramatic irony, belief privacy.
6. **Long-arc gag / grudge memory** — general memory papers help; gag structure does not.
7. **Player as party member** — role-play vs personalization still under-explored in cast sims.
8. **Canon governance as engineering** — retrieval policy for copyrighted lore, not just ethics footnotes.

---

## Paper catalog (all 39)

One line each. Train? = whether character identity is baked into weights. Relevance 0–3 for Rhapsode’s current bet.

### Surveys & maps

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| Two Tales of Persona | — | 2 | Separates role-play vs personalization literatures. |
| From Persona to Personalization | — | 2 | Broad RPLA taxonomy (prompt / parametric / memory). |
| Oscars of AI Theater | — | 2 | Role-play survey; fanfic OOC warning. |

### Frozen scaffolding (best friends)

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| IBSEN | No | 3 | Director–actor controllable interactive drama. |
| BookWorld | No | 3 | Novel → role agents + world agent → story. |
| TTM | No | 3 | Test-time split: personality / memory / style. |
| PersonaForge | No | 3 | Psych layers + selective inner monologue. |
| ChatHaruhi | Hybrid | 3 | Script-memory anime RP; FT optional, often unnecessary. |
| Amadeus (CharacterRAG) | No | 3 | Training-free RAG + attribute inference for OOK queries. |
| Codified Profiles | Hybrid | 3 | Character logic as executable if/then, not only prose. |
| Generative Agents | No | 2 | Memory stream / reflect / plan — scaffolding ancestor. |
| Humanoid Agents | No | 1 | GA + System-1 drives; weak literary fit. |
| GenAgents (1,000 people) | No | 1 | Interview-grounded human simulacra (social science). |
| PCL | Yes* | 2 | Contrastive self-questioning; *steal the chain as prompts. |
| PDD | No† | 1 | Decoding-time persona following; †needs white-box access. |

### Memory / RAG governance

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| DualMem / RoleMemo | Hybrid | 3 | Fact stream + persona insight stream; agnostic summaries fail. |
| RoleRAG | No | 3 | Graph-guided retrieval + scope-aware ignorance. |
| ARPM | No | 3 | Temporal dual memory + analysis-before-answer binding. |
| MemGPT | No | 2 | OS-style working / recall / archival memory. |
| MemoryBank | Hybrid | 2 | Companion long-term memory + forgetting curve. |
| RMM | Hybrid | 2 | Reflective retrieval refinement for long dialogue. |
| Memento / AgentFly | No | 1 | Case-based agent memory without LLM FT (tools, not cast). |

### Weight identity (counter-thesis / schema sources)

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| Character-LLM | Yes | 2 | Per-char SFT via experience reconstruction. |
| RoleLLM / RoleBench | Hybrid | 2 | Bench + RoleGPT prompts + RoCIT LoRA. |
| DITTO | Yes | 2 | Self-alignment SFT; base model dominates. |
| CoSER | Yes | 3 | Literary dataset + speech/action/**thought**; gold schema/data. |
| CharacterGLM | Yes | 1 | Product Chinese character models. |
| Neeko | Yes | 1 | Dynamic multi-character LoRA routing. |
| Open Character Training | Yes | 1 | Constitution→DPO for *assistant* personas. |
| HER | Yes | 3 | Dual-layer system/role thinking + RL — steal schema, not weights. |
| HumanLLM | Yes | 2 | 244 interacting cognitive patterns; warns against trait isolation. |
| Character-R1 | Yes | 2 | RL with verifiable cognitive-focus rewards. |
| RAR (Thinking in Character) | Yes | 2 | Distill in-character reasoning style into LRMs. |

### Steering / representation

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| Representation Engineering | Hybrid | 0–1 | Parent of activation control; not literary RP. |
| CAST | No† | 1 | Conditional activation steering (ICLR’25). |
| PERSONA (steering) | No† | 1 | Big Five vector algebra at inference. |

### Eval & sandboxes

| Paper | Train? | Rel | One-liner |
|-------|--------|-----|-----------|
| CharacterBox | Hybrid | 2 | Trajectory sandbox for role fidelity over arcs. |
| PersonaGym | — | 2 | Dynamic env-conditioned persona eval. |

### Out of scope / low relevance

| Paper | Rel | Note |
|-------|-----|------|
| AgentSociety | 0 | Economic / routing multi-agent incentives — not literary RP. |

---

## How this should change Rhapsode (practical)

If you only take five moves from thirty-nine papers:

1. Treat the **narrator as IBSEN’s director** (objectives, beat constraints) and characters as actors with **separate** identity vs episodic memory.
2. Make every actor call a **TTM-ish pipeline**: content from core+scene → bind retrieved memory → apply style bank.
3. Persist DualMem-style **fact vs insight**; never only “neutral summary.”
4. Enforce **knowledge boundaries** (RoleRAG) so isekai casts don’t leak Earth memes.
5. Gate expensive monologues (**PersonaForge**) and log retrieval (**ARPM**) so failures are debuggable.

Do **not** start a per-character LoRA farm unless the API+scaffolding path fails after those five are real.

---

## Archive & provenance

| Path | Contents |
|------|----------|
| [`raw/papers/llm-roleplay/`](../../raw/papers/llm-roleplay/) | PDFs + `manifest.json` + `download_pdfs.py` |
| `raw/papers/llm-roleplay/txt/` | Extracted text (capped ~120k chars/paper) |
| `raw/papers/llm-roleplay/txt/digests/` | Structured excerpts used for this report |
| [`wiki/research/llm-roleplay-survey.md`](llm-roleplay-survey.md) | Earlier curated survey (weight-centric core set) |

This report covers the **survey set plus later scaffolding finds** (TTM, Amadeus, PersonaForge, BookWorld, DualMem, ARPM, etc.). Older companion-system notes that assume L3 LoRA identity as default are **stale** relative to this read — see [monologue-streams](../architecture/monologue-streams.md) as design of record.
