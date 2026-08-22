---
sources:
  - experiments/session_pipeline/runs/default-guide-300/manifest.json
  - experiments/session_pipeline/runs/default-guide-300/report.json
  - experiments/session_pipeline/runs/default-guide-300/turns.jsonl
  - experiments/session_pipeline/runs/default-guide-300/console.log
  - experiments/session_pipeline/runs/default-guide-300/saves/konosuba.json
  - experiments/session_pipeline/runs/default-guide-300/saves/story.json
  - experiments/session_pipeline/runs/default-guide-300/saves/world.json
  - experiments/session_pipeline/guides/default.md
  - experiments/session_pipeline/player_agent.py
  - server/scenarios/konosuba.json
  - core/include/rhapsode/character_memory.h
  - core/include/rhapsode/json_util.h
  - core/include/rhapsode/node.h
  - core/include/rhapsode/scene_data.h
  - core/src/scenario_bootstrap.cpp
  - core/src/director.cpp
  - core/src/narrator_prompt.cpp
  - core/src/node.cpp
  - core/src/read_tools.cpp
  - core/src/story.cpp
  - core/src/turn_executor.cpp
  - core/src/turn_executor_narrator.cpp
  - core/src/turn_executor_post_turn.cpp
  - core/src/text_downsampling.cpp
  - core/src/world_analysis.cpp
  - core/src/world.cpp
  - core/src/character_memory_reflection.cpp
  - core/src/eval/session_eval.cpp
  - core/src/story_advance.cpp
  - core/src/storyline_policy.cpp
last_updated: 2026-08-12
confidence: verified
tier: episodic
related:
  - "[[research/motif-collapse-default-guide-300]]"
  - "[[architecture/monologue-streams]]"
  - "[[architecture/scene-loop]]"
  - "[[architecture/plot-graph]]"
  - "[[research/narrator-and-mind-weakpoints]]"
  - "[[research/frontier-llm-long-horizon-orchestration]]"
tags:
  - cross-layer
  - research
  - design
crystallized_from: "Discussion and second-pass audit of default-guide-300"
---

# Episode: Default Guide 300 - Coherent Stagnation and Character Convergence

## Current conclusion: model choice does not solve the stability problem

Current frontier models can produce convincing short-term storytelling, but there is no reliable published evidence that any of them can control narration, characters, environment, and evolving causal state across a long open-ended run. A separate literature audit also found no local model trained on one fictional universe that outperforms the strongest frontier models at that complete job. Frontier models are therefore the best available generators, not sufficient or trustworthy world simulators.

Let \(T_M\) be the first turn at which model \(M\) introduces a damaging persistent error, and let \(H\) be the desired run length. The probability that the run reaches \(H\) without such an error is

\[
S_M(H)=\Pr(T_M>H)=\prod_{t=1}^{H}(1-p_t),
\]

where \(p_t\) is the probability of a damaging error at turn \(t\), conditional on the run having survived the preceding turns. This formulation does not assume independent turns. Once an erroneous interpretation is committed to memory as fact, later generations condition on it and subsequent \(p_t\) values may increase. That is the observed spiral mechanism.

Rhapsode therefore cannot safely make either a frontier or local LLM the sovereign causal system. The frontier model should propose narration, dialogue, and possible state changes; the surrounding system must decide what becomes durable, retain provenance, and support correction and recovery. This does not require code to prescribe the story. It requires the system to prevent one uncertain model judgment from silently becoming irreversible world truth.

The 300-turn run stayed readable and completed without runtime errors, yet it converged on one candle-lit emotional mode. The main failure was coherent stagnation. Unfinished events lost pressure, promised times caused nothing, and distinct characters increasingly expressed the same group sentiment.

## Question

Did Rhapsode preserve story movement and distinct character voices across `default-guide-300`? If not, did the frontier model fail, or did the surrounding system remove the information and agency needed for long-form performance?

This episode evaluates one run and the code path that produced it. It does not claim that every frontier model must fail after a fixed number of turns.

## Findings

### What worked

The run reached all 300 requested turns with `end_reason=MaxTurns`, zero errors, and zero timeouts. The prose retained local continuity, stable names, and a coherent emotional theme. These are meaningful strengths for a day-scale autonomous run (`experiments/session_pipeline/runs/default-guide-300/manifest.json`).

The evaluator added 300 turns to a resumed save already at engine turn 1; the final scene has `turn_index=301`. This was a 300-turn evaluation, not a fresh turn-0 launch (`experiments/session_pipeline/runs/default-guide-300/console.log`; `experiments/session_pipeline/runs/default-guide-300/saves/konosuba.json`).

The automated report found five empty beats and a large narration-length decline of `0.723`. Its adjacent-message repetition score remained only `0.136` (`experiments/session_pipeline/runs/default-guide-300/report.json`). Manual review found repeated meaning expressed through new wording, which that word-overlap metric did not measure.

### How the execution loop abandoned the farm quest

This did **not** begin as a memory failure. At turn 24, the player still says, "I'll be at the fence at dawn." At turn 25, while that promise is still in the newest context, the player instead opens a quiet conversation with Luna (`experiments/session_pipeline/runs/default-guide-300/turns.jsonl:24-25`). The quest was remembered but had no control over what happened next.

The full causal chain was:

`promise becomes non-current -> no live goal -> no dawn trigger -> romance dominates recent context -> invented completion passes -> old evidence leaves the automatic prompt`

1. **The promise entered the graph already closed.** After committing the turn-7 beat, the graph pass created node `#55`: "The party will depart at dawn..." The node has both `created_at=7` and `valid_until=7`: it was marked current only through its birth turn and non-current afterward (`experiments/session_pipeline/runs/default-guide-300/saves/world.json`). In code, a new node labelled `resolved` is stored as active with `valid_until` set to the current turn (`core/src/narrator_prompt.cpp:145-163`; `core/src/director.cpp:50-57,111-135`; `core/src/node.cpp:18-30,51-69`). Transitions run before new-node creation, and the later expiry pass logged no expiration. The expiry therefore arrived in the new-node data. The raw response body is missing, so its exact field is not provable; the schema-supported and most likely cause is `state="resolved"`. That confuses **the characters finished agreeing** with **the characters finished the work** (`experiments/session_pipeline/runs/default-guide-300/console.log`).

2. **An observation never became a goal that could drive the scene.** Node `#55` remained an ordinary plot fact. Routing it into character memory could create a perception or belief, but not the `intention` type that drives the storyline board (`core/src/world.cpp:192-219`; `core/src/character_memory_reflection.cpp:177-187`; `core/src/story.cpp:364-397`). The final scene accordingly has an empty `driving_intention`, `charge=0`, and `intention_node_id=0` (`experiments/session_pipeline/runs/default-guide-300/saves/konosuba.json`). The narrator receives graph facts only through an optional tool call. Even exact `query_graph("Player")` would omit `#55`: 25 older Player facts already existed, while exact entity lookup keeps only the oldest 20 (`core/src/narrator_prompt.cpp:39-92,121-126`; `core/src/world_analysis.cpp:40-57,78-110`). Some party-member queries could return it, but as an expired fact.

3. **Nothing made dawn arrive.** `turn_index` counts generated beats, not time inside the fiction. The scene has no due-time field, and the lifecycle can only fork, merge, conclude, or remove cast members; it cannot turn "at dawn" into a mandatory next event (`core/include/rhapsode/scene_data.h:18-31`; `core/src/storyline_policy.cpp:29-57`). The models could therefore spend another turn in the same evening indefinitely without violating any engine state.

4. **The player and narrator then reinforced the detour.** The autonomous player receives the latest 12 displayed story messages and the storyline board. Its guide does say to advance stakes and not stall, but its more specific arc asks for warm slice-of-life, room for romance, and no hard quests. That biases rather than determines the choice (`core/src/eval/session_eval.cpp:238-251`; `experiments/session_pipeline/guides/default.md:3-25`). The player then makes a one-shot call with no graph or history tool loop (`experiments/session_pipeline/player_agent.py:163-202`). The narrator receives the current move, six recent history messages, the compressed past, the storyline board, and any optional tool results. Turn 25's Luna conversation therefore becomes fresh input to both models; each new emotional beat makes another similar beat locally easier to continue. No pending quest or dawn trigger supplies an opposing signal.

5. **A false completion entered through an unchecked input.** At turn 98, the player says the party nearly died fighting the toad "yesterday," although the fight never occurred. The player validator checks response form and NPC impersonation; the narrator validator checks cast and NPC speech. Neither checks whether a claimed event happened (`experiments/session_pipeline/player_agent.py:122-146`; `core/src/turn_executor_narrator.cpp:92-195`). The narrator accepts the premise. The graph pass runs afterward, when the prose is already committed, and records seven candle-covenant developments but neither records nor challenges the invented fight (`core/src/turn_executor_narrator.cpp:315-356`; `experiments/session_pipeline/runs/default-guide-300/turns.jsonl:98`; `experiments/session_pipeline/runs/default-guide-300/saves/world.json`).

6. **Compression later made recovery harder.** By the final save, the downsampling state no longer contained automatic summary coverage for history indices `0-506`, including the signed quest. The raw `scene.history` still retained all 603 messages, and `query_history` could search them, but they would re-enter narration only through an optional tool call (`core/src/read_tools.cpp:33-47`). This loss from the default prompt did not initiate the abandonment—the turn-24-to-25 pivot proves otherwise—but it made the romantic continuity easier to preserve than to dislodge.

In short: the system observed the promise, probably misclassified it as finished, never converted it into a scheduled goal, rewarded the immediately available romantic continuation, and then accepted a fictional completion. The stationary story was produced by this control loop; it was not merely a list of events the model happened to forget.

### One narrator gradually aligned four characters

The beat narrator writes every NPC's exact line in `speech_turns`; no character process proposes an action before that composition (`core/src/narrator_prompt.cpp:110-142`). Character monologues update afterward from the already-written beat (`core/src/turn_executor_post_turn.cpp:37-65`). Their direct stimulus contains player input and narrator prose, but not the exact NPC dialogue just generated. Every on-stage character receives that same public beat stimulus (`core/src/world.cpp:225-240`). They can interpret or reinforce the scene, but they cannot independently redirect it before it exists.

| Character | Result across the run |
|---|---|
| Aqua | Her complaints, divine status, and threats survive best. She nevertheless becomes unusually gentle and emotionally articulate. |
| Megumin | Her Crimson Demon vocabulary remains recognizable. Her range narrows into chronicling, naming, and ratifying the candle covenant. |
| Darkness | Her duty-masochism contradiction is vivid near turn 7. Later she mostly becomes a generic protective "wall." |
| Luna | Her precise receptionist voice drifts furthest. Debt figures, rules, and dry administrative retaliation become romantic, therapeutic prose. |

Turn 287 is the clearest convergence. Megumin wants a chronicle with no final page. Darkness wants them to be last at the table every night and resist morning. Aqua wants everyone to remain, and Luna describes a quest nobody turns in. Their nouns differ, but all four express the same desire (`experiments/session_pipeline/runs/default-guide-300/turns.jsonl:287`).

This does not prove that the frontier model lacks those voices. Early, character-specific circumstances produce sharp Megumin and Darkness performances. The run did not isolate model capacity from shared authorship, shared context, or memory retrieval.

### Mutable cores and recent monologues can amplify drift

`query_mind` supplies at most 600 bytes of `CharacterCore`, but up to three 400-byte lines from every active monologue stream. It also supplies up to 1,200 bytes of beliefs (`core/src/character_memory_reflection.cpp:243-269`). Recent interpretation can therefore occupy much more of the tool result than durable identity.

Core mutation has no evidence threshold or cooldown. Any non-empty `core_revision` returned by the post-turn actor replaces the whole core (`core/src/character_memory_reflection.cpp:309-316`). Luna's saved core shows `revised_at=208`; the replacement makes candle-covenant membership part of her identity (`experiments/session_pipeline/runs/default-guide-300/saves/world.json`). The original scenario core instead centers administrative power, finite patience, loneliness, and quiet retaliation (`server/scenarios/konosuba.json:258-267`).

Ordinary scenario beliefs also decay quickly when untouched. Their default weight is `4`, every successful update multiplies it by `0.9`, and values below `0.05` expire. After 42 successful untouched updates,

\[
4(0.9)^{42} \approx 0.0479 < 0.05.
\]

The numbers are respectively the initial weight, per-update retention, update count, and expiry threshold. A motive encoded only as an ordinary belief can therefore disappear after about 42 on-stage updates (`core/src/scenario_bootstrap.cpp:60-83`; `core/src/character_memory_reflection.cpp:17-23,55-92`).

### The long-history channel is an information bottleneck

Once a summary exists, and ignoring fixed instructions, the narrator's changing input at turn \(t\) is:

\[
N_t = \bigl(C_t, B(H_t), R_t, O_t\bigr).
\]

- \(N_t\) is the context that can affect narration at turn \(t\).
- \(C_t\) is the current cast and live-storyline board.
- \(H_t\) is older player-and-narrator history.
- \(B(H_t)\) is that history after recursive summarization and a 1,500-byte prefix limit.
- \(R_t\) is the six newest player-and-narrator messages, each limited to 400 bytes. It includes the current player input.
- \(O_t\) is any graph, mind, or history result the narrator chooses to request.

These limits are implemented in `core/src/narrator_prompt.cpp:14-16,74-92`; truncation is explicitly byte-based in `core/include/rhapsode/json_util.h:54-60`.

The rendered downsampling summary contains 6,206 characters, or 6,210 UTF-8 bytes. Its retained snippets begin at history index 507, so the automatic summary no longer represents indices 0-506, including the signed quest. Raw `scene.history` still contains all 603 messages and remains searchable through `query_history`. Of the summary, the prompt admits only the first 1,500 bytes: indices 507-530 and the start of the summary for 531-542. It hides every summary for indices 543-596. Indices 597-602 enter separately as the six-message tail (`experiments/session_pipeline/runs/default-guide-300/saves/konosuba.json`; `core/src/text_downsampling.cpp:172-180`; `core/src/read_tools.cpp:33-47`).

This compression is many-to-one: distinct histories can produce the same retained memory. Two different older histories can satisfy

\[
H_t \ne H'_t \quad\text{while}\quad B(H_t)=B(H'_t).
\]

If their cast, recent tail, and tool results also match, the narrator cannot condition on the lost difference. This proves an information bottleneck. It does not prove that the story lies on a mathematical low-dimensional manifold. A narrow, self-reinforcing story mode is a plausible consequence, not a theorem from the byte cap alone.

A follow-up literature audit found no demonstrated system that repairs this by repeatedly rebuilding an open-ended story from bounded records without human correction. Existing systems show that such packets can be built, not that they preserve the right narrative working set over successive handoffs. See [Repeated bounded-context reconstruction](../research/frontier-llm-long-horizon-orchestration.md#repeated-bounded-context-reconstruction).

### Dialogue and the graph preserve less than they appear to

NPC dialogue is not wholly discarded. The same-turn graph pass receives `speech_turns`, so it can extract facts from what characters said (`core/src/turn_executor_narrator.cpp:217-228,341-352`). The exact lines also remain in the saved `scene.dialogue` buffer.

Normal future narration is different. User input and narrator prose enter `scene.history`, while NPC lines enter `scene.dialogue` (`core/src/turn_executor.cpp:323-346`). Narrator history, downsampling, post-turn monologues, and normal `query_history` read `scene.history`, not the dialogue buffer (`core/src/story_advance.cpp:47-61`). Later turns therefore receive prior speech only through a lossy graph fact, a narrator paraphrase, or reconstruction.

The final world graph contains 1,007 nodes: 527 have `valid_until=-1`, while 480 have expired. The `state="active"` label does not mean a node remains true; validity is a separate field (`core/include/rhapsode/node.h:15-33`). Exact entity lookup sorts by `created_at` ascending and returns the oldest 20 facts, so newer facts are deterministically omitted once an entity has a long timeline (`core/src/world_analysis.cpp:40-57,78-110`).

The graph pass also parsed as empty on 116 of 300 turns (`experiments/session_pipeline/runs/default-guide-300/console.log`). The graph is therefore a lossy record written after narration. It can remain observation-only, but it cannot also be treated as the authority for unfinished goals, story time, or independent character action.

### Most supported failure chain

The artifacts and code support this explanation:

1. The player brief selects a quiet romantic direction.
2. One narrator writes every character into that shared direction.
3. Post-turn monologues interpret the same public beat, and an unrestricted core revision can make the direction durable.
4. Downsampling removes the original plot from the automatic summary, though raw history remains tool-searchable. Prefix truncation keeps an older covenant-saturated slice and hides newer middle summaries. Exact graph lookup also hides newer facts behind older ones.
5. No goal tracker, time trigger, or pre-narration character action applies an opposing force.

This is an evidence-backed mechanistic explanation, not a controlled test that changes one component at a time. The run shows that this system failed at long-form dynamics. It does not separate every contribution from the model, prompt, guide, and memory policy.

## Lessons

Four responsibilities should be separated from the observation-only world graph:

1. Track unfinished goals with a status, due condition, and next expected step.
2. Let each character propose an intention or action before the narrator composes the shared scene.
3. Recall attributed dialogue and retrieve the newest facts that are still current.
4. Keep `CharacterCore` mutable, but require supporting evidence, source turns, and a deliberate threshold for replacement.

The smallest useful evaluation should also measure unfinished-goal progress, promised-time execution, and separation between character voices. Adjacent word overlap cannot detect polished paraphrases of the same emotional beat.

## Questions for the next discussion

- What is the smallest goal record that prevents abandonment without turning the world graph into a planner?
- Should characters propose only actions, or both actions and candidate dialogue, before narration?
- What evidence should justify a core revision, and should a revision replace the core or amend it?

## Wiki Impact

- Created this episodic analysis of `default-guide-300`.
- Added it to the wiki index and concept routing.
- Recorded the audit in `wiki/log.md`.
- Made no code change or final architecture decision; the design questions remain open.

## See Also

- [[research/frontier-llm-long-horizon-orchestration]]
- [[research/motif-collapse-default-guide-300]]
- [[architecture/monologue-streams]]
- [[architecture/scene-loop]]
- [[architecture/plot-graph]]
- [[research/narrator-and-mind-weakpoints]]
