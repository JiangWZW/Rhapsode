---
sources:
  - experiments/session_pipeline/runs/default-guide-300/
  - wiki/episodes/2026-08-10-long-run-storyline-and-character-collapse.md
  - "arxiv:2608.08160"
  - "doi:10.18653/v1/2025.acl-long.546"
  - "arxiv:2502.19519"
  - "arxiv:2506.13356"
  - "arxiv:2501.09099"
  - "doi:10.18653/v1/2024.acl-long.747"
  - "doi:10.18653/v1/2024.acl-long.88"
  - "doi:10.1109/COG60054.2024.10645607"
  - "arxiv:2507.15518"
  - "arxiv:2607.00918"
  - "arxiv:2606.13348"
  - "doi:10.1145/3586183.3606763"
  - "arxiv:2410.10813"
  - "arxiv:2502.13270"
  - "doi:10.1609/aaai.v40i36.40288"
  - "doi:10.18653/v1/2022.emnlp-main.296"
  - "github:yingpengma/NCP-Bench@89f7ef1d02c4b3eb6be409b037a05b285baa3ec4"
  - "github:gingasan/interactive-drama@db3a4df508aaecf70fea23dcddd660de8ced7cfb"
  - "github:KarmaKamikaze/ChatRPG@f501e13b5d12f3690af09be9ddb3f6b1cddd8041"
last_updated: 2026-08-13
confidence: verified
tier: semantic
related:
  - "[[research/motif-collapse-default-guide-300]]"
  - "[[architecture/pragmatic-turn-transaction-refactor]]"
  - "[[architecture/monologue-streams]]"
  - "[[architecture/plot-graph]]"
  - "[[research/narrator-and-mind-weakpoints]]"
tags:
  - research
  - third-party-analysis
  - design
  - cross-layer
  - memory-architecture
---

# Frontier LLM orchestration over 300 interactive turns

No published system demonstrates reliable operation across 300 free-form player turns for Rhapsode's complete task. The only verified 100-turn stress test reports steep failure.

The strongest architecture hypothesis is a layered transaction, not a larger prompt or another general validator. Characters propose exact actions and dialogue. The orchestrator declares accepted consequences before prose. Code owns only narrow mechanics, obligations, time, and versioned commits.

Verbatim evidence remains available for targeted retrieval. Semantic summaries and the [[architecture/plot-graph|WorldGraph]] help locate or interpret evidence, but neither becomes sovereign world state.

## Reliability boundary

One player turn is one free-form player input followed by one committed world response. Model calls, NPC messages, pages, scenes, and choice-graph nodes do not count.

For benchmark distribution \(B\) and failure class \(C\), define:

\[
S_{B,C}(H)=\Pr(T_C>H),
\]

where \(T_C\) is the first player turn containing a persistent class-\(C\) failure. The benchmark must state its scenarios, player policy, model, prompts, and adjudication policy.

Persistent failures include:

- contradictions later adopted as fact;
- ignored, overwritten, or silently rewritten player actions;
- premature obligation closure or unexplained abandonment;
- invalid location, possession, access, or resource changes;
- character voices or motives collapsing into one shared stance;
- causal stalling hidden by fluent prose;
- state corruption that survives into later turns.

Completion, enjoyment, and LLM-judge scores are separate outcomes. A run may finish a script while failing reliability on its first player turn.

## Evidence taxonomy

| Class | Player input | State and future | Valid inference for Rhapsode |
|---|---|---|---|
| Character chat | Free dialogue with a persona | Conversation memory; little causal world state | Persona recall and attribution only |
| Autonomous drama | No player, or bounded intervention | Director and actors follow a blueprint | Actor coordination and anti-stall behavior |
| Scripted interactive story | Fixed choices or fixed scenes | Authored graph determines legal futures | Recovery inside an explicit state machine |
| Coded simulation | Commands over executable mechanics | Code owns a narrow state schema | Mechanical validation and state ownership |
| Finite generated story | No player | Outline or completed simulation supplies the future | Planned prose and bounded handoffs |
| Evolving interactive world | Free actions change later possibilities | Mixed causal and semantic state | The complete Rhapsode target |

Evidence does not transfer automatically between rows. Long context, RAG, graphs, and multi-agent decomposition are mechanisms, not reliability results.

## Search and audit method

The search started with work released or revised after the earlier review. Backward citations then led to established actor, memory, simulation, and prose systems.

Primary papers were read in full. Released prompts, memory code, state transitions, and evaluation harnesses were inspected when available. Repository claims below refer to fixed commits.

The audit records actual player inputs. It rejects aggregate dialogue turns, graph size, pages, and model calls as horizon substitutes.

Citation maturity describes independent uptake and venue status. It does not raise the evidentiary strength of a mismatched task.

## Primary evidence comparison

`NR` means the paper did not report the number of sequential player inputs per run.

| Class and source | Venue, date, affiliations | Sample and models | Actual sequential player horizon | Input and durable state | Repository and citation maturity | Reliability result |
|---|---|---|---:|---|---|---|
| Evolving world: [NCP-Bench](https://arxiv.org/abs/2608.08160) | arXiv v1, 2026; Macau, Westlake, HIT, Cambridge, Aberdeen. It self-reports ICML/PMLR 306. | 100 movie environments; six narrator models; 600 adversarial runs. Narrators include GPT-5.2, GPT-4o-mini, DeepSeek-V3.2, Qwen3-235B-A22B, Kimi-K2.5, and Grok-4.1-Fast. | 100-turn cap | Free-form adversarial or natural simulated player; extracted facts and fixed trajectory commitments | [Code and prompts at `89f7ef1`](https://github.com/yingpengma/NCP-Bench/tree/89f7ef1d02c4b3eb6be409b037a05b285baa3ec4). Very recent; no live PMLR record was verified. | GPT-5.2 conflict-free survival is 0.42 at turn 20. Only 21 of 600 runs reached turn 100 conflict-free. |
| Evolving world: `default-guide-300` | Rhapsode artifact, 2026 | One frontier-model auto-player run | 300 | Free-form generated moves; mutable CharacterCore, history, WorldGraph, and story state | Complete local transcript and saves; not a publication | The obligation was already closed at turn 7. The run pivoted near turns 24-25 and accepted a false completion at turn 98. |
| Interactive drama: [Enhanced Immersion and Agency](https://doi.org/10.18653/v1/2025.acl-long.546) | ACL 2025; Shanghai Jiao Tong and Zurich | 60 play histories over one fixed three-scene drama; GPT-4o | NR | Free-form human or simulated player; flattened actor memory and fixed script | [Code at `db3a4df`](https://github.com/gingasan/interactive-drama/tree/db3a4df508aaecf70fea23dcddd660de8ced7cfb). Peer reviewed; emerging citations. | Actor reflection improved local human-rated agency. Future-script leakage and invented locations remained. |
| Evolving RPG: [ChatRPG](https://arxiv.org/abs/2502.19519) | arXiv v2, 2025; Aalborg University | 8-person pilot and 12-person comparison; ChatGPT-4 | NR | Free-form human play; narrator tools plus post-prose archivist | [Code at `f501e13`](https://github.com/KarmaKamikaze/ChatRPG/tree/f501e13b5d12f3690af09be9ddb3f6b1cddd8041). Preprint; limited citation maturity. | Preference and immersion improved. No turn survival, first-failure, or state-integrity result was reported. |
| Coded simulation: [Player-Driven Emergence](https://doi.org/10.1109/COG60054.2024.10645607) | IEEE CoG 2024; Microsoft Research | 28 one-hour participants; GPT-4 | At most 30 per persistent world; 75 actions average across resets | Free dialogue and actions over TextWorld; world and NPC memories reset every 30 actions | Supplement promised; no standalone repository was verified. Established conference paper. | Supports narrow coded mechanics. It does not test persistent NPCs or one evolving 75-turn world. |
| Coded story world: [IVIE](https://arxiv.org/abs/2606.13348) | ICCC 2026; Universidad de la Republica | 16 tiny worlds; eight evaluators; frontier APIs | NR | Free-form play over four locations, four items, two NPCs, and one puzzle | [Code at `a6e35e7`](https://github.com/micaelavaucher/IVIE/tree/a6e35e761cd36ed83b413053d52997e49b2c5e06). Very recent. | Two worlds passed validation with inaccessible items. Three accepted claimed puzzle completion. |
| Scripted drama: [IBSEN](https://aclanthology.org/2024.acl-long.88/) | ACL 2024; Shanghai Jiao Tong and Suzhou Laboratory | Ten autonomous plays and five player plays; 785 aggregate dialogue turns | NR for player inputs; nine dialogue turns per objective cap | Restricted player intervention; director and character actors | [Code at `ad63221`](https://github.com/OpenDFM/ibsen/tree/ad63221171c2e76b32034b0d253ee77e17f6f038). Peer reviewed; moderate maturity. | Actors own final speech. The objective checker has F1 0.77, and unresolved objectives are force-completed. |
| Autonomous drama: [HAMLET](https://arxiv.org/abs/2507.15518) | ICLR 2026; ECUST, Sydney, TeleAI, Datawhale | 100 drama cases; frontier LLM actors | 0 player turns; 30 actor turns per point without advancement | Blueprint, shared props, actor actions, and an Advancer | Repository revision contains a README and poster only. Recent peer-reviewed work. | Advancer removal reduced completion from 100% to 68.1%. Completion is not player-world reliability. |
| Finite prose: [MAGNET](https://arxiv.org/abs/2607.00918) | arXiv, 2026; Pocket FM and university collaborators | Three short stories and one 100-page proof; frontier LLMs | 0 | Character action proposals, critic-selected updates, then narrator prose | Anonymous code links; no durable commit. Very recent. | Supports action and state proposals before prose. It has no player and only one long sample. |
| Character chat: [LoCoMo](https://aclanthology.org/2024.acl-long.747/) | ACL 2024; UNC, USC, Snap | Ten conversations; GPT-3.5 generation and frontier retrieval baselines | 0 player turns; 588.2 generated dialogue turns average | Scheduled future events, recursive summary, attributed observations, raw dialogue | Data and code released. Mature ACL paper with hundreds of citations. | Nearly 15% of generated turns required human correction. Autonomous survival cannot be derived. |
| Scripted choices: [StoryBench](https://arxiv.org/abs/2506.13356) | Preprint, 2025; Tsinghua AIR and UESTC | One graph with 311 scenes and 86 choice nodes; four models; five trials per model condition | NR | Model selects authored options; code owns branches and endings | No dedicated repository was found. Preprint with limited maturity. | GPT-4o completed 0/5 original self-recovery trials and 2/5 improved trials. |
| Authoring system: [Drama Llama](https://arxiv.org/abs/2501.09099) | Preprint, 2025; Midjourney Storytelling Lab, sadstory.gg, Northwestern | Six authors; two authoring tasks each; generation model not reported | NR; representative transcripts average about 25-28 generated messages | Player character plus LLM characters; natural-language storylet triggers | No complete runtime repository was verified. Preprint with limited maturity. | Authors could reset earlier states. Anti-stall fallbacks remain proposed work. |
| Coded simulation: [Generative Agents](https://doi.org/10.1145/3586183.3606763) | UIST 2023; Stanford and Google | 25 agents over two simulated days; ChatGPT | 0 | Explicit clock, full memory stream, retrieval, reflection, daily planning | Public implementation. Highly mature with thousands of citations. | Location, retrieval, embellishment, over-cooperation, and interest-convergence failures remained. |
| Finite prose: [StoryBox](https://doi.org/10.1609/aaai.v40i36.40288) | AAAI 2026; Sun Yat-sen University | 20 settings; about 12,000 words each; released config uses GPT-4o-mini | 0 | Simulation finishes before five planned chapters are written | [Code at `ba24ff6`](https://github.com/amcghm/StoryBox/tree/ba24ff64e6023e8d071af4adb7e64b03bd9b08b1). Recent peer-reviewed work. | Supports finite prompt reconstruction after an offline simulation. No player changes the future during prose generation. |
| Memory benchmark: [LongMemEval](https://arxiv.org/abs/2410.10813) | ICLR 2025; UCLA, Tencent, UCSD | 500 QA cases; up to 500 sessions and about 1.5M tokens | 0 | Fixed conversation history; raw rounds or extracted keys | Code and data released. Mature benchmark. | Extracted keys improved raw-evidence retrieval. Replacing raw evidence with summaries or facts lost information. |

The table excludes several apparent long-horizon claims from player-turn counts. Re3's 12 passages, MAGNET's 100 pages, and StoryBench's 311 scenes are not player inputs.

## Benchmark-specific survival evidence

| Benchmark and endpoint | \(S(20)\) | \(S(100)\) | \(S(300)\) | Interpretation |
|---|---:|---:|---:|---|
| NCP-Bench, GPT-5.2, no detected conflict | 0.42 | Not separately reported | Not tested | Best published turn-20 estimate for free-form stress |
| NCP-Bench, six-model pool, no detected conflict | Not reported | 21/600 = 0.035 | Not tested | Pooled across unlike narrator models |
| NCP-Bench, natural GPT-4o-mini player, no detected conflict | Not reported | 19/100 = 0.19 | Not tested | None of the 19 satisfied every achievement |
| NCP-Bench, natural GPT-4o-mini player, all achievements satisfied | Not reported | 0/100 = 0 | Not tested | Progress and conflict survival differ |
| Rhapsode `default-guide-300`, one-run no-persistent-failure indicator | 0 | 0 | 0 | The turn-7 obligation error makes each later indicator zero. This is not a probability estimate. |

No credible source estimates \(S(300)\) for the complete task. Extrapolation from turn 20 would require stationary, independent hazards. Long stories provide neither condition.

## Primary-source and repository audit

### NCP-Bench: direct evidence with a railroaded baseline

NCP-Bench supplies 600 sequential free-form stress runs. Fact conflicts appear in 40-68% of runs across narrator models. Human experts disputed four of 100 fact judgments and no commitment or player-input judgments.

GPT-5.2 averages 32.92 turns before termination. Its fact-conflict rate is 40%.

The natural GPT-4o-mini player raises mean survival from 22.16 to 46.08 turns. Nineteen runs reach the cap, but none satisfy every achievement.

The baseline code removes all earlier player messages from the narrator prompt. It retains prior system responses and the current player input.

The released prompt calls trajectory advancement the "single highest priority." It ranks that priority above faithful execution of the player's attempted action.

Those choices weaken intent retention and player agency. The results remain a strong warning, but they are not an upper bound on different architectures.

HiAgent raises mean GPT-4o-mini survival from 22.16 to 30.05 turns. It reduces commitment conflicts from 26% to 4%.

The same intervention raises player-input conflicts from 13% to 38%. Complete achievement successes fall from two runs to zero.

The released method has a 12,000-token input budget. Each completed-subgoal summary is capped at 220 characters. This is direct evidence that lossy compression can trade global obligations against local intent.

The repository serializes checkpoints after committed boundaries and can resume generation. It does not demonstrate semantic rollback, replay from a sound version, or contamination removal.

### Character-authored output: local gains, unknown survival

The ACL interactive-drama system lets role agents produce their final actions and dialogue. A director supplies plots, and plot-based reflection can alter a character's reaction to player intent.

The study contains 60 histories over one fixed three-scene drama. It does not report the number of player inputs per history.

Human ratings show local agency gains. The failure analysis still reports future-script leakage and invention of a nonexistent tennis court.

The implementation flattens each actor's full memory into prompts. Its guarded scene-transition check is commented out. The `/next_scene` route advances directly in the inspected commit.

This supports exact character authorship as a mechanism. It does not show durable voice separation or causal reliability.

IBSEN reaches the same mechanism through a different scripted architecture. Its director gives direction while actors write final utterances.

IBSEN also supplies negative evidence. Its semantic objective checker reaches only F1 0.77, and the runtime forces unresolved objectives closed after nine turns.

### Consequences before prose: partial implementations

MAGNET asks characters for actions, lets a critic revise them, and selects related world updates before narrator prose. Only updates attached to selected actions enter state.

The result supports ordering and conditional commit. It contains no player, and its long result is one 100-page proof.

ChatRPG's ReAct narrator can call wound, heal, battle, character, and environment tools before producing final prose. This is closer to mechanical consequence declaration during live play.

The mutations occur immediately inside the narrator chain. The archivist then extracts character and environment state after narration.

The inspected code has snapshots but no atomic state-and-transcript transaction. It also lacks demonstrated rollback and replay after a bad tool call.

### Recovery: authored choices remain difficult

StoryBench contains 311 scenes and 86 total choice nodes. Those figures describe the graph, not one playthrough's player-turn horizon.

In the original self-recovery condition, GPT-4o completes 0 of 5 trials. Claude 3.5 Sonnet completes 2 of 5.

An improved condition raises GPT-4o to 2 of 5. Models usually backtrack only one or two choices.

The benchmark tests recovery within explicit authored branches. Completion still does not prove that intermediate state stayed reliable.

### Verbatim evidence survives summaries better

LoCoMo is the closest repeated session-handoff experiment. Its ACL version averages 588.2 dialogue turns over 27.2 sessions.

Humans corrected nearly 15% of generated turns before release. The system is character chat with scheduled events, not a player-driven evolving world.

LongMemEval separates retrieval keys from returned evidence. Extracted facts improve the search key, but raw rounds work better as returned values.

RealTalk reports the same direction with human dialogue. Replacing raw dialogue with human-authored event statements reduces GPT-4o multi-hop accuracy from 0.519 to 0.266.

Its temporal score falls from 0.737 to 0.501. Its commonsense score falls from 0.621 to 0.348.

Summaries, facts, and graph nodes should therefore route retrieval. They should not replace the attributed transcript.

## What the new audit adds

Four findings extend the earlier review:

1. NCP-Bench supplies actual 20-turn and 100-turn conflict-survival evidence. Its code also exposes player-history omission, forced progression, compression trade-offs, and checkpoint limits.
2. The ACL actor study supplies direct character-authored action and dialogue. Its reported gains remain local, and its scene-transition code bypasses the intended readiness guard.
3. ChatRPG supplies a live pre-prose mutation path. Immediate unversioned tool mutations prevent it from demonstrating atomic transactions or recovery.
4. StoryBench supplies a negative recovery result. Even explicit scripted branches and backtracking do not produce consistent completion.

No new source validates the complete mechanism stack. No source demonstrates 300 free-form turns without persistent failure.

## Mechanism-by-failure-mode matrix

| Proposed mechanism | Evidence for and against | Rhapsode failure addressed | Authority boundary | Falsification experiment |
|---|---|---|---|---|
| Character-authored exact actions and dialogue | IBSEN and the ACL study implement actor ownership. Neither reports long player horizons. | One narrator converged four voices into one stance. | Actor proposals remain inert until selected. Narrator may frame, but not paraphrase, accepted dialogue. | Blind raters fail to distinguish characters, or 100-turn voice survival does not improve against shared authorship. |
| Consequence declaration before prose | MAGNET selects actions and updates before prose. ChatRPG mutates before prose but lacks atomicity. | Post-prose extraction closed a live obligation and accepted an invented completion. | Orchestrator declares accepted semantic effects. Code validates only enumerated mechanics. | Predeclared effects still diverge from rendered prose, or mechanical conflicts do not fall against post-prose extraction. |
| Obligation, goal, and diegetic-time ledger | IBSEN and HAMLET need explicit progress controls. Their force-completion policies can hide stalls. | The dawn promise had no live goal, due trigger, or closure evidence. | Ledger owns open, due, escalated, satisfied, waived, and failed states. WorldGraph cannot close them. | Known promises remain unactionable, or explicit closure evidence does not reduce premature closure and abandonment. |
| Verbatim attributed archive with targeted retrieval | LongMemEval favors raw returned evidence. RealTalk and LoCoMo expose summary loss and attribution errors. | Exact NPC speech left normal history retrieval. A 1,500-byte summary hid older obligations. | Summaries and graph facts are retrieval keys. Raw player and speaker text remains immutable evidence. | Offline reconstruction misses transcript-verifiable obligations, quotations, or stances more often than the summary baseline. |
| Narrow symbolic state plus semantic narrative state | TextWorld and IVIE help with location and items. IVIE still accepts impossible semantic claims. | Mechanical facts and narrative implications were mixed into one lossy graph. | Code owns finite mechanics. Semantic stores retain provenance and uncertainty. | Symbolic checks fail to reduce enumerated invalid transitions, or the schema expands until it silently dictates prose. |
| Versioned patches, checkpoints, rollback, and replay | NCP-Bench demonstrates resume checkpoints only. No reviewed story system demonstrates contamination recovery. | Whole-core replacement made one interpretation durable. Later turns could inherit the error. | Every accepted patch has a base version, source turn, and inverse or replay path. | Injected corruption survives rollback, damages unrelated state, or replay cannot reproduce a valid continuation. |
| Visible anti-stall pressure | HAMLET's Advancer raises completion. MAGNET and IBSEN also use stall budgets. All risk railroading. | The farm quest lost pressure while fluent romance repeated. | Pressure enters as a visible in-world event. The player may reject the intended route. | Progress rises only because player refusals are overridden, or causal diversity does not improve. |
| Observation graph remains non-authoritative | The Rhapsode graph closed work early and parsed empty on many turns. IVIE shows semantic validation gaps. | An extractor's interpretation became control state. | WorldGraph records attributed observations with validity and provenance. It cannot own mechanics, obligations, time, or core versions. | Removing graph authority does not reduce false closure, or another semantic store recreates the same irreversible failure. |

The falsification tests are necessary conditions. Passing them does not establish 300-turn reliability.

## Ranked architecture hypotheses

Confidence refers to the mechanism's expected effect on its named failure. It does not mean confidence in 300-turn success.

| Rank | Hypothesis | Confidence | Reason for rank |
|---:|---|---|---|
| 1 | Narrow transactional authority | Moderate | Pre-prose ordering appears in MAGNET and ChatRPG. Atomic player-world commits remain untested. |
| 2 | Verbatim attributed archives plus targeted retrieval | Moderate-high for evidence preservation; low for story survival | LongMemEval and RealTalk directly favor raw evidence. Correct retrieval can still yield a bad continuation. |
| 3 | Character-authored exact action and dialogue proposals | Moderate for local voice; low for long horizons | Two actor systems implement ownership. Neither estimates voice survival across 100 player inputs. |
| 4 | Separate obligation, goal, and diegetic-time ledger | Moderate | Several systems require explicit progress machinery. Existing force-completion policies damage agency. |
| 5 | Versioned CharacterCore and world patches with rollback and replay | Moderate for containment; low for prevention | Versioning has clear engineering semantics. No reviewed narrative study measures recovered contamination. |
| 6 | Visible in-world anti-stall pressure with player refusal preserved | Moderate for movement; low for agency preservation | Advancers reduce scripted stalls. Their benefit under unrestricted refusal remains unknown. |
| 7 | Non-authoritative WorldGraph | High for avoiding the observed authority error; low for total reliability | The local failure directly implicates graph authority. Removing authority does not solve retrieval, voice, or progress. |

No reviewed system validates this combination. Multi-agent decomposition only assigns work; it does not guarantee correct proposals or commits.



## Architecture handoff

The active records, authority map, transaction stages, and gradual migration live in [[architecture/pragmatic-turn-transaction-refactor]].

That page treats the design as reversible experiments. The evidence here does not establish that the complete transaction reaches 300 turns.

## Evaluation plan

### Common protocol

Freeze the scenario distribution, player-policy distribution, model versions, prompts, and decoding settings. One campaign contains uninterrupted sequential player inputs into one evolving world.

Record first-failure turn for every class. Use human adjudication against the raw transcript and committed state.

Two blinded raters should label each suspected failure. A third resolves disagreements. LLM judges may triage candidates but cannot supply the final reliability label.

Report Kaplan-Meier survival by failure class. Also report first-failure distributions, confidence intervals, and censored runs.

Entertainment, voice quality, progress, agency, and reliability remain separate outcomes. Report them side by side rather than combining them.

### Gate 0: offline reconstruction

Use the existing 300-turn transcript before generating another long run. Sample boundaries before and after turns 7, 25, 98, 200, and 287.

At each boundary, reconstruct a fresh context packet. Test recovery of obligations, causal dependencies, attributed dialogue, character stances, mechanics, and diegetic time.

The packet must cite raw source turns. Failure rejects the retrieval design. Success only licenses continuation testing.

### Horizon 20: mechanism screening

Use adversarial boundary cases, ordinary play, refusal, false completion claims, time promises, and speaker-confusion probes. Count exactly 20 free-form player inputs per surviving campaign.

A 16-cell resolution-IV fractional factorial can screen seven binary mechanisms. Repeat each cell across at least 20 scenario-player blocks.

The screen estimates main effects and selected interactions. It is not a reliability claim.

### Horizon 100: finalist comparison

Advance only architectures that improve relevant failure classes without suppressing player choices. Include varied settings, player policies, seeds, and character counts.

Lock simulated player policies before the comparison. Add human campaigns to expose strategies absent from simulators.

Use the same scenario and player-policy blocks across finalists. Plot survival curves for facts, player intent, obligations, mechanics, voice, and stalls.

### Horizon 300: reliability claim

Run 300 uninterrupted player inputs without world resets. Checkpoints may support recovery, but a recovered fault still counts as a failure for prevention survival.

Report prevention survival and containment success separately. Do not hide recovered faults inside a failure-free label.

Zero failures in 29 independent campaigns gives a one-sided 95% lower confidence bound near 0.90. Zero failures in 59 gives a bound near 0.95.

These bounds apply only to the sampled scenario and player-policy distribution. They do not establish universal reliability.

### Ablations

| Factor | Baseline | Proposed condition | Primary endpoint |
|---|---|---|---|
| Dialogue ownership | Narrator writes all lines | Character supplies exact accepted line | Voice first-failure turn |
| State ordering | Extract after prose | Declare consequence before prose | Fact and obligation survival |
| History | Summary-only return value | Attributed raw evidence with derived search keys | Retrieval recall and intent survival |
| Progress state | Observation graph alone | Separate obligation and time ledger | Premature closure and stall rate |
| Character mutation | Whole-core replacement | Versioned evidence-linked patch | Core drift and rollback span |
| Anti-stall | No pressure policy | Visible pressure with refusal preserved | Progress without refusal override |
| Recovery | Checkpoint resume only | Rollback, replay, and contamination audit | Recovery success and residual contamination |

Run removal ablations against the complete candidate stack. Pair traces when a fixed player action remains legal in both conditions.

### Sample-size requirements

Use 20-30 paired pilot blocks to estimate baseline survival and discordant-pair rates. Then power confirmatory McNemar tests from those observed rates.

As a conservative independent-arm reference, detecting survival of 0.70 versus 0.90 needs about 62 runs per arm. This assumes 80% power and two-sided \(\alpha=0.05\).

Do not infer an ablation effect from one campaign. Report effect sizes and confidence intervals even when a significance threshold is missed.

The 29-run and 59-run zero-failure rules apply only to the final 300-turn threshold claim. They are not substitutes for powered mechanism comparisons.

### Recovery and error containment

Inject known corruption at fixed checkpoints. Faults should include a false possession, wrong speaker, premature obligation closure, invalid core patch, and wrong diegetic time.

Measure:

- **detection delay:** committed turns from injection to detection;
- **rollback span:** valid turns reverted to restore a sound version;
- **replay success:** fraction of faults yielding a consistent resumed state;
- **residual contamination:** later claims still derived from the injected fault;
- **unaffected-state preservation:** valid facts and relationships retained after recovery;
- **player-visible discontinuity:** blinded rating of the repair's narrative break;
- **recovery cost:** extra calls, tokens, latency, and discarded valid turns.

Containment succeeds only when the bad dependency disappears and unrelated state survives. Reloading a checkpoint without removing the dependency is resume, not recovery.

## Impact on Rhapsode

The evidence supports a narrow authority split for experiments:

- `TurnExecutor` should stage transcript and state patches before one atomic commit.
- Character processes should propose exact speech and actions before narrator composition.
- Memory should retain immutable attributed turns and return them through targeted retrieval.
- A small ledger should own obligations, goals, closure evidence, and due conditions.
- CharacterCore changes should become evidence-linked patches over named versions.
- WorldGraph should remain a provenance-bearing observation index.

This split does not prescribe the story. It limits which uncertain model outputs can become irreversible truth.

## Limitations

The complete task has no positive 300-turn control result. Every architecture ranking therefore remains a hypothesis.

The studies use different models, scenarios, player policies, and failure definitions. Their numerical outcomes cannot be pooled into one universal survival curve.

NCP-Bench stresses fixed movie commitments and prioritizes trajectory progress. Rhapsode permits a more open future, so the benchmark is informative but not equivalent.

The local diagnosis comes from one auto-player campaign. Controlled ablations must separate prompt, model, memory, guide, and orchestration effects.

Citation maturity and repository state can change. The fixed paper versions and commit identifiers preserve this audit's evidence boundary.

## See Also

- [Default Guide 300 long-run episode](../episodes/2026-08-10-long-run-storyline-and-character-collapse.md)
- [[research/motif-collapse-default-guide-300]]
- [[architecture/pragmatic-turn-transaction-refactor]]
- [[architecture/monologue-streams]]
- [[architecture/plot-graph]]
- [[research/narrator-and-mind-weakpoints]]
- [[research/literature-review]]
