# Literature review — LLM story generation

A survey of recent research (2022-2026) relevant to Rhapsode's architecture. Papers selected from [awesome-llm-story-generation](https://github.com/Picrew/awesome-llm-story-generation) for relevance to our core subsystems: the Director, the plot graph, the memory system, and interactive narrative control.

## Summary of adopted ideas

| Source | Adopted idea | Maps to |
|--------|-------------|---------|
| CFPG (2026) | Foreshadow-Trigger-Payoff triples as first-class objects | Tension node states: dormant → foreshadowed → active → resolved |
| CFPG | Inject structured (F,T,P) into prompts, not vague summaries | `foreshadow_ctx` / `active_ctx` fields on tension nodes |
| CFPG | New-thread extraction after each beat | Generation pipeline: reactive spawning |
| CFPG | Tension typing taxonomy (object / event / rule / speech-act / symbol) | Tension node `type` field |
| RecurrentGPT (2023) | Plan-as-retrieval-query | Director context = memory retrieval query |
| RecurrentGPT | Bounded STM rewrite with token budget | Raw recent → summarized archive transition |
| RecurrentGPT | Multiple candidate plans for player choice | Constrained choices at graph nodes |
| Suspenseful Stories (2024) | Rational ordering — close easiest escapes first | Director narrowing policy |
| Suspenseful Stories | Knowledge-state tagging: player_known / npc_known / hidden | Per-tension metadata for revelation timing |
| Suspenseful Stories | Clue insertion is empirically significant for suspense | Validates foreshadowing mechanism |
| FACTTRACK (2024) | Pre/post facts with validity intervals per event | Edge preconditions/postconditions |
| FACTTRACK | Supersession vs. contradiction distinction | Director: "world changed" vs. "inconsistency" |
| StoryVerse (2024) | Placeholder resolution — named slots bound at runtime | Unresolved tension slots (e.g., "the traitor = ?") |
| StoryVerse | Character motivation check as alignment gate | Director validates NPC action plausibility |
| IBSEN (2024) | Synopsis + keywords instead of golden lines | `active_ctx` as directional hints, not prose |
| IBSEN | Turn/tension budgets to prevent stalls | Anti-stall policy on tension nodes |
| Generative Agents (2023) | Memory stream with importance + recency + relevance retrieval | Three-layer memory with importance scoring |
| Generative Agents | Reflection triggers on cumulative importance threshold | Off-screen NPC depth via periodic reflection |
| EvoSpark (2026) | Reflect-synthesize-consolidate for NPC state | Efficient off-screen NPC updates |
| Intra (2025) | Input rewriting as intent parser — prevent false narrative demands | Preprocessing step between freeform input and SceneLoop |
| Intra | Guided thinking — structured question chains for LLM reasoning | Director prompt design pattern |
| Intra | Event filtering per NPC for information asymmetry | Complement to explicit KnowledgeState metadata |
| Intra | Ground truth first — formal state outside the LLM's narrative | Validates plot graph as authoritative game state |

## Confirmed gaps (Rhapsode's novelty)

No reviewed paper implements the combination of:

1. **Explicit multi-dimensional tension DAG** — IBSEN uses linear objectives; StoryVerse uses implicit prerequisites; CFPG tracks triples in a flat pool; Generative Agents has no plot structure.
2. **Constrained choices at graph nodes + freeform simulation on edges** — no paper combines visual-novel-style mandatory branching with open-world freeform input.
3. **Read/write action distinction** as an input mode controller — no paper uses graph position to determine whether the player gets freeform text or curated options.
4. **Multi-dimensional arrival** — no paper addresses concurrent tension threads reaching decision points simultaneously.

---

## Paper summaries

### IBSEN — Director-Actor Agent Collaboration for Controllable and Interactive Drama Script Generation

**Han et al., ACL 2024** — [arXiv:2407.01093](https://arxiv.org/abs/2407.01093) — [Code](https://github.com/OpenDFM/ibsen)

**Architecture.** Separates a centralized Director from distributed Actor agents. The Director receives a predefined plot objective list `⟨G₁, G₂, …⟩`, writes a continuation outline for the current objective, translates it into a dialogue script `⟨T̂₁, T̂₂, …⟩`, then builds instructions for actors. Actors produce actual dialogue from profile, memory, and director instruction — they never see the raw planned lines, only a synopsis + keywords + objective. This prevents copy-paste while preserving intent.

**Plot coherence.** Anchored by: sequential objectives, outline → script hierarchy, objective completion checking after each turn (LLM → JSON `{completed, reason}`), and prompts that forbid jumping ahead. Anti-stall: force-complete if objective not met after 9 turns.

**Player agency.** Player is an actor not bound by instructions. Player involvement triggers regeneration of the current storyline and script so NPCs can react. Consecutive player actions are rate-limited to avoid constant rebuilds.

**Relevance to Rhapsode.**
- Validates Director-Actor separation with hierarchical planning and softened directives.
- Our DAG of tensions directly improves on their linear `⟨Gᵢ⟩` by supporting merge/branch structure.
- Their weakest point — pure-LLM objective verification (F1 ≈ 0.77) — argues for graph-topology-derived predicates at critical transitions.
- Per-act directors ≈ per-thread subgraph managers, reducing prompt complexity.
- Turn budgets (force-advance after N turns) are directly adoptable as anti-stall policy.

**Limitations.** Text drama only (no spatial interaction). Agents push plot aggressively, reducing behavioral simulation. LLM safety skew flattens morally dark arcs. Repetition and over-narration remain practical defects.

---

### StoryVerse — Co-authoring Dynamic Plot with LLM-based Character Simulation via Narrative Planning

**Wang, Zhou, Ledo (Autodesk Research), FDG 2024** — [arXiv:2405.13042](https://arxiv.org/abs/2405.13042)

**Architecture.** Three components: Act Director (LLM-based narrative planner), Character Simulator (autonomous agent behavior between beats), and Game Environment (structured world state with action schemas). Per-timestep: check if any pending abstract act has all prerequisites satisfied → if yes, Director instantiates the act via iterative generate → review → revise; if no, Character Simulator produces autonomous actions.

**Plot structure.** Abstract acts carry: (1) narrative goal, (2) prerequisites (AND/OR formula over world-state assertions, player actions, or other act outcomes), (3) placeholders (named slots bound at runtime and carried forward). Execution order is determined by when prerequisites become true, not by authorial sequence. This creates an implicit dependency graph.

**Authorial control vs. emergence.** Writers author abstract acts (beat-level intent), not micro-actions. Between beats, the Character Simulator drives all behavior autonomously. Character Simulation Evaluation during plan review: an LLM-as-character checks whether motivation is established, pushing plans toward psychologically plausible paths.

**Relevance to Rhapsode.**
- "Eligible act → Director plan; no act → simulation" maps exactly to "at a node → constrained choices; on an edge → freeform."
- Placeholder resolution (named slots filled at runtime, carried forward) is a mechanism for propagating narrative state across graph nodes — our tension nodes could have unresolved slots.
- Three-pronged review loop (coherency + environment + character motivation) is a quality-control template for Director fragment assembly.
- Validates separation of narrative structure from world/simulator layer — same abstract-act outline instantiates in completely different story domains.

**Limitations.** No evaluation (proof of concept only). Shallow player modeling (world-state edits only). No explicit global graph object. Latency from many LLM calls per act.

---

### Codified Foreshadowing-Payoff Text Generation (CFPG)

**Yun et al. (UCSD), ArXiv 2026** — [arXiv:2601.07033](https://arxiv.org/abs/2601.07033) — [Code](https://github.com/LongfeiYun17/CFPG)

**Formalization.** Narrative commitments as causal debts. Each commitment is a Foreshadow-Trigger-Payoff (F-T-P) triple: F creates the debt (setup/anomaly), T is the prerequisite condition that must hold before P becomes actionable, P is the resolution. The trigger is what separates premature payoff (spoiling) from missing payoff (inconsistency).

**Data structures.** Foreshadow pool `C_t` = set of all unfulfilled triples at step t. Eligible subset `S_t` = triples whose triggers are satisfied given current text. A `codify` function checks trigger satisfaction — this is the gating mechanism.

**Pipeline.** Select → Generate → Update loop: (1) evaluate which triples are eligible, (2) condition generation on active debts and their payoffs as explicit constraints, (3) verify which commitments were realized, remove them, extract new F-T-P triples from generated text.

**Relevance to Rhapsode.**
- Maps directly to our tension states: dormant (F in pool, T not satisfied), foreshadowed (F visible but P deferred), active (T satisfied, P injected into prompt), resolved (removed from pool).
- Key finding: injecting structured (F,T,P) into prompts is dramatically better than vague "remember the thread" — our `foreshadow_ctx` / `active_ctx` fields are the right approach.
- New-thread extraction after each beat (parse generated text into new F-T-P triples) maps to our reactive spawning in the generation pipeline.
- Tension typing taxonomy: object (48.2%), event (35.3%), speech-act (9.7%), rule (5.1%), symbol (1.7%) — adoptable for our tension node `type` field.

**Limitations.** Tested on BookSum summaries, not full novels. Linear stories only, no interactive/branching. Subtlety of foreshadowing is not algorithmically enforced — it depends on model writing quality. Small corpus (629 pairs from 148 books).

---

### RecurrentGPT — Interactive Generation of (Arbitrarily) Long Text

**Zhou, Jiang et al. (ETH Zürich), ArXiv 2023** — [arXiv:2305.13304](https://arxiv.org/abs/2305.13304) — [Code](https://github.com/aiwaves-cn/RecurrentGPT)

**Architecture.** Replaces LSTM vector states with natural-language equivalents. Each timestep: receive previous paragraph + plan → retrieve from LTM using plan as query → read STM → prompt LLM → produce new paragraph + new candidate plans + rewritten STM + LTM summary entry.

**Memory.** Two layers: Short-term memory (10-20 sentences, rewritten every step — LLM drops stale facts, adds new) and Long-term memory (Sentence-BERT embeddings in vector DB, append-only, queried by semantic similarity using the plan as query). Ablation confirms removing either layer degrades coherence significantly.

**Plans.** Rolling, local (next step only), not a fixed global outline. System generates 3 candidate plans for diversity. In interactive fiction mode, plans are framed as meaningful character choices. The plan doubles as the retrieval query — narrative intent drives what history is recalled.

**Relevance to Rhapsode.**
- Plan-as-retrieval-query: Director's assembled context (active tensions + world state) doubles as memory retrieval query. Elegant and directly adoptable.
- Bounded STM rewrite: mirrors our "raw recent → summarized archive" transition. Enforce a token budget on working summary; LLM rewrites each turn with explicit instructions to drop stale info.
- Multiple candidate plans: directly supports constrained-choice mechanism at nodes.
- Per-turn artifact triad (narration + plan + memory update): clean contract for our turn pipeline.
- Human-editable NL state: expose memory and plans to GM tools for canon fixes.

**Limitations.** Quantitative eval only up to ~5,000 words. Consistency gaps in IF mode. Backbone dependence (requires ChatGPT minimum; GPT-4 dramatically better). No architectural guarantee — information flow depends entirely on prompt quality.

---

### Generative Agents — Interactive Simulacra of Human Behavior

**Park et al., UIST 2023** — [arXiv:2304.03442](https://arxiv.org/abs/2304.03442) — [Code](https://github.com/joonspk-research/generative_agents)

**Memory stream.** Append-only natural-language log of observations, reflections, and plans. Each record has description, creation timestamp, last-access timestamp. Retrieval selects subset via scored combination of recency (exponential decay, factor 0.995), importance (1-10 integer, LLM-assigned at creation), and relevance (embedding cosine similarity). All three scores min-max normalized, equally weighted.

**Reflection.** Triggered when cumulative importance of recent memories exceeds 150. LLM generates salient questions, retrieves supporting memories, synthesizes cited insights into a reflection tree. Reflections are themselves memories that can be retrieved and reflected upon — creating a hierarchy of abstraction.

**Planning.** Top-down decomposition: day plan → hourly blocks → 5-15 minute action chunks. Plans are stored as memories and can be revised when new information arrives (via re-planning prompt).

**NPC autonomy.** All 25 agents tick continuously. Locale-based perception triggers dialogue or pass-by decisions. Multi-turn conversations generated with per-agent memory retrieval. Both sides record the exchange as new observations — information propagates through social interaction.

**Relevance to Rhapsode.**
- Importance-weighted retrieval maps directly to our memory importance scoring. Our three sources (graph events, structural signals, LLM assessment) refine their single LLM score.
- Per-agent subjective memory enables secrets and unreliable narrators — NPCs remember different versions of events.
- Reflection threshold (cumulative importance > 150) is a tunable knob for NPC depth — adoptable for off-screen NPC processing.
- Social diffusion through dialogue supports emergent rumors and quests — player actions propagate through NPC conversations naturally.

**Limitations.** No inherent dramatic structure or pacing — pure simulation gets boring without a Director. Trait drift and over-cooperation from instruction tuning. Location choice degrades as memory grows. High cost/latency at scale. Validates that simulation alone is insufficient; Rhapsode's Director is essential.

---

### FACTTRACK — Time-Aware World State Tracking in Story Outlines

**NAACL 2025** — [arXiv:2407.16347](https://arxiv.org/abs/2407.16347)

**World state representation.** Maximal set of mutually non-contradictory atomic facts at a given narrative time. Each event decomposes into pre-facts (preconditions) and post-facts (postconditions) via LLM prompting.

**Temporal model.** Global timeline [0, 1] with hierarchical nesting — parent event intervals split into k equal sub-intervals for children. Facts carry validity intervals: pre-facts default to (-∞, l], post-facts to [r, ∞). When same-direction facts contradict, the newer one wins and the older is trimmed.

**Contradiction detection.** Finetuned NLI model scores fact pairs. Two thresholds: >0.8 for interval updates (strict, low-harm if missed), >0.24 for contradiction flagging (sensitive, high-harm if missed). Cross-direction contradictions require strict interval overlap (Allen's Interval Algebra) to separate legitimate state evolution from true inconsistency.

**Relevance to Rhapsode.**
- Pre/post facts per event: each plot graph edge could carry explicit preconditions and postconditions as atomic NL facts.
- Supersession vs. contradiction: distinguish "the world changed" (trim interval, replace fact) from "this is inconsistent" (block Director, surface to narrator). Critical for handling freeform player actions that might invalidate graph state.
- Dual thresholds: high bar for mutating stored world state; lower bar for raising Director warnings.
- Hierarchical depth = interval nesting: matches multi-scale plot graphs (act → scene → beat).

**Limitations.** Tested on ~2000-3000 word outlines (39 nodes, depth 3). No true gold labels. Decomposition bottleneck (LLM extraction is error-prone). Binary contradictions only.

---

### Creating Suspenseful Stories — Iterative Planning with Large Language Models

**Xie & Riedl, EACL 2024** — [arXiv:2402.17119](https://arxiv.org/abs/2402.17119)

**Suspense model.** Based on Gerrig & Bernardo (1994): suspense = hope + fear + uncertainty, driven by shrinking escape space. The reader mentally enumerates how the protagonist might escape; as options are eliminated, suspense increases. Formalized as an iterative generation pipeline.

**Pipeline.** Background setup (genre, protagonist, goal, dire consequence) → outline planning loop (what action would protagonist take? → why does it fail? → repeat) → final iteration succeeds. Constraints: later actions must acknowledge prior failures; rational protagonist tries best options first, so perceived likelihood decreases across iterations.

**Information asymmetry.** Two revelation modes: early reveal (tell reader why plan will fail before protagonist acts — dramatic irony) and late reveal (explain after the fact). Empirical finding: clue insertion (foreshadowing hints) is statistically significant for suspense (57.9% vs 10.9%, p<0.05). Early vs. late reveal difference was not statistically significant.

**Relevance to Rhapsode.**
- Rational ordering heuristic: close easiest escape routes first to build pressure. Director should close high-value tension paths early.
- Knowledge-state tagging: per-beat `{player_known, npc_known, hidden}` — Director decides revelation timing as a pacing lever. Default policy: prefer dramatic irony over surprise reveals.
- Clue insertion is empirically validated — our foreshadowing mechanism is justified by evidence.
- Cumulative failure state: plot graph tracks closed edges as permanent world state. Director's tension score = f(open paths remaining, time pressure).
- Empathy ↔ suspense correlation: invest in character setup beats before ramping tension.

**Limitations.** English-only, Western storytelling tradition. No automated suspense metric. "Decreasing likelihood" ordering perceived by only ~55% of annotators. Linear, non-interactive, single protagonist. Genre-thriller-heavy.

---

### EvoSpark — Endogenous Interactive Agent Societies for Unified Long-Horizon Narrative Evolution

**ACL 2026** — [arXiv:2604.12776](https://arxiv.org/abs/2604.12776)

**Architecture.** Four-agent hierarchy: Genesis (world seed), Architect (narrative design), Director (scene orchestration), Role (character agents). Endogenous narrative = stories emerge from agent interactions and internal state changes, not from external scripting.

**Memory.** Stratified with reflect-synthesize-consolidate pipeline: agents periodically reflect on accumulated observations, synthesize high-level insights, and consolidate state. Triggered by intensity thresholds on accumulated events — similar to Generative Agents' reflection but with in-place state overwrite rather than append-only logs.

**Spatial grounding.** GMS (Geographic Management System) provides spatial blocking — characters must be co-located to interact. Director uses this as a scheduling constraint.

**Relevance to Rhapsode.**
- Reflect-synthesize-consolidate pattern for off-screen NPC updates: efficient between-turn processing.
- In-place state overwrite (mutable NPC "living snapshot") vs. append-only logs — worth considering for frequently-changing NPC state.
- Spatial blocking as a Director substep: characters can only interact if co-located, creating natural dramatic constraints.
- Lacks an explicit multi-arc graph — Rhapsode's multi-dimensional tension DAG fills this gap.

**Limitations.** No explicit multi-arc graph structure. Evaluation focuses on coherence, not dramatic quality. Heavy compute requirements for large agent societies.

---

### Intra — Design Notes on an LLM-Driven Text Adventure

**Ian Bicking, blog post, July 2025** — [playintra.win](https://playintra.win) — [Blog post](https://ianbicking.org/blog/2025/07/intra-llm-text-adventure) — [GitHub](https://github.com/ianb/intra)

Not a paper but a practitioner's design log from building a working LLM text adventure. Valuable as ground-truth evidence of problems Rhapsode is designing to solve.

**Central thesis: ground truth vs. narrative demand.** Bicking draws a sharp line between collaborative storytelling (AI Dungeon, Character.ai) and a game with formal state. He defines "narrative demand" as a setup that forces a conclusion to satisfy it — e.g., "I gloat after disarming my opponent" implies disarmament happened. Without ground truth, the player's meta-game becomes nudging the narrative to make success a demand. Bicking's solution: rooms, NPCs, exits, and plot elements with formal states exist outside the LLM's narrative.

**Architecture.** TypeScript, entirely client-side (no backend). Game loop sends each event/task to an LLM, processes results with inline XML markup (not tool calls). State: player, NPCs, rooms/exits, plot elements with formal states. No tools — uses inline XML tags in text responses (`<dialog>`, `<action>`, `<removeRestriction>`).

**Input rewriting.** All player input passes through an intent parser (separate LLM call) before action resolution. "say hello" → `<dialog>Hello!</dialog>`, "open the door" → `<action>Player attempts to open the door</action>`. Critically, "Marta and Ama get into a disagreement" → `<action>Player attempts to provoke a disagreement</action>` — the rewrite prevents players from asserting outcomes.

**Action resolution.** Separate LLM prompt with guided thinking (structured series of questions): is this possible? Is it trivial? What happens on success/failure? Rate difficulty. Use a d20 roll at discretion. This separates "what the player does" from "what the player attempts to do."

**Guided thinking.** Instead of letting the LLM invent its own reasoning process, the prompt provides a fixed series of questions the LLM must answer in order. This forces consideration of specific factors and commits the LLM to conclusions before they're needed downstream. Equivalent to our Director's structured per-turn operations, implemented as prompt engineering.

**Individual perspectives.** Event log is filtered per NPC: they only see events in their room, don't hear private conversations, and have access to their own thoughts/history. Lightweight information asymmetry without explicit knowledge-state metadata.

**"The user is the game engine."** In LLM prompts, `role:user` is the game engine, not the player. The LLM assists the engine in resolution; the player's input is data to the engine. Subtle but important framing for prompt design.

**Relevance to Rhapsode.**
- Validates ground-truth-first architecture. Bicking's minimal implementation (rooms + NPCs + states) is what Rhapsode's plot graph generalizes into a full tension DAG.
- Input rewriting as intent parser: worth considering as a preprocessing step between freeform input and the SceneLoop. Prevents players from creating false narrative demands during "on-edge" freeform phases.
- Action resolution with dice rolls: simple non-determinism for freeform actions without a full rules engine.
- Event filtering per NPC: lightweight alternative/complement to our explicit `KnowledgeState` metadata.
- Guided thinking: validates structured question chains over open-ended reasoning for game-specific LLM prompts.

**What Bicking discovered he needed (and we already designed).**

| Bicking's "further direction" | Rhapsode equivalent |
|---|---|
| NPC memory — "critical memories can easily be lost in time" | Memory system with importance scoring |
| Off-screen simulation | World-background loop (Director) |
| "Plot arcs modeled as entities" | Plot graph — tensions as first-class nodes |
| NPC autonomy — "right now NPCs are very reactive, effectively zombies" | NPC autonomy via off-screen goals (Director rule 3) |
| Force LLM brainstorming (list of responses, pick best) | Multiple candidate plans / constrained choices |
| Context-window management, summarization | Three-layer memory with bounded STM rewrite |
| Integrate changes into core descriptions | `consequences: list[Mutation]` on tension nodes |

**Limitations.** Incomplete game (self-described). No persistent memory beyond event history. No plot structure beyond hard-coded mysteries. No NPC autonomy. No off-screen simulation. Single-player only. All the things Bicking lists as "further directions" — essentially the problem space Rhapsode is designed to solve.

---

## Key warnings from the literature

1. **Pure simulation is boring.** Generative Agents confirms that emergent behavior without dramatic structure produces realistic but dramatically flat output. The Director is essential.
2. **LLM-based objective checking is unreliable.** IBSEN's F1 ≈ 0.77 for objective completion verification. Graph-topology-derived predicates are more reliable at critical transitions.
3. **Structural suspense cues are noisy.** Only ~55% of annotators perceived the "decreasing likelihood" ordering in Suspenseful Stories. Don't over-rely on structural heuristics for pacing.
4. **Foreshadowing subtlety is a prompt engineering problem, not a structural one.** CFPG's trigger gating controls *when* payoffs appear, but how subtle the foreshadowing *itself* is depends entirely on model writing quality.
5. **Long-form coherence remains unsolved at scale.** RecurrentGPT evaluates only to ~5,000 words quantitatively. FACTTRACK tests on ~3,000 word outlines. Real RPG sessions may run 50,000+ words.

## References

| Short name | Full citation |
|------------|--------------|
| IBSEN | Han et al. "IBSEN: Director-Actor Agent Collaboration for Controllable and Interactive Drama Script Generation." ACL 2024. [arXiv:2407.01093](https://arxiv.org/abs/2407.01093) |
| StoryVerse | Wang, Zhou, Ledo. "StoryVerse: Towards Co-authoring Dynamic Plot with LLM-based Character Simulation via Narrative Planning." FDG 2024. [arXiv:2405.13042](https://arxiv.org/abs/2405.13042) |
| CFPG | Yun et al. "Codified Foreshadowing-Payoff Text Generation." ArXiv 2026. [arXiv:2601.07033](https://arxiv.org/abs/2601.07033) |
| RecurrentGPT | Zhou, Jiang et al. "RecurrentGPT: Interactive Generation of (Arbitrarily) Long Text." ArXiv 2023. [arXiv:2305.13304](https://arxiv.org/abs/2305.13304) |
| Generative Agents | Park et al. "Generative Agents: Interactive Simulacra of Human Behavior." UIST 2023. [arXiv:2304.03442](https://arxiv.org/abs/2304.03442) |
| FACTTRACK | "FACTTRACK: Time-Aware World State Tracking in Story Outlines." NAACL 2025. [arXiv:2407.16347](https://arxiv.org/abs/2407.16347) |
| Suspenseful Stories | Xie & Riedl. "Creating Suspenseful Stories: Iterative Planning with Large Language Models." EACL 2024. [arXiv:2402.17119](https://arxiv.org/abs/2402.17119) |
| EvoSpark | "EvoSpark: Endogenous Interactive Agent Societies for Unified Long-Horizon Narrative Evolution." ACL 2026. [arXiv:2604.12776](https://arxiv.org/abs/2604.12776) |
| Intra | Bicking, Ian. "Intra: design notes on an LLM-driven text adventure." Blog post, July 2025. [Blog](https://ianbicking.org/blog/2025/07/intra-llm-text-adventure) / [GitHub](https://github.com/ianb/intra) |
| awesome-llm-story-generation | Picrew. "A curated list of LLM papers for story generation." [GitHub](https://github.com/Picrew/awesome-llm-story-generation) |
