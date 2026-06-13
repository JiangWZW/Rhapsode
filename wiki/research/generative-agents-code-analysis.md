---
title: Generative Agents — Code Architecture Analysis
date: 2026-05-22
tags: [memory, agents, stanford, observe-reflect-plan, retrieval]
---

# Generative Agents (Park et al., 2023) — Code Architecture Analysis

Source: `E:\generative_agents` ([GitHub](https://github.com/joonspk-research/generative_agents))

This is the official Stanford implementation of "Generative Agents: Interactive Simulacra of Human Behavior" (UIST 2023, 4,781 citations). The codebase implements the observe-reflect-plan cognitive architecture for 25 autonomous NPCs in a tile-based world.

---

## File Structure

```
generative_agents/
├── reverie/                           # Backend simulation engine
→   └── backend_server/
→       ├── reverie.py                 # Main loop — ReverieServer (665 lines)
→       ├── maze.py                    # World grid, tiles, events (417 lines)
→       ├── path_finder.py             # A* pathfinding (269 lines)
→       └── persona/
→           ├── persona.py             # Agent class — cognitive loop (272 lines)
→           ├── cognitive_modules/
→           →   ├── perceive.py        # Observe environment (192 lines)
→           →   ├── retrieve.py        # Memory retrieval scoring (285 lines)
→           →   ├── plan.py            # Long/short-term planning (1053 lines)
→           →   ├── reflect.py         # Reflection / abstraction (272 lines)
→           →   ├── execute.py         # Path execution (175 lines)
→           →   └── converse.py        # Multi-agent dialogue (324 lines)
→           ├── memory_structures/
→           →   ├── associative_memory.py  # Memory Stream (361 lines)
→           →   ├── scratch.py             # Working state + hyperparams (638 lines)
→           →   └── spatial_memory.py      # Location tree (119 lines)
→           └── prompt_template/
→               ├── gpt_structure.py       # OpenAI API wrappers (332 lines)
→               ├── run_gpt_prompt.py      # All LLM prompt functions (~2931 lines)
→               └── v3_ChatGPT/            # Prompt templates (.txt)
→ +└── environment/frontend_server/       # Django + Phaser visual frontend
    ├── storage/                        # Live simulation data (JSON per step)
    └── static_dirs/assets/            # Tiled map assets
```

**Total:** ~39 Python files. Cognitive core is ~15 files under `persona/`.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────→ +→                     ReverieServer (reverie.py)                   → +→  curr_time, step, sec_per_step=10, maze, personas{}             → +└─────────────────────────────────────────────────────────────────→ +                           → each step: persona.move(tile, time)
                           ▼+┌─────────────────────────────────────────────────────────────────→ +→                        Persona (persona.py)                      → +→                                                                 → +→  ┌─────────→  ┌──────────→  ┌──────→  ┌─────────→  ┌─────────│ │
→  → PERCEIVE│ │ → RETRIEVE │ │ → PLAN │ │ → REFLECT │ │ → EXECUTE │ │
→  └─────────→  └──────────→  └──────→  └─────────→  └─────────│ │
→       →            →           →           →            →      → +→  ┌────└────────────└───────────└───────────└────────────└────→ → +→  →                    Memory Structures                        → → +→  →  s_mem (spatial)  →  a_mem (associative)  →  scratch (WM)  → → +→  └────────────────────────────────────────────────────────────→ → +└─────────────────────────────────────────────────────────────────→ +                           → movement/{step}.json
                           ▼+┌─────────────────────────────────────────────────────────────────→ +→              Django + Phaser (frontend_server)                   → +→  process → update → execute → animate sprites on tile map       → +└─────────────────────────────────────────────────────────────────→ +```

---

## The Cognitive Loop

The entire agent tick is 5 lines in `persona.py`:

```python
# persona.py lines 219-231
perceived = self.perceive(maze)
retrieved = self.retrieve(perceived)
plan = self.plan(maze, personas, new_day, retrieved)
self.reflect()
return self.execute(maze, personas, plan)
```

One step = 10 seconds of in-game time. Day boundaries trigger long-term replanning.

---

## Memory System: `AssociativeMemory` (Memory Stream)

### The Atomic Unit: `ConceptNode`

Every memory is stored as a `ConceptNode`:

```python
# associative_memory.py lines 19-43
class ConceptNode:
  def __init__(self, node_id, node_count, type_count, node_type, depth,
               created, expiration, s, p, o,
               description, embedding_key, poignancy, keywords, filling):
    self.node_id = node_id
    self.node_count = node_count
    self.type_count = type_count
    self.type = node_type       # "event" | "thought" | "chat"
    self.depth = depth          # 0 for events/chats; thoughts = 1 + max(evidence depths)

    self.created = created
    self.expiration = expiration
    self.last_accessed = self.created

    self.subject = s            # SPO triple
    self.predicate = p
    self.object = o

    self.description = description
    self.embedding_key = embedding_key
    self.poignancy = poignancy  # 1-10 importance (LLM-rated)
    self.keywords = keywords    # set — used for keyword index
    self.filling = filling      # evidence node IDs (thoughts) or chat transcript (chats)
```

### Storage Architecture

```python
# associative_memory.py lines 50-64
class AssociativeMemory:
  def __init__(self, f_saved):
    self.id_to_node = dict()       # node_id → ConceptNode

    self.seq_event = []            # chronological event list (newest first)
    self.seq_thought = []          # chronological thought list
    self.seq_chat = []             # chronological chat list

    self.kw_to_event = dict()      # keyword → [event nodes] (inverted index)
    self.kw_to_thought = dict()    # keyword → [thought nodes]
    self.kw_to_chat = dict()       # keyword → [chat nodes]

    self.kw_strength_event = dict()   # keyword frequency counter
    self.kw_strength_thought = dict()

    self.embeddings = json.load(...)  # text → float[] (ada-002)
```

Three access patterns:
1. **Sequential** — `seq_event`, `seq_thought`, `seq_chat` (ordered, newest-first)
2. **Keyword inverted index** — `kw_to_event[keyword]` for fast lookup by subject/object
3. **Embedding space** — `embeddings[text]` for cosine similarity retrieval

### Persistence (JSON per simulation fork)

```
personas/{name}/bootstrap_memory/
├── scratch.json              # Working state
├── spatial_memory.json       # Explored locations tree
└── associative_memory/
    ├── nodes.json            # All ConceptNodes serialized
    ├── embeddings.json       # text → float[] cache
    └── kw_strength.json      # keyword frequency counters
```

---

## Perceive: Writing Memories

When a new event is observed (not in last `retention` events):

```python
# perceive.py lines 109-178
for p_event in perceived_events:
    s, p, o, desc = p_event

    latest_events = persona.a_mem.get_summarized_latest_events(persona.scratch.retention)
    if p_event not in latest_events:
        # Extract keywords from subject/object
        keywords = set()
        sub = p_event[0].split(":")[-1] if ":" in p_event[0] else p_event[0]
        obj = p_event[2].split(":")[-1] if ":" in p_event[2] else p_event[2]
        keywords.update([sub, obj])

        # Get or create embedding
        event_embedding = get_embedding(desc_embedding_in)
        event_embedding_pair = (desc_embedding_in, event_embedding)

        # LLM rates importance 1-10
        event_poignancy = generate_poig_score(persona, "event", desc_embedding_in)

        # Store to memory stream
        ret_events += [persona.a_mem.add_event(
            persona.scratch.curr_time, None,
            s, p, o, desc, keywords, event_poignancy,
            event_embedding_pair, chat_node_ids)]

        # Decrement reflection trigger counter
        persona.scratch.importance_trigger_curr -= event_poignancy
        persona.scratch.importance_ele_n += 1
```

Key details:
- Vision radius `vision_r = 8` tiles, filtered to same arena
- Attention bandwidth `att_bandwidth = 8` (max events per step)
- Retention window `retention = 8` (dedup against last 8 events)
- Each event's poignancy is **subtracted from the reflection counter** — high-importance events trigger reflection faster

---

## Retrieve: The Three-Factor Scoring Function

The paper's core retrieval mechanism, implemented as a weighted sum of three normalized components:

```python
# retrieve.py lines 132-260
def extract_recency(persona, nodes):
    recency_vals = [persona.scratch.recency_decay ** i
                    for i in range(1, len(nodes) + 1)]
    recency_out = dict()
    for count, node in enumerate(nodes):
        recency_out[node.node_id] = recency_vals[count]
    return recency_out

def extract_importance(persona, nodes):
    importance_out = dict()
    for count, node in enumerate(nodes):
        importance_out[node.node_id] = node.poignancy
    return importance_out

def extract_relevance(persona, nodes, focal_pt):
    focal_embedding = get_embedding(focal_pt)
    relevance_out = dict()
    for count, node in enumerate(nodes):
        node_embedding = persona.a_mem.embeddings[node.embedding_key]
        relevance_out[node.node_id] = cos_sim(node_embedding, focal_embedding)
    return relevance_out

def new_retrieve(persona, focal_points, n_count=30):
    retrieved = dict()
    for focal_pt in focal_points:
        nodes = [[i.last_accessed, i]
                  for i in persona.a_mem.seq_event + persona.a_mem.seq_thought
                  if "idle" not in i.embedding_key]
        nodes = sorted(nodes, key=lambda x: x[0])
        nodes = [i for created, i in nodes]

        recency_out = normalize_dict_floats(extract_recency(persona, nodes), 0, 1)
        importance_out = normalize_dict_floats(extract_importance(persona, nodes), 0, 1)
        relevance_out = normalize_dict_floats(extract_relevance(persona, nodes, focal_pt), 0, 1)

        gw = [0.5, 3, 2]  # recency, relevance, importance weights
        master_out = dict()
        for key in recency_out.keys():
            master_out[key] = (persona.scratch.recency_w * recency_out[key] * gw[0]
                           + persona.scratch.relevance_w * relevance_out[key] * gw[1]
                           + persona.scratch.importance_w * importance_out[key] * gw[2])

        # Return top n_count nodes
        ...
```

### The Scoring Formula

```
score(node, query) = 0.5 × recency + 3 × relevance + 2 × importance
                     (normalized)      (normalized)    (normalized)
```

Where:
- **Recency** = `0.995^position` (exponential decay by access order, not absolute time)
- **Importance** = `node.poignancy` (LLM-rated 1—0 at creation time)
- **Relevance** = `cosine_sim(node_embedding, query_embedding)` (ada-002)

The weights `gw = [0.5, 3, 2]` heavily favor **relevance** (3×) and **importance** (2×) over recency (0.5×).

Side effect: retrieved nodes get `last_accessed = curr_time` — reinforcing recently-accessed memories.

---

## Reflect: Importance-Triggered Abstraction

### Trigger Mechanism

```python
# reflect.py lines 135-155
def reflection_trigger(persona):
    if (persona.scratch.importance_trigger_curr <= 0 and
        [] != persona.a_mem.seq_event + persona.a_mem.seq_thought):
        return True
    return False
```

- `importance_trigger_max = 250` (Isabella's default)
- Each perceived event subtracts its poignancy (1—0) from `importance_trigger_curr`
- When counter hits ≈0 → reflection fires, counter resets to max
- Average: ~25—0 events between reflections (depending on poignancy distribution)

### Reflection Pipeline

```python
# reflect.py lines 99-132
def run_reflect(persona):
    # 1. Generate 3 focal points (questions about recent events)
    focal_points = generate_focal_points(persona, 3)

    # 2. Retrieve relevant memories per focal point
    retrieved = new_retrieve(persona, focal_points)

    # 3. For each focal point, generate insights with evidence
    for focal_pt, nodes in retrieved.items():
        thoughts = generate_insights_and_evidence(persona, nodes, 5)

        # 4. Store as thought nodes (with depth > evidence)
        for thought, evidence in thoughts.items():
            created = persona.scratch.curr_time
            expiration = persona.scratch.curr_time + datetime.timedelta(days=30)
            s, p, o = generate_action_event_triple(thought, persona)
            keywords = set([s, p, o])
            thought_poignancy = generate_poig_score(persona, "thought", thought)
            thought_embedding_pair = (thought, get_embedding(thought))

            persona.a_mem.add_thought(created, expiration, s, p, o,
                                      thought, keywords, thought_poignancy,
                                      thought_embedding_pair, evidence)
```

### Abstraction Depth

Thought depth is computed recursively from evidence:

```python
# associative_memory.py lines 207-212 (inside add_thought)
depth = 1
try:
    if filling:
        depth += max([self.id_to_node[i].depth for i in filling])
except:
    pass
```

This creates an **abstraction hierarchy**: events have depth 0, first-order reflections have depth 1, reflections-on-reflections have depth 2+.

---

## Plan: Hierarchical Temporal Decomposition

### Long-term (new day)

1. `generate_wake_up_hour()` — LLM picks wake time
2. `revise_identity()` — updates self-description from recent memories
3. `generate_hourly_schedule()` — 24 one-hour activity strings
4. Compressed to `[activity, minutes]` pairs totaling 1440 min
5. Stored as a thought in memory stream

### Short-term (each step when current action expires)

1. **Decompose** hourly block into 5—5 min sub-tasks via LLM
2. Pick current sub-task based on elapsed minutes
3. LLM selects: sector → arena → game object → emoji → event triple

### Social Reactions

When retrieved events include another persona:

```
generate_decide_to_talk() = yes?
  └── yes + not sleeping + buffer expired → _chat_react()
          → +          鈹溾攢 Multi-turn dialogue (up to 8 rounds)
          鈹溾攢 Store chat in memory stream
          鈹斺攢 Rewrite remaining schedule
```

Chat buffer: after conversation, `chatting_with_buffer[target] = 800` steps (~2+ hours game time).

---

## Theoretical Framing: Event-Driven Accumulator with Triggered Compression

The Generative Agents memory system is fundamentally different from Summaryception's recursive filter. It is an **event-driven append-only log** with **triggered compression** (reflection).

### The Recurrence Relation

There is no recurrence in the Summaryception sense. Instead:

```
Memory(t) = Memory(t-1) ∝{ perceived_events(t) }

If importance_accumulated > threshold:
    Memory(t) = Memory(t) ∝{ reflect(retrieve(focal_points)) }
```

The system **never discards** — it only **adds**. Old memories aren't compressed or replaced; they naturally lose retrieval priority through the recency decay (`0.995^position`).

### Comparison: Summaryception vs Generative Agents

| Dimension | Summaryception | Generative Agents |
|-----------|---------------|-------------------|
| **Model** | Recursive filter (IIR) | Append-only log + triggered abstraction |
| **Compression** | Lossy, irreversible, continuous | Additive — reflections don't replace events |
| **Recurrence** | Output feeds back as input context | No feedback — retrieval is stateless scoring |
| **Memory growth** | Bounded (promotion drains layers) | Unbounded (all nodes persist forever) |
| **Forgetting** | Explicit (ghosting + overwrite) | Implicit (recency decay in scoring) |
| **Trigger** | Turn count threshold | Importance accumulator |
| **Time constant** | Fixed batch size | Variable (high-importance events trigger faster) |
| **Error correction** | None (manual snippet edit) | Natural — new reflections can contradict old ones |
| **Context usage** | Full accumulator as LLM input | Top-30 nodes per focal point |
| **Abstraction depth** | Layer index (0, 1, 2, ...) | Node depth (0 = event, 1+ = thought of thoughts) |

### Key Architectural Insight

Generative Agents uses **implicit forgetting** via the retrieval scoring function — old, unimportant, irrelevant memories simply never get retrieved. They exist but are effectively invisible.

Summaryception uses **explicit forgetting** via lossy compression — old messages are permanently reduced to a single summary line.

The trade-off:
- **GA approach**: Unbounded memory → eventual retrieval degradation as the embedding space gets crowded. But any specific memory can be "resurrected" if the right query activates it.
- **SC approach**: Bounded memory → predictable resource usage. But lost detail is permanently unrecoverable.

### The Reflection System as Episodic → Semantic Conversion

Reflection converts **episodic memories** (concrete events) into **semantic memories** (abstract beliefs/insights). This parallels the cognitive science distinction:

```
Events (depth 0):  "Isabella saw Maria writing at the cafe at 2pm"
                        → reflection trigger
Thoughts (depth 1): "Maria is a dedicated writer who works regularly"
                        → another reflection
Thoughts (depth 2): "The cafe has become a creative workspace for the community"
```

Each level strips temporal specificity and gains generality. The `depth` field tracks this pyramid.

---

## Hyperparameters

| Parameter | Default | Effect |
|-----------|---------|--------|
| `vision_r` | 8 | Tile perception radius |
| `att_bandwidth` | 8 | Max events perceived per step |
| `retention` | 8 | Dedup window (last N events) |
| `recency_decay` | 0.995 | Exponential decay per position |
| `recency_w` | 1 | Weight on recency score |
| `relevance_w` | 1 | Weight on relevance score |
| `importance_w` | 1 | Weight on importance score |
| `importance_trigger_max` | 250 | Reflection threshold (sum of poignancy) |
| Retrieval weights `gw` | [0.5, 3, 2] | Hardcoded: recency, relevance, importance |
| `n_count` | 30 | Top-K retrieval results |
| `sec_per_step` | 10 | Seconds per simulation tick |

---

## Simulation Loop (File-Based IPC)

```
┌──────────────────────────────────────────────────────────────────→ +→                    One Simulation Step                            → +→                                                                  → +→  Frontend (Phaser)                    Backend (reverie.py)        → +→  ─────────────────                    ────────────────────        → +→  POST persona positions ──────────▼environment/{step}.json      → +→                                              →                   → +→                                    For each persona:             → +→                                      persona.move()              → +→                                        鈹溾攢 perceive(maze)         → +→                                        鈹溾攢 retrieve(perceived)    → +→                                        鈹溾攢 plan(...)              → +→                                        鈹溾攢 reflect()              → +→                                        鈹斺攢 execute(...)           → +→                                              →                   → +→  Poll for results ├───────────────── movement/{step}.json        → +→  Animate sprites                     (next_tile, emoji, desc)    → +→                                                                  → +→  step++, curr_time += 10s                                        → +└──────────────────────────────────────────────────────────────────→ +```

---

## LLM Usage

| Function | Model | Purpose |
|----------|-------|---------|
| `ChatGPT_safe_generate_response` | gpt-3.5-turbo | Most structured outputs |
| `get_embedding` | text-embedding-ada-002 | Memory storage & retrieval |
| `run_gpt_prompt_event_poignancy` | gpt-3.5-turbo | Rate importance 1—0 |
| `run_gpt_prompt_focal_pt` | gpt-3.5-turbo | Generate reflection questions |
| `run_gpt_prompt_insight_and_guidance` | gpt-3.5-turbo | Generate abstract thoughts |
| `run_gpt_prompt_decide_to_talk` | gpt-3.5-turbo | Social decision |
| `run_gpt_prompt_daily_plan` | gpt-3.5-turbo | Broad daily goals |
| `run_gpt_prompt_task_decomp` | gpt-3.5-turbo | Break hourly task to 5-min chunks |

30+ prompt functions total, all template-based with `!<INPUT N>!` placeholders.

---

## Relevance to Rhapsode

Rhapsode's `CharacterMemory` system (Generative Agents-inspired per `wiki/architecture/plot-graph.md`) maps directly to this codebase:

| GA concept | Rhapsode equivalent |
|---|---|
| `ConceptNode` | `MemoryNode` in CharacterMemory's boost graph |
| `AssociativeMemory` | Per-character belief graph + ChromaDB index |
| `poignancy` scoring | Quality pipeline (planned) |
| `new_retrieve()` three-factor scoring | Rhapsode's hybrid retrieval (BM25 removed, now embedding-based) |
| Reflection trigger (importance accumulator) | Could adopt — currently no trigger mechanism |
| `depth` field on thoughts | Rhapsode's node hierarchy (not yet explicit depth tracking) |
| Spatial memory tree | Rhapsode's `WorldGraph` with typed edges |
| Scratch (working memory) | Rhapsode's `Character` struct (current state fields) |

Key lessons:
1. The **importance accumulator** as reflection trigger is elegant — avoids both time-based (too regular) and count-based (ignores significance) approaches
2. The **three-factor retrieval** with learned weights is more principled than pure embedding similarity
3. The **append-only + implicit forgetting** model avoids Summaryception's information loss problem but has unbounded growth
4. The **abstraction depth** tracking enables distinguishing concrete vs. abstract memories in retrieval
