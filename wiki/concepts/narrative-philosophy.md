# Narrative philosophy

The foundational design beliefs that shape every technical decision in Rhapsode. These are not features to implement -- they are constraints on *how* we think about features.

## The five principles

### 1. The Director is a rhapsode -- an arranger, not a puppeteer

The project name is the architectural thesis.

A *rhapsode* in ancient Greece didn't invent the stories. The myths, the characters, the battles already existed as oral tradition -- fragments passed down through generations. The rhapsode's art was in the **arrangement**: which episodes to tell, in what order, how to pace the tension, when to linger, when to skip ahead. Homer didn't create Achilles or Troy. He arranged the fragments into the Iliad.

The Director works the same way:

| Role | Who |
|---|---|
| **Composer** | The LLM -- generates raw dramatic material (character secrets, conflicts, forces in motion) as free text |
| **Arranger / Rhapsode** | The Director -- parses the raw material into the plot graph, manages timing and pacing, decides when plot nodes surface |
| **Performer** | The LLM again -- renders the arranged structure into prose for the player |
| **Audience and co-composer** | The player -- their actions feed back into the next composition |

Talemate's Director was a meta-controller: a separate LLM call each turn asking "what should happen?" It became a god-object with no structural model of story. The LLM was making structural decisions through a middleman.

Rhapsode's Director doesn't ask the LLM what should happen. It takes what the LLM has already imagined and **arranges** it -- structures it into a graph, tracks timing, manages which threads surface when, ensures the whole hangs together.

### 2. Long-term memory is the emotional backbone

Vonnegut identified ~8 story shapes. Researchers confirmed this with NLP across 2,000 novels. The shapes are few, but what makes a story *yours* is accumulated experience. When a character you've spent 20 hours with betrays you, the arc shape doesn't matter -- the *memory* does.

This means the memory system is not a technical feature (RAG retrieval). It is the emotional backbone. The system needs to remember:

- **What happened** -- event log, summarized history
- **What it meant** -- emotional tags, significance scores
- **What changed** -- character relationship deltas, world state mutations

Talemate's ChromaDB approach is correct in principle: layered context (raw recent dialogue + summarized past + RAG-retrieved relevant memories) with token budgeting. But it treats all memories as equal text chunks. A better system has *weighted* memories -- some moments matter more than others, and the system should know which ones.

Talemate's "reinforcements" (periodic Q&A refreshes that keep canonical facts alive) are also a good idea worth carrying forward in simpler form.

### 3. The Hamlet quality -- ambiguity as depth

Vonnegut called out Hamlet as the story where "we don't know whether the news is good or bad." The ghost might be salvation or damnation. The audience cannot tell if fortune is rising or falling. That ambiguity -- where interpretation genuinely depends on what happens next -- is what separates good stories from great literature. It is also the shape of real life.

For Rhapsode this means:

- There is no fortune tracker. The arc is not a number.
- The arc **emerges** from accumulated memory + active plot nodes. The player looks back and sees a shape, but it was never computed.
- The world should present situations that *could* be good or bad. The player's interpretation and response determines which world becomes real.

This is why the LLM-as-simulator framing matters: a simulator doesn't decide if the ghost is good or evil. It computes the consequences and lets the player's actions determine the outcome.

Reference: [The Simple Shapes of Great Stories, According to Kurt Vonnegut (And Science)](https://storytellingedge.substack.com/p/the-simple-shapes-of-great-stories)

### 4. The LLM is a world simulator, not a storyteller

The critical distinction:

| Storyteller | Simulator |
|---|---|
| "What would make a good story here?" | "Given these characters, rules, history, and player action -- what would happen?" |
| Prone to cliches, deus ex machina, forced drama | Emergent, surprising, real-feeling |
| Decides the plot | Discovers the plot by following the rules |

The Director provides *arrangement* (which plot nodes are active, what context to inject). The memory system provides *state* (what happened, who knows what). The LLM renders the world within those boundaries. It doesn't decide the plot; it computes the next moment.

But the LLM also has a second role: **composing the raw material**. Before the Director can arrange anything, the LLM generates the dramatic landscape -- what secrets exist, what conflicts are latent, what forces are in motion. This happens at scenario initialization, periodically between turns, and reactively after major events. See [[plot-graph#The generation pipeline]].

### 5. The player breaks everything (and that's the point)

The player will break the arc. That is the fundamental tension of interactive narrative. Three approaches exist:

| Approach | How | Result |
|---|---|---|
| **Railroad** | Force the player back onto the arc | Players feel powerless, quit |
| **Sandbox** | No arc at all, just simulate | Boring after a while, no payoff |
| **Elastic arc** | Story has a *tendency* toward a shape; the player can stretch it | Meaningful agency with narrative coherence |

The elastic arc works by steering the *world's response*, not the player's actions. The player has agency over what they do. The Director has agency over the consequences.

- The player burns the tavern? The city watch comes.
- The quest-giver dies? Someone else picks up the thread.
- The player refuses the call to adventure? The adventure comes to them, in a different form.

The world pushes back not by blocking the player, but by introducing consequences that naturally bend the story. The more the player deviates, the more interesting the consequences become.

### 6. The interface is part of the dramaturgy — read actions vs. write actions

Not all player actions are created equal. The distinction is not "big vs. small" but **whether the action mutates the plot graph**.

| | Read action | Write action |
|---|---|---|
| **What** | Observe, talk, explore, investigate | Decide, commit, act irreversibly |
| **Graph effect** | None — the player gathers information | Transitions a plot node, chooses an edge |
| **Input mode** | Freeform text | Constrained choices (adaptive count) |
| **Who handles it** | LLM simulates freely | Director presents curated options |

The player is always **on an edge** of the plot graph, traveling toward a plot node. While on that edge, freeform input is fine — the player chats, explores, builds relationships. Nothing they do changes the destination.

The mode shift to constrained choices happens **at nodes** — when the player arrives at a plot node and must choose which outgoing edge to take next. This is a visual-novel moment embedded in a freeform RPG. The UI change itself signals weight: "this decision matters."

The shift between freeform and constrained input is itself a storytelling device. When the text box disappears and buttons appear, the player *feels* the gravity of the moment before reading a single word.

#### Input mode spectrum

The system supports a spectrum, not a binary:

| Mode | When | UI |
|---|---|---|
| **Freeform** | On an edge, exploring | Text input |
| **Guided freeform** | Approaching a node, soft nudge | Text input + suggested action chips |
| **Constrained choice** | At a node, write action required | N buttons, no text input |
| **Forced progression** | Climax, cutscene-equivalent | Single "Continue" button |

The Director decides the mode based on the player's position in the graph and the plot node state. The frontend renders whatever input mode the backend sends.

#### What the plot graph tracks vs. doesn't

| Tracked by plot graph | NOT tracked |
|---|---|
| Which plot node the player is traveling toward | Moment-to-moment dialogue |
| Which choice they made at the last node | Combat, exploration paths |
| Plot node states (dormant/foreshadowed/active/resolved) | Freeform roleplay, flavor |
| Structural edges between plot nodes | World simulation details |

The memory system handles the "not tracked" column. The plot graph only cares about the structural skeleton.

#### Talemate comparison

Talemate's "Dynamic Actions" are floating suggestion chips generated by a random probability roll (`generate_choices_chance`) on each `player_turn_start` signal. The player can always ignore them and type freely. There is no mechanism to constrain input based on narrative weight — the choices are suggestions, never requirements.

Rhapsode's constrained choices are structurally different: they are **mandatory at graph nodes**, driven by plot state rather than randomness, and each option maps to a specific graph edge transition with designed consequences.

---

## Lessons from Talemate

### What to carry forward

- **Separation of narrative structure from prose generation.** The Director thinks about arrangement; the LLM thinks about prose.
- **Layered context.** Raw recent dialogue + summarized older history + retrieved relevant memories. This is the right architecture.
- **Canonical fact reinforcement.** Small, periodically refreshed facts (character traits, world rules) that stay alive without relying solely on retrieval.
- **Per-character instructions.** The idea of giving each character private stage directions is powerful.

### What to avoid

- **Director as god-object.** Talemate's Director accumulated mixins for every narrative concern. Keep responsibilities small and separate.
- **LLM-driven structural decisions at runtime.** The LLM composes raw material; the Director structures it. The LLM never traverses the graph or decides timing.
- **Equal-weight memories.** Not all moments matter equally. A betrayal after 20 hours should weigh more than what the barkeep said 5 turns ago.
- **Over-engineered memory pipelines.** Multiple backends, AI-assisted query generation, placement modes. The operation is fundamentally simple: "what does the world remember that's relevant right now?"

---

## The plot graph

These principles led to a concrete architecture: the **plot graph**. See [[plot-graph]] for the full design.

In brief: the plot graph is a DAG of latent plot nodes (secrets, ticking clocks, forces in motion). The LLM generates the raw dramatic material as free text; the Director extracts and structures it into graph nodes. The Director traverses the graph deterministically each turn, checking trigger conditions and injecting foreshadowing context. The LLM never sees the graph -- it sees only the prompt context the Director extracts.

The Director also runs a **world-background loop** -- advancing off-screen plot nodes between player turns so the world lives and breathes independently of the player's attention.

## Why there is no fortune tracker

Early design considered a fortune tracker (a float from -1 to +1 representing the player's dramatic fortune). This was rejected for three reasons:

1. **Story arcs are not one-dimensional.** A character can simultaneously be winning a battle and losing a relationship. Reducing this to a number is reductive.
2. **The arc should emerge, not be tracked.** If the world has rich enough state (active plot nodes, weighted memories, consequences), the arc appears naturally. You don't need a number to tell the LLM "things are bad" -- the memories and world state already encode that.
3. **Tracking the arc creates a circular dependency.** The fortune tracker is supposed to constrain the LLM, but if the LLM also sets it (via sentiment analysis), the system is chasing its own tail. Rule-based triggers from the graph would work mechanically but add authoring burden for little gain -- the graph itself already carries the dramatic weight.

The Director's job is not "track the arc" but **"ensure the world has active forces."** It checks: are there active plot nodes? Are new ones being seeded? Has any plot node line gone stale? If the world is too quiet, the Director generates new material. The arc emerges from the interaction between the player's choices, the world's consequences, and accumulated memory.

## Open questions (for future design sessions)

1. What is the right data structure for weighted memories? A tagged event log with decay? An explicit significance score at write time?
2. **Trigger language**: how do we express conditions like "player befriends barkeep"? Keyword matching? LLM classification?
3. Where is the line between "Director context injection" and "railroading"? How much steering is too much?
4. How should the extraction step work in detail? What happens when extraction fails or produces inconsistent nodes?
5. How do plot node templates (reusable patterns like "character has a secret") interact with LLM-generated raw material?
