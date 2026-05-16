---
sources:
  - core/include/rhapsode/node.h
  - core/include/rhapsode/node_pool.h
  - core/src/node.cpp
  - core/src/node_pool.cpp
  - core/include/rhapsode/director.h
  - core/src/director.cpp
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/memory-system]]"
  - "[[concepts/narrative-philosophy]]"
  - "[[research/literature-review]]"
tags:
  - cpp-core
  - design
---

# Plot graph

The plot graph is Rhapsode's core narrative data structure. It tracks **latent facts about the world** — secrets, conflicts, ticking clocks — that the Director manages across turns.

This page covers both the **implemented** system (Node/NodePool) and the **planned** full DAG architecture.

## Current implementation: Node and NodePool

### What is a node?

A node is a fact about the world that could become dramatically relevant. It sits in a state and the Director transitions it based on LLM analysis.

Examples of node facts:

- "barkeep owes thieves guild 200g"
- "knight drinks alone at tavern every night"
- "village well has been poisoned"

### Node structure

```cpp
struct Node {
    uint64_t id;
    std::string fact;              // atomic assertion
    std::string type;              // "plot", "scene", "world", "relationship"
    NodeState state;               // Dormant, Foreshadowed, Active, Resolved
    std::string foreshadow_ctx;    // hint for prompt when foreshadowed
    std::string active_ctx;        // directive for prompt when active
    std::vector<std::string> entities;   // named entities
    std::vector<std::string> known_by;   // who knows about this
    int created_at;                // turn of creation
    int resolved_at;               // turn of resolution (-1 if unresolved)
};
```

### Node states

States form a one-way progression:

| State | Meaning | Prompt effect |
|-------|---------|---------------|
| **Dormant** | Known to the system, not yet surfaced | None — invisible to narrative |
| **Foreshadowed** | Hints being seeded | `foreshadow_ctx` injected as subtle context |
| **Active** | Directly relevant to current scene | `active_ctx` injected as explicit directive |
| **Resolved** | Completed, consequences applied | Removed from pool after processing |

The Director transitions nodes each turn by calling the LLM with a JSON prompt listing all non-resolved nodes and the current scene context. The LLM returns requested transitions and new nodes.

### NodePool

The pool is a flat `unordered_map<uint64_t, Node>` with indexed access:

- `by_state(s)` — all nodes in a given state
- `by_entity(e)` — all nodes mentioning an entity
- `by_known_by(who)` — all nodes known by a character
- `wavefront()` — all Active nodes

Resolved nodes are removed from the pool after the Director processes them. Their facts are stored in the memory system for long-term retrieval.

### Scenario seed nodes

Scenarios can include seed nodes in the JSON file:

```json
{
  "nodes": [
    {
      "fact": "The barkeep owes money to the thieves guild.",
      "type": "plot",
      "state": "Foreshadowed",
      "foreshadow_ctx": "The barkeep keeps glancing nervously at the door.",
      "active_ctx": "The barkeep is frightened; debt collectors are expected soon.",
      "entities": ["barkeep", "guild"],
      "known_by": ["barkeep"]
    }
  ]
}
```

These provide initial dramatic content before the Director generates new nodes during play.

### Director's node management

Each turn, the Director:

1. Serializes all non-resolved nodes to JSON
2. Includes scene context (system prompt, characters)
3. Retrieves established facts from memory to prevent contradictions
4. Calls the LLM with a strict system prompt enforcing atomic fact format
5. Parses the response for `transitions` (state changes) and `new_nodes`
6. Applies transitions and adds new nodes to the pool
7. Removes resolved nodes
8. Collects context blocks from surviving foreshadowed/active nodes

The LLM response format:

```json
{
  "transitions": [{"id": 3, "state": "active"}],
  "new_nodes": [{"fact": "guild sends collector to tavern", "type": "plot", "state": "dormant", ...}]
}
```

## What is not built (the planned DAG)

The current NodePool is a **flat collection** — nodes exist independently without edges connecting them. The full vision adds edges with trigger predicates, forming a directed acyclic graph.

### Planned edge structure

```
Edge {
    source:    node_id
    target:    node_id
    trigger:   Trigger
}

Trigger = one of:
    PlayerAction { tags: list[string] }
    TurnCount    { threshold: int }
    WorldCondition { predicate: string }
    NodeState { node_id: string, state: enum }
```

With edges, the Director traverses the graph deterministically each turn — checking which edge triggers are satisfied and firing transitions without an LLM call. The LLM would only be involved in the generation pipeline (creating new subgraphs) and the narrative rendering.

### Planned features requiring the DAG

**Trigger-based transitions.** Instead of asking the LLM whether a node should transition, edges carry predicate conditions that the Director evaluates mechanically.

**Auto-merging.** When two active nodes share characters or locations, the Director spawns a merge node combining their contexts.

**Revert.** A transition log enables undoing recent transitions and replaying from an earlier state.

**Player traversal model.** Players are "on an edge" (freeform input) until they arrive "at a node" (constrained choices). The graph position determines the input mode.

**World-background loop.** Off-screen plot nodes advance between player turns on a timer or turn counter. NPCs act on their goals independently.

**Multi-dimensional problem.** Multiple concurrent threads mean the player may have several plot lines active simultaneously. The Director serializes, merges, or queues decision points.

### Foreshadow-Trigger-Payoff triples

Informed by CFPG; see [[research/literature-review]]. Each plot node maps to an (F, T, P) triple:

- **F (Foreshadow)** — the setup or anomaly creating a "causal debt." Injected via `foreshadow_ctx`.
- **T (Trigger)** — the prerequisite condition before payoff becomes actionable. The edge trigger predicate.
- **P (Payoff)** — the resolution event. Injected via `active_ctx` when the node transitions to Active.

This maps to the implemented state machine:

| State | F-T-P status |
|-------|-------------|
| Dormant | F registered, T not checked, P suppressed |
| Foreshadowed | F injected as hint, T monitored, P suppressed |
| Active | T satisfied, P injected as constraint |
| Resolved | P realized, triple removed from active set |

### Knowledge state

Each node carries `known_by` — which characters know about this fact. The Director can use this for revelation timing:

- **Dramatic irony**, where the player knows but the character does not — creates dread
- **Surprise**, where the character knows but the player does not — creates shock
- **Default policy:** prefer dramatic irony — Xie & Riedl (2024) validated this empirically for suspense

### The generation pipeline (planned)

The LLM composes raw dramatic material. The Director extracts it into graph nodes.

```
World state + memory + current graph
        ↓
    LLM (free text, unconstrained)
        ↓
    Director / extraction pass → structured nodes
        ↓
    Plot graph (updated with new dormant nodes)
```

Three triggers for generation:

| Trigger | When |
|---------|------|
| Scenario initialization | Once, before the player starts |
| Periodic world-building | Every N turns |
| Reactive spawning | After a major node resolves |

### Five rules for interesting worlds (planned)

1. **Minimum node floor.** Maintain a minimum count of active + foreshadowed nodes. When it drops, fire the generation pipeline.
2. **Timescale balance.** Active nodes should span immediate (this turn), short-term (5-10 turns), and long-term (30+ turns) horizons.
3. **NPC autonomy.** NPCs have goals and act on them off-screen. The player encounters them mid-action.
4. **Disproportionate consequences.** Small player actions cascade into unexpected outcomes.
5. **Reputation propagation.** Player actions spread through NPC awareness via memory.

## Open questions

1. **Trigger language.** How to express "player befriends barkeep"? Keyword matching? LLM classification? A dedicated model?
2. **Extraction robustness.** How to handle ambiguous or inconsistent LLM output during generation? Retry? Partial extraction?
3. **Multi-dimensional arrival.** When the player reaches decision points in multiple threads simultaneously, how should the Director prioritize?
4. **Freeform edge-breaking.** What happens when a freeform action invalidates the destination node?
5. **Visual editor.** The plot graph is the natural candidate for a node inspection/editing tool.

## References

- [system overview](system-overview.md) — how the Director uses the node pool
- [memory system](memory-system.md) — where resolved node facts are stored
- [[concepts/narrative-philosophy]] — the design principles behind the graph
- [[research/literature-review]] — CFPG, IBSEN, StoryVerse, and others
