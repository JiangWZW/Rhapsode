# Plot graph

The plot graph is the core narrative data structure in Rhapsode. It is a **directed acyclic graph of latent tensions** that the Director traverses deterministically, while the LLM renders the results into prose.

## What is a tension?

A tension is a fact about the world that *could* become dramatically relevant. It sits dormant until a trigger fires.

Examples:

- "The barkeep owes money to the thieves' guild"
- "The knight is secretly the missing prince"
- "The village well has been poisoned"

A tension is not a plot beat. It is a **loaded spring**. The player's actions (or the world clock) release it.

## The graph

Tensions are nodes. Edges are triggers. The Director walks the graph each turn.

```mermaid
graph TD
    T1["barkeep_debt"] -->|"player befriends barkeep"| T1a["barkeep confides"]
    T1 -->|"10 turns pass"| T1b["collector arrives"]
    T1b -->|"player present"| T1c["confrontation scene"]
    T1b -->|"player absent"| T1d["barkeep beaten"]
    T1a -->|"player offers help"| T1e["player vs guild"]
    T1a -->|"player ignores"| T1b

    T2["knight_secret"] -->|"player investigates"| T2a["finds royal signet"]
    T2 -->|"20 turns pass"| T2b["assassin arrives"]
    T2a --> T2c["player confronts knight"]
    T2b -->|"knight alive"| T2d["knight revealed publicly"]
    T2b -->|"knight dead"| T2e["secret dies"]
```

Each node has:

- **State**: `dormant`, `foreshadowed`, `active`, `resolved`
- **Trigger conditions**: player action, turn count, world condition, another tension resolving
- **Prompt context**: what to inject into the LLM prompt when this tension is foreshadowed or active
- **Consequences**: world state mutations when the tension activates or resolves

Edges fire when their trigger condition is met. The Director checks all edges each turn. No LLM call is needed to traverse the graph -- it is pure deterministic logic.

## Auto-merging

When the Director activates a tension node, it checks whether any other `active` node shares characters or locations. If so, the tension lines have **collided** -- and the Director automatically spawns a **merge node**.

Example: `barkeep_debt` leads to "collector arrives at tavern." `knight_mystery` leads to "assassin arrives at tavern." If both activate around the same time, the Director detects the collision (same location) and creates a merge node: "collector and assassin both at the tavern."

The merge node:

- Has both colliding nodes as parents
- Combines their prompt context (the LLM gets a richer, more chaotic scene)
- May have its own consequences (the collision itself changes the world)

The LLM doesn't need to know about the merge operation. It just receives a denser context and renders the collision naturally in prose.

## Revert

The player can **revert to the last node** -- undo the most recent tension transition and replay from an earlier state.

This requires a **transition log**: an ordered list of state changes.

```
TransitionEntry {
    turn:       int
    node_id:    string
    old_state:  enum
    new_state:  enum
}
```

Reverting walks the log backwards to the target turn, undoing each transition (restoring `old_state`). History and memory are also rolled back to that turn -- messages after the revert point are discarded.

This is the VCS "revert" operation: `HEAD` moves back, and the future becomes open again. Reverted nodes return to their prior state (typically `dormant` or `foreshadowed`), and the player gets a second chance to make different choices.

## The version control analogy

The plot graph behaves like a version control system:

| VCS concept | Plot graph equivalent |
|---|---|
| Branch | Possible future that hasn't happened yet (dormant tension) |
| Commit | Tension state that has been activated (irreversible under normal play) |
| Merge | Separate tension lines collide (auto-detected by shared characters/locations) |
| Revert | Player undoes recent transitions, replays from earlier state |
| HEAD | Current world state -- the set of all active and resolved tensions |
| Log | Transition log -- ordered list of all state changes, enables revert |

Scenarios define the initial graph. Player actions and the world clock advance HEAD. Resolved tensions are immutable history (unless reverted). Dormant tensions are the space of possible futures.

## The Prophet is the graph

The "Prophet" concept from [[narrative-philosophy]] is not a separate agent. It is the graph itself.

- **Sealed predictions** = dormant nodes with trigger conditions. The system knows they exist; the player doesn't.
- **Foreshadowing** = when a tension transitions from `dormant` to `foreshadowed`, the Director injects subtle hints into the prompt context. Not "tell the player X" but "the atmosphere should carry unease" or "this character's generosity feels performative."
- **Payoff** = when a foreshadowed tension activates, the player experiences the dramatic moment. The signs were there all along -- because the system was seeding them.
- **Ambiguity** = some tensions have edges labeled with conditions that are genuinely uncertain. The system doesn't know which branch will fire until the player acts. This is the Hamlet quality.

## Two loops

The Director runs two loops against the plot graph:

### Player-facing loop

What we have now: player acts, the Director checks which edges fire, world responds. This is synchronous with the SceneLoop.

### World-background loop

Between player turns (or on a turn counter), the Director advances off-screen tensions. Things happen when the player isn't watching:

- The thieves' guild sends a collector. The barkeep can't pay. He's desperate.
- The assassin reaches the city gates. She asks about the knight.
- The well water starts making people sick.

The player discovers these changes through interaction, not exposition. They walk into the tavern and the barkeep has a black eye. They hear rumors in the market. The world reveals itself through detail, not narration.

## The generation pipeline

The plot graph is **not hand-authored**. It is LLM-generated and Director-maintained.

The LLM composes the raw dramatic material. The Director (the rhapsode -- see [[narrative-philosophy#1. The Director is a rhapsode -- an arranger, not a puppeteer]]) extracts and arranges it into structured graph nodes.

```
World state + memory + current graph
        |
        v
   LLM (free text -- unconstrained creative output)
   "The barkeep has been losing sleep. He borrowed
    heavily from Vex, the guild fence, to keep the
    tavern running after the drought. Vex is patient
    but her lieutenant Korr is not -- he's been
    pressing for repayment. Meanwhile, the knight
    Sir Aldric drinks alone every night, speaking to
    no one. The locals whisper he arrived the same
    week the royal courier went missing..."
        |
        v
   Director / extraction pass
   Parse the free text into structured nodes:
   - barkeep_debt (characters: barkeep, Vex, Korr)
   - knight_mystery (characters: Aldric, courier)
   - triggers, foreshadowing, connections
        |
        v
   Plot graph (updated with new nodes)
```

### Three moments when generation happens

**Scenario initialization.** When a scenario starts, the LLM reads the world setup (characters, locations, setting) and generates an initial set of tensions. This happens once, before the player starts.

**Periodic world-building.** Every N turns, the Director asks the LLM to look at the current world state + memory and generate new tensions. New dormant nodes enter the graph. The world deepens over time rather than exhausting its initial content.

**Reactive spawning.** When a major tension resolves (a secret is revealed, a character dies, a faction is defeated), the Director asks the LLM to generate consequences. One resolution spawns new tensions, keeping the world alive.

### Why free text, not structured output

The LLM's creative output is **unconstrained prose**, not JSON. This is deliberate:

- Free text produces richer, more surprising dramatic material than filling in a schema
- The extraction step is a separate concern -- a second LLM call with a tight prompt, or even a smaller/cheaper model
- The extractor validates against existing world state (no duplicate tensions, no phantom characters)
- If extraction fails, the raw text is discarded -- no corrupt graph state

### The LLM's two roles

The LLM is used twice, for fundamentally different purposes:

1. **Composer** -- generates raw dramatic material (free text, creative, unconstrained)
2. **Performer** -- renders the current world state into prose for the player (constrained by prompt context from the Director)

The Director sits between these two uses. It never calls the LLM itself at runtime to make structural decisions. It only structures what the LLM has already imagined.

## Why the Director doesn't use the LLM for structure

The LLM is bad at:

- **Consistency** -- it forgets what it predicted 20 turns ago
- **Timing** -- it can't reliably count turns or track parallel events
- **Secrecy** -- it tends to either blurt secrets or be uselessly vague
- **Determinism** -- same prompt can yield different structural decisions

The LLM is good at:

- **Imagination** -- generating rich dramatic landscapes from a world setup
- **Prose** -- rendering a world state into vivid, contextual narrative
- **Improvisation** -- filling in details that the graph doesn't specify
- **Dialogue** -- making characters feel alive within their constraints

So: the LLM imagines and performs. The graph holds structure. The Director arranges. This is the [[narrative-philosophy#1. The Director is a rhapsode -- an arranger, not a puppeteer|rhapsode principle]] in practice.

## Scenario authoring

A scenario provides the initial world state. The plot graph is then generated from it:

```json
{
  "title": "The Dusty Flagon",
  "system_prompt": "You are the narrator of a fantasy RPG...",
  "characters": [...],
  "locations": [...],
  "seed_messages": [...]
}
```

At initialization, the LLM reads this setup and generates the initial tensions as free text. The Director extracts them into graph nodes. The scenario author designs the *world* -- the characters, their relationships, the setting. The LLM discovers the *dramatic potential*. The Director structures it.

## Debuggability

The plot graph is fully inspectable at any time. You can visualize:

- Which tensions are dormant, foreshadowed, active, resolved
- Which edges are close to firing
- What the world clock shows
- What prompt context the Director is currently injecting

This is a radical improvement over Talemate's Director, where narrative decisions are opaque LLM outputs that cannot be inspected, reproduced, or debugged.

## Resolved questions

1. **Dynamic tensions** -- Yes. New tensions are spawned at runtime via the generation pipeline (periodic world-building + reactive spawning). The graph grows over the course of a session.
2. **Authoring** -- Scenarios are not hand-authored tension graphs. The author designs the world (characters, locations, relationships). The LLM generates the dramatic potential. The Director structures it.
3. **Fortune tracker** -- Rejected. There is no numeric arc tracker. The arc emerges from accumulated memory + active tensions. See [[narrative-philosophy#Why there is no fortune tracker]].

## Open questions

1. **Trigger language**: how do we express "player befriends barkeep"? Keyword matching? LLM classification of player intent? A small dedicated model?
2. **Extraction robustness**: how does the extraction step handle ambiguous or inconsistent LLM output? Retry? Partial extraction? Human review queue?
3. **Tension templates**: reusable patterns ("character has a secret," "faction conflict," "ticking clock") that the extraction step could match against?
4. **Visual editor**: the plot graph is the natural candidate for a visual inspection/editing tool. A node editor where each node is a tension and edges are triggers.
5. **Serialization**: the graph must be saveable (for session save/load). Format? Separate from world state or embedded?
