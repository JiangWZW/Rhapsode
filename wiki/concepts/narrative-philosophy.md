---
sources:
  - core/include/rhapsode/director.h
  - core/include/rhapsode/memory_system.h
  - core/include/rhapsode/node.h
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[concepts/rhapsode-overview]]"
  - "[[architecture/system-overview]]"
  - "[[architecture/plot-graph]]"
  - "[[architecture/memory-system]]"
  - "[[research/literature-review]]"
tags:
  - design
---

# Narrative philosophy

The foundational design beliefs that shape every technical decision in Rhapsode. These are not features to implement — they are constraints on *how* we think about features.

## The six principles

### 1. The Director is a rhapsode — an arranger, not a puppeteer

The project name is the architectural thesis.

A *rhapsode* in ancient Greece did not invent stories. The myths already existed as oral tradition — fragments passed down through generations. The rhapsode's art was in the **arrangement**: which episodes to tell, in what order, how to pace the tension, when to linger, when to skip ahead.

The Director works the same way:

| Role | Who |
|------|-----|
| **Composer** | The LLM — generates raw dramatic material — character secrets, conflicts, forces in motion |
| **Arranger / Rhapsode** | The Director — parses raw material into nodes, manages timing and pacing, decides when threads surface |
| **Performer** | The LLM again — renders the arranged structure into prose for the player |
| **Audience and co-composer** | The player — their actions feed back into the next composition |

**Status:** The Director is implemented. Each turn, it builds a JSON prompt from non-resolved nodes plus scene context, calls the LLM, and parses the response into state transitions and new nodes. It collects foreshadow and active context strings for the narrative prompt. See [[architecture/system-overview|system overview]].

### 2. Long-term memory is the emotional backbone

What makes a story *yours* is accumulated experience. When a character you have spent 20 hours with betrays you, the arc shape does not matter — the *memory* does.

The memory system is not a technical feature (RAG retrieval). It is the emotional backbone:

- **What happened** — facts distilled from plot nodes
- **What it means** — quality scores, entity links, relationship context
- **Who knows** — `known_by` metadata per fact for information asymmetry

**Status:** Implemented. The MemorySystem stores facts in Chroma with embeddings and supports hybrid retrieval: semantic similarity, BM25 keyword matching, and entity boosting. A local LLM runs the quality pipeline. See [[architecture/memory-system|memory system]].

### 3. The Hamlet quality — ambiguity as depth

Vonnegut called out Hamlet as the story where "we don't know whether the news is good or bad." That ambiguity separates good stories from great ones. Interpretation depends on what happens next.

For Rhapsode:

- There is no fortune tracker. The arc is not a number.
- The arc **emerges** from accumulated memory + active plot nodes.
- The world presents situations that *could* be good or bad. The player's response determines which world becomes real.

**Why no fortune tracker:** (1) Story arcs are not one-dimensional — winning a battle while losing a relationship cannot be a single float. (2) Tracking the arc creates a circular dependency if the LLM both sets and reads it. (3) The graph and memory already carry dramatic weight implicitly.

### 4. The LLM is a world simulator, not a storyteller

| Storyteller | Simulator |
|-------------|-----------|
| "What would make a good story here?" | "Given these characters, rules, history, and this action — what would happen?" |
| Prone to cliches, deus ex machina | Emergent, surprising, real-feeling |
| Decides the plot | Discovers the plot by following the rules |

The Director provides *arrangement* — which nodes are active, what context to inject. The memory provides *state* — what happened, who knows what. The LLM renders the world within those boundaries.

The LLM also has a second role: **composing raw material**. Before the Director can arrange anything, the LLM generates the dramatic landscape — what secrets exist, what conflicts are latent, what forces are in motion. This happens at scenario initialization and as new nodes are created during play.

**Status:** Implemented. The Director uses one LLM call per turn for node management. The narrative LLM call receives Director context blocks and memory-retrieved facts, constraining the response without controlling it.

### 5. The player breaks everything (and that is the point)

| Approach | How | Result |
|----------|-----|--------|
| **Railroad** | Force the player back onto the arc | Players feel powerless, quit |
| **Sandbox** | No arc at all, just simulate | Boring after a while, no payoff |
| **Elastic arc** | Story has a *tendency* toward a shape; the player can stretch it | Meaningful agency with narrative coherence |

The elastic arc works by steering the *world's response*, not the player's actions. The player has agency over what they do. The Director has agency over the consequences.

### 6. The interface is part of the dramaturgy — read actions vs. write actions

Not all player actions are equal. The distinction is whether the action mutates the plot graph.

| | Read action | Write action |
|---|---|---|
| **What** | Observe, talk, explore | Decide, commit, act irreversibly |
| **Graph effect** | None — the player gathers information | Transitions a node, chooses an edge |
| **Input mode** | Freeform text | Constrained choices |
| **Who handles it** | LLM simulates freely | Director presents curated options |

The input mode spectrum: freeform → guided freeform → constrained choice → forced progression. The Director decides the mode based on graph position; the frontend renders whatever mode the backend sends.

**Status:** Not yet implemented. Current UI is freeform-only. The constrained choice system requires the full DAG with edges, which is future work.

## Lessons from Talemate

Rhapsode draws from analysis of the [Talemate][1] codebase.

### Carry forward

- **Separation of narrative structure from prose generation.** Director thinks about arrangement; the LLM thinks about prose.
- **Layered context.** Raw recent dialogue + summarized past + retrieved relevant memories.
- **Canonical fact reinforcement.** Small facts that stay alive without relying solely on retrieval.
- **Per-character instructions.** Private stage directions per character.

### Avoid

- **Director as god-object.** Talemate's Director accumulated mixins for every narrative concern.
- **LLM-driven structural decisions at runtime.** The LLM composes material; the Director structures it.
- **Equal-weight memories.** A betrayal after 20 hours should weigh more than small talk 5 turns ago.
- **Over-engineered memory pipelines.** The operation is fundamentally: "what does the world remember that is relevant here?"

## References

- [Vonnegut shapes of stories][2]
- [GRRM on outlines][3]
- [GRRM architects vs gardeners][4]
- [[research/literature-review|literature review]] — 8 papers surveyed

[1]: https://github.com/vegu-ai/talemate
[2]: https://storytellingedge.substack.com/p/the-simple-shapes-of-great-stories
[3]: https://www.youtube.com/watch?v=XF1PyB5v9jI
[4]: https://www.youtube.com/watch?v=nK6VoL76r3Q
