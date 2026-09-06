## [2026-09-06] plan | Configurable Kimi narrator

- Narrator scene-writing and inner thoughts can use `kimi-k3` via env (`RHAPSODE_NARRATOR_MODEL`, `RHAPSODE_NARRATOR_API_BASE`, `MOONSHOT_API_KEY`). Default remains DeepSeek. Official K3 request shape: `reasoning_effort` (default `max`), no `thinking` body, no `temperature`. China host `https://api.moonshot.cn/v1` for platform.kimi.com keys.

## [2026-09-06] experiment | Live five-turn player pair 20260906-play

- `narrator_experiment.py --mode none --stamp 20260906-play`. Prompt 48/48. Player played; no scripted Kazuma (`injects=[]`, no `injections.log`). Both MaxTurns 5/5, errors 0, timeouts 0.
- `empty_beats` old=0 new=1. Studies (new) called `query_character_core` 11 times across 6 beat rounds; old 0.
- Fixture: `offline/character_study/checkpoints/narrator-ab-20260906-play/` (`reading.md`, `plain.md`, `manifest.json`). Runs: `experiments/session_pipeline/runs/study-ab-{old,new}-none-20260906-play/`.

## [2026-09-06] experiment | Two-turn pair after how-to no longer names tools

- `narrator_experiment.py --mode none --injects board,vote --stamp 20260906-core2`. Prompt 48/48. Both MaxTurns, two injects, no player LLM. `_core.pyd` rebuilt first (how-to says “Use tools…”, does not name tools).
- Old and new `empty_beats=0`. Studies-side profile still never called `query_character_core` (graph/mind only on the beat). Fixture: `offline/character_study/checkpoints/narrator-ab-20260906-core2/`.

## [2026-09-06] plan | Offload mind calls to a trained model

- Staged plan with gates: Flash-off baseline → HER-RM judge → LoRA on HER-32B with own mind logs + CoSER inner-thought data → RL only if SFT fails → serving cost decision → optional drift detector. Frontier control in every A/B; narrator untouched. Page: [research/mind-model-offload-plan.md](research/mind-model-offload-plan.md).

## [2026-09-06] experiment | Two-turn core-tool A/B

- `narrator_experiment.py --mode none --injects board,vote --stamp 20260906-core`. Prompt 48/48. Both MaxTurns, two injects, no player LLM. `_core.pyd` rebuilt first.
- Old `empty_beats=0`. New turn 1 had speech and no narrator prose (`empty_beats=1`). New-side profile never called `query_character_core` (graph/mind/history only). Fixture: `offline/character_study/checkpoints/narrator-ab-20260906-core/`.

## [2026-09-06] runtime | Narrator query_character_core

- Beat narrator can pull the full Who you are page via `query_character_core`. `query_mind` is perception plus the last three monologue lines. Old A/B arm omits the new tool; new arm enables it. Narrator tool loop default 24.

## [2026-09-06] experiment | Second fair five-turn A/B

- `narrator_experiment.py --mode none --injects all --stamp 20260906-ab2`. Narrator prompt 48/48 words. Both arms MaxTurns, five injects, no player LLM. No tool-loop / 0-char.
- Both sides spoke on all five turns. New-side turns 1 and 4 had speech with no narrator prose (`empty_beats=2` in the new-side report).
- Fixture: `offline/character_study/checkpoints/narrator-ab-20260906-ab2/`.

## [2026-09-06] experiment | Fair five-turn A/B

- `narrator_experiment.py --mode none --injects all --stamp 20260906-ab`. Narrator prompt 48/48 words. Both arms MaxTurns, five injects, no player LLM.
- Old side spoke on all five turns. New side empty on turn 4 (`Are you coming with us?`) — narrator tool loop, 0 chars.
- Fixture: `offline/character_study/checkpoints/narrator-ab-20260906-ab/`.

## [2026-09-06] experiment | Short narrator A/B retries

- `narrator_experiment.py` now defaults to `--mode none` (do not paste studies onto the narrator) and two injects (`board,vote`).
- Try 1 (clean, board+vote): both sides spoke; vote was not empty.
- Try 2 (full paste, board+vote): ~6k-word new-side dump emptied the vote (tool loop, 0 chars). ~500-word old-side paste still spoke.
- Try 3 (clean, board+pocket): both sides answered the pocket. First-night empty pocket was the dump, not the pages in private text.
- Notes: `offline/character_study/checkpoints/short-tries.md`.

## [2026-09-05] experiment | Study A/B in session eval

- Offline `narrator_experiment.py` built temp Konosuba copies from `study/darkness.md` and `study/megumin.md`, then launched both `run.py` arms in parallel (8080/8081).
- Pair `checkpoints/narrator-ab-20260905-235107/`: both `MaxTurns`, five injects, no player LLM. Reading pack + `judgment.md` in that folder.
- Treatment minds added civic weight (Darkness) and clause/formation ownership (Megumin) beyond the compact gags. No novel-history dump. Treatment Darkness t=3 perception lost first person. Vote was empty on both arms; pocket was empty on treatment only.

## [2026-09-05] revise | Megumin Candidate 2, C1 relationships grown in

- Left Candidate 2 standing. Grew Kazuma, Wiz-as-person, Darkness (including Vanir), and Aqua’s denied divinity into C2’s voice.
- Did not rewrite C2 sentences, resolve Yunyun, or overwrite Pass 1 / the two originals.
- File: `offline/character_study/checkpoints/megumin/study-candidates-20260905-224326/candidate-2-revised.md`.

## [2026-09-05] run | Megumin vols 1–4 three-pass

- Megumin used the same three-pass path as Darkness, isolated via `config-megumin.yaml`.
- Pass 1: critic `--until-volume 4` → `offline/character_study/study/megumin.md` (sections 1–11).
- Pass 2: `checkpoints/megumin/reader-baseline-20260905-222637/`.
- Pass 3: `checkpoints/megumin/study-candidates-20260905-224326/` (two unmerged candidates). Darkness study unchanged.

## [2026-09-01] experiment | Identical-input blind readers

- Added an offline baseline that sends the same first-pass Darkness study and neutral prompt to two
  fresh DeepSeek sessions at temperature 0.9; explicit per-session seeds prevent exact-request replay.
- Repaired OpenAI-compatible gateway support after the public DeepSeek endpoint rejected the
  workspace credential.
- The initial unseeded requests returned byte-identical 918-word letters, consistent with cached or
  deterministic request handling. The seeded run produced distinct 974- and 1,149-word letters.
- Despite different wording and secondary emphases, both seeded readers selected the same central
  scenes and interpretation. Sampling added local nuance but did not create independent readings.
- Local artifacts: `offline/character_study/checkpoints/darkness/reader-baseline-20260901-195321/`.

## [2026-08-30] plan | Offline character study in Rhapsode

- Moved the Konosuba extract → critic → cite → refine stack into
  `offline/character_study/`. Segregated from the turn loop and session eval.
- Tracked: `study/`. Not tracked: `books/`, novel text, `sections/`, `checkpoints/`.
- Docs: [architecture/character-study](architecture/character-study.md).

## [2026-08-29] change | Autoplay process isolation

- Spawned eval no longer uses `server/saves` + `server/chroma` + a killed port 8080.
  Each `run.py` spawn gets `<out_dir>/live/{saves,chroma}`, the next free port if
  the preferred one is busy, and aborts if its own uvicorn never becomes healthy.
- Server honors `RHAPSODE_SAVES_DIR` / `RHAPSODE_CHROMA_DIR`. `autoplay.bat` no
  longer kills the listener on the chosen port.
- Port claim is a process-held lock (`runs/.port-locks/`), not a listen-check:
  uvicorn binds only after embedding warmup. Dispatch:
  [architecture/session-eval](architecture/session-eval.md).

## [2026-08-29] change | Narrator author contract

- Replaced the beat narrator instructions: player input is declaration, not outcome; the
  world stays in motion; characters play baseline first; private monologue lines steer
  behavior and are never narrated; silence is a legal take.
- Injects `scene.system_prompt` as a subordinated `Scene style` block. Turn state appends
  each on-stage NPC's latest monologue line as `On their mind` when one exists.
- Speech validation no longer requires a cue whenever cast is present. It still rejects
  Player cues, and a mute cast only when the player names a present NPC.
- Replay script: `server/replay_gold_fishing.py`. Transcript:
  `server/logs/replay_transcript_20260829.txt`. Docs:
  [architecture/scene-loop](architecture/scene-loop.md).
- Replay judgment vs the gold-and-fishing baseline: gold is acted on (Aqua
  hoards, Megumin takes the contract); night return is occupied (Megumin
  awake, Aqua drunk behind a barricade); turn 4 re-walks a homecoming that
  turn 3 already completed; Darkness still fires the kink voice on the
  rationing beat. Player speech leaked as a `Kazuma` cue on turn 0.

## [2026-08-29] research | Player-tools 149-turn autoplay

- Continuation 300-turn run stopped at 149/300 on DeepSeek 402. Tools used on 58.7% of player decides; 9 forks / 7 merges / 1 conclude.
- Collapse mode vs `default-guide-300`: cemetery send-off recycled six times; player leaks board/guide into the action (43/149). Marsh quest concluded off-stage. Player is 0.8% of API wall.
- Analysis: [research/player-tools-149-narrative-analysis-2026-08-29.md](research/player-tools-149-narrative-analysis-2026-08-29.md).
- Reading edition: [research/player-tools-149-story-with-minds.md](research/player-tools-149-story-with-minds.md).

## [2026-08-28] change | Auto-player uses narrator read tools

- `Story.player_situation()` is C++ cast + storyline board. Eval player calls `complete_with_tools` with `NARRATOR_TOOLS` via `dispatch_tool` (flash, thinking off). Deleted the Python situation/list_scenes builder.

## [2026-08-28] research | Clean 20-turn Ashenmoor narrative analysis

- Cleared all saves and Chroma state with `reset.bat`, verified both stores empty, and ran a guide-free 20-turn siege autoplay.
- The run completed 20/20 with zero errors or timeouts; mean ready/idle latency was 62.83/171.86 s.
- Produced a complete reading edition with 36 retained perceptions and 34 monologues, plus an evidence-based analysis of narrative stasis, private-intention loops, graph growth, tool-loop failures, and evaluator blind spots.
- Story: [research/ashenmoor-clean-20turn-story-with-minds.md](research/ashenmoor-clean-20turn-story-with-minds.md).
- Analysis: [research/ashenmoor-clean-20turn-narrative-analysis-2026-08-28.md](research/ashenmoor-clean-20turn-narrative-analysis-2026-08-28.md).

## [2026-08-28] story | Complete Ashenmoor story with minds

- Arranged the complete scene through turn 26 as a five-chapter novel-format reading edition.
- Interleaved all 32 retained perceptions and all 30 retained monologues at their originating turns, and restored the merged Maren fork as an in-sequence interlude.
- Documented archival gaps explicitly; no missing private prose was reconstructed.
- Story: [research/ashenmoor-complete-story-with-minds.md](research/ashenmoor-complete-story-with-minds.md).

## [2026-08-27] change | Deferred graph settlement and batched expiry

- Moved graph extraction out of `execute_turn` into a version-checked settlement consumed by
  `Story::complete_turn`, after player output delivery and the server's `status: ready` boundary.
- Preserved the turn's frozen read-tool snapshot across that boundary; stale or failed settlements
  cannot overwrite current state.
- Batched up to eight disjoint expiry groups and 80 live nodes per Weaver call under a prompt-size
  budget. Priority order, overlap ordering, stop behavior, and group-local validation are preserved.
- Verification: clean native rebuild and all 95 C++ tests; rebuilt Python extension and all 50
  server tests pass.

## [2026-08-27] change | Separate ready and idle evaluator latency

- Session eval records the first `status: ready` as `ready_ms` and terminal `status: idle` as
  `idle_ms`; legacy `t_ms` remains an exact alias for `idle_ms`.
- A fresh four-turn autoplay completed 4/4 with zero errors or timeouts and server exit code 0.
  Mean ready/idle latency was 81.49/165.44 s; post-ready work averaged 83.95 s and represented
  50.7% of total measured turn time.
- The runner's relative profile path initially split five player records from 75 server records;
  both files parsed cleanly and were retained together in the run artifacts.

## [2026-08-26] research | Interactive latency re-audit after mind-ring update

- Re-audited the foreground response path and new four-slot perception/monologue scheduler at `029edba`.
- Combined the current 3-turn fresh and 4-turn continuation profiles: beat 47.8 s, graph 55.7 s, total 103.6 s, so moving graph after response delivery models a 53.8% reduction.
- Background reasoning share fell from 74.4% to 57.6%, but provider admission remains unprioritized and superseded ring futures are not cancelled.
- Verification: current source builds; four Python `PromptJobs` tests pass; a rebuilt C++ suite has 39/91 access-violation or heap-corruption failures requiring separate diagnosis.
- Docs: [research/rhapsode-interactive-latency-reassessment-2026-08-26.md](research/rhapsode-interactive-latency-reassessment-2026-08-26.md).

## [2026-08-26] change | Monologue prompt: continue the thought

- Rewrote `monologue_system_instructions`: the line continues one developing train of thought ("what did this beat change for you"), 1-3 sentences, rough not quotable. Null is for redundancy ("would only restate previous lines"), replacing "most beats you only listen" which primed silence and starved the character's only durable subjective memory. Sources: Open Souls goal-directed `internalMonologue`, Stanford generative-agents reflection-over-reflection, Bicking's repetition/uplift notes.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-25] change | 4-slot mind rings

- Perception and monologue each have a 4-slot ring. `head` is this beat’s slot; `end_mind_turn` increments it even on skip-send and kills a leftover occupant. Harvest applies only the newest ready result. `process_post_turn` harvests and catch-up-sends monologue before weave; idle `poll_minds` harvests and catch-up-sends without incrementing. Prompt jobs carry a generation so a killed HTTP cannot apply onto the new claim.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md), [architecture/python-server.md](architecture/python-server.md).

## [2026-08-25] change | Scene turn_index is last committed

- `scene.turn_index` is the beat that just committed (`-1` if none). Deleted `post_turn_index` and `beat_clock`. Fresh/fork scenes start at `-1`; first beat is 0.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md).

## [2026-08-24] implementation | Idle mind poll

- Play session calls `Story.poll_minds` every 0.25s while idle: harvest finished perception, then claim monologue. Perception claim stays in `process_post_turn`.
- Docs: [architecture/python-server.md](architecture/python-server.md).

## [2026-08-24] change | Perception copy, then monologue

- Deleted the objective journal. Narration stays on the scene; `format_narration_window` slices the last three turns and passes that string into Perception.
- Each character stores one overwritten `perception_` string. Monologue copies it and never reads narration. Claim by scene turn.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md), [architecture/scene-loop.md](architecture/scene-loop.md), [decisions/2026-08-24-linear-monologue.md](decisions/2026-08-24-linear-monologue.md).

## [2026-08-24] change | Linear untagged monologue

- Cut stream roster, fork/merge/conclude ops, and the replaced tail. Private lines sit after name + Who you are as untagged prose interleaved with the objective journal.
- Sidecar is `{"line":"..."}` or `{"line":null}`. Observation still writes `take`/`seen` and facts.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md), [decisions/2026-08-24-linear-monologue.md](decisions/2026-08-24-linear-monologue.md).

## [2026-08-23] implementation | Drain in-flight journals on socket close

- `World::apply_ready_observations` / `apply_ready_monologues` harvest without dispatch. Poll calls them, then claims.
- `Story.apply_ready_journals` then save. Session waits `PromptJobs` and shuts the pool without waiting again.
- Docs: [architecture/python-server.md](architecture/python-server.md).

## [2026-08-23] implementation | Observation/monologue poll from the play session

- Story setters: `set_observation_ready_callback`, `set_observation_submit_callback`, `set_monologue_ready_callback`, `set_monologue_submit_callback`.
- Session wires two `PromptJobs` on one `ThreadPoolExecutor`. Blocking observation/reflection callbacks stay unset so `process_post_turn` polls.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md), [architecture/monologue-streams.md](architecture/monologue-streams.md), [architecture/python-server.md](architecture/python-server.md).

## [2026-08-23] cleanup | Drop perception reroute

- Removed `World::route_perceptions`, `CharacterMemory::route_fact`, and `Node.audience`. Old saves may still hold `type=perception` nodes; monologue expires the live ones. Graph extract stays plot-only.
- Docs: [architecture/plot-graph.md](architecture/plot-graph.md), [architecture/subjective-character-minds.md](architecture/subjective-character-minds.md).

## [2026-08-23] change | Monologue tail is this take plus seen

- `World::update_monologues` builds `What just happened` from this turn's journal: capped `take`, then `seen`. Prefix (name / core / foci / stream journal) is unchanged. No routed perceptions, no full public log.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-23] change | Observation uses flash, thinking on

- `make_observation_callback` is base/`RHAPSODE_MODEL` (flash) with thinking on. Monologue stays narrator/pro with thinking on.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-23] implementation | Per-character objective journal

- World `GRAPH_UPDATE` no longer routes `new_nodes` into minds. Each on-stage NPC journal gets this turn's narrator + speech (`take`), then a no-tools observation call may append `seen`.
- Monologue `What just happened` is this turn's `seen` lines only.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md), [architecture/plot-graph.md](architecture/plot-graph.md), [architecture/scene-loop.md](architecture/scene-loop.md).

## [2026-08-23] fix | Monologue public log is append-only

- `What just happened` is the full player/narrator history (oldest first, per-line cap), not last-4 `format_graph_seed` on a history window. Journal was already append-only; the take is too.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-22] implementation | Monologue prompt: core, foci, append-only journal

- User sheet is name, Who you are, On your mind (`id: focus`), What you've been thinking (global `seq` log, including closed streams), What just happened (replaced take + perceptions).
- Voice, compact beliefs, and last-3 inner beats are out of the monologue prompt. Craft treats a stream as a focus, not only a want.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-22] implementation | execute_turn extract copies and speech helpers

- Graph extract mutates working copies and assigns them only on success; dropped
  `completed_turn` / `base_state_version` / `resulting_state_version`.
- Speech validation helpers folded into `validate_speech_turns`.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md).

## [2026-08-22] implementation | Cut turn-path ceremony

- Local `TurnRollback` in `execute_turn`; deleted re-entry guard, LLM-null and stale-version
  checks, `SpeechCue`, timing averages, `GraphPlanResult::context_blocks`, and paired catch blocks.
- Narrator sanitize-once; one chroma swallow; fork/merge synthesize no longer takes a run guard.
- Python `run_session` still always `complete_turn` after a successful `advance_player`. `view_of`
  remains the Python name for `render_thoughts`.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md),
  [architecture/cpp-data-model.md](architecture/cpp-data-model.md).

## [2026-08-22] fix | Autoplay WS probe racing Chroma

- Eval spawn-wait now `GET /health` instead of opening `/ws`. A probe `/ws` started a full
  Story+Chroma session, then the real connection opened Chroma again and chromadb 1.5.9 crashed
  (`RustBindingsAPI.bindings`).
- Chroma persistent client is opened once during FastAPI lifespan (`warmup_chroma`).
- Docs: [architecture/python-server.md](architecture/python-server.md),
  [architecture/memory-system.md](architecture/memory-system.md).

## [2026-08-22] implementation | Readable C++ (every file)

- Deleted unread C++: legacy turn-shadow types, `resuming` / `resume_history_window`, `LogContext`,
  `pressing_thought` / `charge_state`, `BeatSummary`, `GraphPlanResult::rejections`,
  `World::character_description`, binding-only `Director` / `Weaver(graph)`, and unread eval `pid_`.
- Flattened `execute_turn` (prompt mutations and graph extract sit in the turn script). Moved
  fork/merge synthesize into `story_lifecycle.cpp`. Collapsed `Weaver::weave` / `weave_impl`.
- Python `Story.world()` remains a detached snapshot; `set_resuming` is gone. Docs:
  [architecture/scene-loop.md](architecture/scene-loop.md),
  [architecture/system-overview.md](architecture/system-overview.md),
  [architecture/cpp-data-model.md](architecture/cpp-data-model.md).

## [2026-08-22] implementation | Fork/merge visible in story.txt

- Parent timeline now keeps `[Fork]` / `[Merge]` system notes (`fork from=… to=…`, `merge from=… into=…`).
- Merge archives the retired fork as a `SceneClosure` with `merged_into`, so `story.txt` still has a
  `## Fork — {id} (merged into {parent})` section after the live scene file is deleted.
- Docs: [architecture/cpp-data-model.md](architecture/cpp-data-model.md),
  [architecture/system-overview.md](architecture/system-overview.md).
- Rebuild `_core.pyd` before the next autoplay run or the eval transcript will stay on the old renderer.

## [2026-08-22] experiment | Fork/merge autoplay guide

- Added player brief [`experiments/session_pipeline/guides/fork_merge.md`](../experiments/session_pipeline/guides/fork_merge.md): one Aqua-to-cemetery (or Megumin crater) split, then travel-and-greet until co-presence; no second split.
- `config.toml` default guide now points at it (autoplay.bat still defaults to `guides/default.md` until you pick the new file).
- Older stub `guides/merge_fork_test.md` kept so prior run notes still resolve.

## [2026-08-22] implementation | Narrator prompt unclump

- Rewrote Phase A as stage craft with the speech/cast schema last; user sheet is on-stage+voice,
  off-stage names, readable threads, story so far, then attributed Player/Narrator/Name lines.
- Phase B (`GRAPH_UPDATE`) now sees only this take's prose and speech, not the Phase A sheet.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md).

## [2026-08-22] implementation | Monologue prefix cache (minimum)

- Reordered the on-stage monologue prompt: shared craft+JSON schema, then name/voice/core, then want-roster and inner beats, then this take and raw perceptions.
- Native blob still one `LLMCallback` string; sentinel `<<<RHAPSODE_MONOLOGUE_USER>>>`. Python splits to `system` + `user` with no history store. Voice comes from `Character::build_prompt__dialogue_voice()`.
- Apply path unchanged (`appends` / `ops` / `knows` / `core_revision`). Rebuild `_core.pyd` or Python falls back to a single user blob.
- Docs: [architecture/monologue-streams.md](architecture/monologue-streams.md).

## [2026-08-22] fix | Unlock input during weave/monologue

- `/ws` now sends `status: ready` after streaming `advance_player` outputs, before `complete_turn`.
- The input bar was locked for the whole weave/expiry/monologue tail; `idle` still arrives after
  post-turn so eval keeps waiting for the full turn. A typed next action waits in the socket until
  the loop receives again.
- Docs: [architecture/python-server.md](architecture/python-server.md),
  [architecture/vue-frontend.md](architecture/vue-frontend.md).

## [2026-08-22] implementation | Flattened Story turn dependencies

- Reduced the native ownership graph to `Story` owning `StoryData`, `TurnServices`, and one pending-turn value.
- Replaced parallel player/autonomous entry points with `execute_turn(StoryData&, TurnServices&, TurnInput)` and removed temporary turn wrapper records.
- Removed the core `Director` class in favor of stateless `apply_graph_plan`; `Weaver` now receives its graph per operation instead of retaining a graph reference.
- Separated live observation storage from coded `World` state while preserving the old save and Python World shape through explicit import/snapshot conversion.
- Kept historical `Director(graph)` and `Weaver(graph)` constructors inside binding-only compatibility wrappers.
- Renamed intermediate `story_state_ops` files and the ambiguous aggregate version to `story_data_ops` and `transaction_version`.
- Rewrote the system, data-model, turn-execution, plot-graph, Python-boundary, and implementation-plan documentation against the final code.
- Passed a clean native/binding build, 76 native cases with 442 assertions, 41 Python tests, Ruff, compileall, and targeted wiki lint.

## [2026-08-21] implementation | Data-oriented turn pipeline

- Removed `TurnExecutor` and replaced it with free turn functions over `StoryState`, `TurnRuntime`, and short-lived turn records.
- Kept `Story` as the C++/Python compatibility façade while moving scene lookup, summaries, scheduling, lifecycle application, and turn execution into focused modules.
- Changed foreground execution to commit complete candidate World and SceneData values after a base-version check.
- Moved graph extraction after foreground commit, contained extractor failure, and removed graph-driven character death.
- Preserved Python APIs, old saves, both transcript buffers, and compatibility-only “beat” names.
- Rewrote the system, data-model, turn-execution, and pragmatic-plan pages around the executable design.
- Rewrote the plot-graph page to document its post-commit, non-mechanical authority boundary.
- Kept actor authority and consequence-first rendering disabled pending shadow and sequential gates.
- Passed a clean native and binding build, 76 native cases with 440 assertions, 41 Python tests, and Ruff.

## [2026-08-20] implementation | Turn transaction phases 0–4

- Repaired and froze the native baseline, including MSVC UTF-8 compilation and roster-derived CharacterCore initialization.
- Added one attributed transcript view and exact `query_transcript` dispatch over existing history and dialogue storage; the production model schema remains unchanged.
- Added persisted aggregate state versions, deterministic message references, frozen read-tool snapshots, stale-turn rejection, and rollback coverage.
- Delayed output callbacks until after commit and made callback exceptions explicit post-commit delivery failures.
- Added inert legacy `ActorProposal` and `TurnDecision` records for audit only; legacy graph output remains a separate observation payload and gains no authority.
- Passed a clean native build and all 74 Catch2 cases / 431 assertions, built `_core.pyd`, and passed 28 focused Python binding/lifetime tests.

## [2026-08-20] plan | Pragmatic turn transaction refactor

- Added [Pragmatic turn transaction refactor](architecture/pragmatic-turn-transaction-refactor.md) as the obligation-free implementation plan.
- Kept the current `Story`, `TurnExecutor`, `SceneData`, `World`, transcript buffers, and compatibility path as the migration base.
- Defined phased gates for attributed transcript evidence, one frozen versioned context, staged callbacks, shadow actor proposals, consequence-first rendering, narrow authority transfer, whole-turn recovery, and graph demotion.
- Limited deterministic validation to implemented mechanics and kept proposal quality, canonical-event correctness, retrieval recall, and 100/300-turn survival explicitly unproven.
- Required one reversible compatibility boundary per phase instead of story-specific fixes or an additional LLM validator.

## [2026-08-19] audit | Current executable system baseline

- Rewrote [Current executable system baseline](architecture/system-overview.md) from the production C++, bindings, Python server, Vue client, and evaluation harness.
- Separated the rollback-capable foreground beat from non-transactional post-turn, lifecycle, off-stage, and save behavior.
- Verified that exact NPC dialogue is stored outside normal narrator history; Chroma is not used by narrator retrieval; graph mutation occurs after prose; undo and saves are partial; and no Vulkan subsystem exists.
- Marked obligations, actor proposals, and consequence patches as experiments rather than current architecture.
- Corrected stale knowledge-graph and concept-index entries that still named removed `SceneLoop`, `CharacterAgent`, and prompt-builder components.
- Ran the repository verification chain: production native build passed; native tests failed to compile on a code-page issue; 41 Python tests passed with one failing smoke module excluded; Ruff and all frontend checks passed.

## [2026-08-17] plan | Obligation capability evaluation

- Added an agent-executable benchmark plan at [Obligation capability evaluation](../experiments/obligation_capability/agent-plan.md).
- Separated commitment authoring, registration, lifecycle preservation, and prompt contamination into independently scored experiments.
- Required human gold labels, raw evidence, sequential survival reporting, reproducible run manifests, and explicit authority gates.
- Made no production code or obligation-authority change.

## [2026-08-13] research | Concrete long-horizon turn transaction

- Moved the implementation design out of the evidence review; its maintained successor is [Pragmatic turn transaction refactor](architecture/pragmatic-turn-transaction-refactor.md).
- Restored the agreed `ActorProposal`, `Obligation`, and `ConsequencePatch` contracts verbatim, made the ten-step execution order explicit, and promoted turns 7, 25, 98, 200, and 287 into a pre-generation falsification gate.
- Rewrote the implementation page around verified current code seams, one monotonic compatibility mode, bounded review phases, exact file-level changes, rollback paths, and explicit post-turn limitations.
- Added implementable proposal, obligation, and consequence-patch records to the migration plan.
- Defined authority boundaries, transaction invariants, mutation rules, and failure behavior for each of ten turn stages.
- Added a ten-phase migration that reuses TurnExecutor rollback, SceneMessage metadata, existing histories, TurnWork, and transaction tests.
- Deferred durable receipts, split planner-renderer calls, and authoritative ledgers until additive shadow phases pass explicit exit criteria.
- Kept atomic commit, rollback, replay, and cross-stage composition labeled as unproven hypotheses.

## [2026-08-12] research | Orchestration mechanisms and 300-turn reliability

- Rewrote [Frontier LLM orchestration over 300 interactive turns](research/frontier-llm-long-horizon-orchestration.md) from verified primary papers and fixed repository revisions.
- Separated character chat, autonomous drama, scripted choices, coded simulations, finite prose, and evolving free-form worlds.
- Added a source comparison, repository audit, benchmark-specific survival estimates, mechanism falsifiers, and ranked architecture hypotheses.
- Defined a staged 20/100/300-turn evaluation with ablations, powered comparisons, recovery injections, and containment metrics.
- Found no published demonstration of reliable 300-turn operation for Rhapsode's complete task.

## [2026-08-12] research | Repeated bounded-context reconstruction

- Extended [Frontier LLM orchestration over 300 interactive turns](research/frontier-llm-long-horizon-orchestration.md) with a primary-source audit of automatic scene, event, and session handoffs.
- Separated the demonstrated mechanism -- assembling a bounded prompt from persistent records -- from the unproven claim that repeated reconstruction preserves the correct narrative working set.
- Audited LoCoMo, StoryBox, RecurrentGPT, Re3, DOME, MemGPT, LongMemEval, Generative Agents, BOOK WORLD, OPEN-THEATRE, EvoSpark, and NARRA-Gym. Released code paths were checked where available.
- Snowballed backward and forward citations from NCP-Bench, StoryWriter, LoCoMo, and LongMemEval; added BOOKMARKS, ConWriter, Lost in Stories, RealTalk, Conversation Chronicles, and StoryBench where they sharpen the Rhapsode boundary.
- Identified the central evidence split: positive compression systems know the next event or restore canonical history, while Rhapsode must select evidence for an unknown future and then live with its own errors. Corrected the LoCoMo version statistics and NCP-Bench venue status against primary records.
- Rejected automatic chapter resets as an established reliability mechanism. Recommended a low-cost reconstruction-fidelity gate on the existing 300-turn artifact before another long generation run.

## [2026-08-11] research | Frontier LLM orchestration over 300 turns

- Created [Frontier LLM orchestration over 300 interactive turns](research/frontier-llm-long-horizon-orchestration.md).
- Reviewed eight close primary systems across free-form dialogue, scripted drama, coded simulation, long-form prose, and sequential narrative stress testing.
- Verified evaluation horizons, affiliations, venue status, citation maturity, and released repositories against specific paper and code revisions.
- Found no credible estimate of 300-turn survival; NCP-Bench supplies bounded evidence at turns 20 and 100, with material baseline limitations.
- Derived a Rhapsode design and evaluation program around character-owned dialogue, pre-prose consequences, separate obligations, verbatim retrieval, and reversible state.

## [2026-08-10] research | Long-run storyline and character failure analysis

- Created [Default Guide 300: coherent stagnation and character convergence](episodes/2026-08-10-long-run-storyline-and-character-collapse.md).
- Traced the dawn-quest failure through likely same-turn graph resolution, an empty scene intention, short-context feedback, and the unchecked invented fight at turn 98; separated direct evidence from the missing-response inference.
- Re-audited the code paths for shared NPC authorship, post-turn monologues, mutable cores, dialogue persistence, graph retrieval, and narrator context limits.
- Framed the 1,500-byte summary cap as a many-to-one information bottleneck, not proof of a low-dimensional manifold.
- Kept the observation graph observational; separated proposed goal tracking, pre-narration character action, attributed-dialogue recall, and core-mutation policy.

## [2026-08-05] research | Motif-collapse post-mortem of default-guide-300

- Analyzed the 300-turn autoplay run: plot vocabulary → 0 by block 6 while "candle" saturates;
  final mip summary covers only history messages 507–596 (turns 1–506 evicted); graph grew
  18 → 1,007 nodes with expiry firing 5 times all run.
- Framed as reinforced stochastic process (Pólya-urn lock-in in the summary bottleneck), not a
  Banach fixed point; discriminating experiment = seed-variance rerun.
- Wrote [motif-collapse-default-guide-300.md](research/motif-collapse-default-guide-300.md):
  loop structure, forensics, formalization, predictions, ranked fixes (importance-based eviction,
  pinned plot state, expiry repair, diegetic clock, dialogue feedback, collapse-sensitive metrics).
- Incidental: working-tree `server/rhapsode/scheduler.py` / `config.py` are 0 bytes (truncation?).

## [2026-08-04] implementation | Early-deliver narrator (advance_player / complete_turn)

- Split Story turn: `advance_player` returns player-beat outputs; `complete_turn` runs post-turn
  (weave/expiry/reflection/downsample), lifecycle, off-stage, save.
- FastAPI streams beat outputs before `complete_turn`; status stays `processing` until finish.
- Docs: [architecture/scene-loop.md](architecture/scene-loop.md),
  [architecture/python-server.md](architecture/python-server.md),
  [architecture/system-overview.md](architecture/system-overview.md).

## [2026-08-04] research | Role-play PDF archive + comprehensive report

- Downloaded 39 PDFs to `raw/papers/llm-roleplay/` (wiki survey + scaffolding finds); extracted text + digests.
- Wrote [llm-roleplay-comprehensive-report.md](research/llm-roleplay-comprehensive-report.md): taxonomy, support/contradiction for frontier-API scaffolding, closest cousins (IBSEN/BookWorld/TTM/…), ranked steal list, Konosuba-style gaps.

## [2026-08-03] content | Konosuba cores from local novels

- Rewrote Aqua / Megumin / Darkness / Luna / Wiz `core` sheets using Yen Press Vol.1 text under `D:/cursor-workspace/Konosuba` (plus corpus scenes for Wiz’s cemetery introduction), not wiki paraphrase alone.
- Cores stay third-person continuity analysis; Cast `description` remains short.


# Rhapsode wiki — log

Append-only timeline of wiki and project evolution. Newest entries at the **top**.

## [2026-08-02] content | Deep CharacterCore for Konosuba

- Scenario characters may author a deep `core` sheet separate from short Cast `description`.
- Konosuba NPCs (Aqua, Megumin, Darkness, Luna, Wiz) now have soul-level core analyses; bootstrap prefers `core` over `description`.

## [2026-08-02] implementation | Minds debug views for monologue streams

- `/minds`, `/characters`, and new `/character/{name}/mind` show CharacterCore + active monologue streams + factual beliefs; belief-graph SVG unchanged.
- `render_mind_query` pybind now returns a JSON string (was unconvertible `nlohmann::json`).

## [2026-08-02] implementation | Monologue streams + character core

- Replaced forced `reflect_perceptions` (Thought-per-subject) with on-stage **monologue updater**: actor-framed streams (subtext), optional `knows[]` into the belief graph (factual LTM), rare core revision.
- CharacterCore = continuity sheet (not a thought stream); bootstrap stream required (may be empty); fork/merge/conclude with min 1 / max 5 active.
- Perceptions are prompt stimulus only (not auto-promoted). `query_mind` returns core + streams + compact beliefs.
- Design page: [architecture/monologue-streams.md](architecture/monologue-streams.md). Older character-system / memory-system reflection claims are stale relative to this.
- Verified: Catch2 `[character_memory]` / post-turn tests green; autoplay `runs/monologue-streams-1` 2 turns, MaxTurns, 0 errors; stage=`monologue` on DeepSeek pro; saves show cores, forked streams with lines, live perceptions consumed, sparse `knows` beliefs.

## [2026-08-02] research | DeepSeek V4 latency / streaming / cache

- Captured exploration notes: streaming mechanics and Rhapsode sketch; why `max_tokens` is a hard stop not a “hurry” signal; Context Caching on Disk as prefix/KV-related reuse and why dates must leave the prompt front.
- New page: [research/deepseek-v4-latency-streaming-cache.md](research/deepseek-v4-latency-streaming-cache.md).
- Added `RHAPSODE_LLM_PROFILE` JSONL profiler (`server/rhapsode/llm_profile.py`) for per-call wall_ms + usage/cache/reasoning + beat/graph phase.

## [2026-08-02] implementation | Board-scoped lifecycle check

- After every scene step, lifecycle sees the whole board and may emit merge/conclude/fork/exit ops
  for any storyline (fork still restricted to the scene that just stepped).
- Removed the per-scene `merge_into` verdict that required co-presence only in the fork's own prose —
  merges can now land on the same turn as a main-scene reunion.
- Unified player and off-stage advance into `Story::step_scene`; scheduler picks up to two off-stage
  scenes with a staleness starvation guard.
- Narrator turn state includes a live-storylines board section; autonomous cues no longer ask the
  fork to re-author Player arrival for merge.

## [2026-07-22] implementation | Storyline lifecycle continuity

- Fork now validates canonical non-Player cast and intention, asks the narrator for one
  `fork_story_so_far`, and commits the child, owned intention, and membership transfer only after a
  valid response.
- Conclusion now rejects Player/final scenes, persists a compact closure, expires the exact
  fork-owned subjective intention, retires membership, and removes stale scene files on save.
- Lifecycle judgment now receives bounded history/graph/mind tools plus completed dialogue;
  contradictory verdicts and invalid or overlapping cast operations are no-ops.
- Python scene reads are detached values, scenario NPCs always have subjective memories, and the
  network-free verifier covers fork, conclusion, and merge synthesis.

## [2026-07-22] plan | Storyline lifecycle continuity

- Defined one lifecycle transaction rule: validate, synthesize only when needed, then commit once.
- Planned narrator-synthesized fork context, exact ownership/expiry of fork intentions, and compact
  persisted conclusion records without an extra conclusion LLM call.
- Moved Player/cast/final-scene checks into Story, added lifecycle read tools and verdict validation,
  and separated whole-Story undo into a later focused change.
- Included detached Python scene values, stale scene-save cleanup, and lifecycle verifier coverage.

## [2026-07-21] plan | Narrator-synthesized scene merge

- Replaced the cast-transfer-only merge with a dedicated narrator call over both scenes' rendered
  summaries, recent narration/dialogue, cast, and intentions.
- Added optional scene selection to `query_history` so merge synthesis can inspect either stream
  while retaining bounded, expiring read-tool borrows.
- The narrator returns one `merged_story_so_far`; only a valid response replaces the destination's
  downsampled context and permits membership transfer/source retirement. Failure is a no-op.
- Kept destination history, dialogue, turn index, and scheduling fields unchanged; save/load
  persists the synthesized context through the existing downsampling schema.

## [2026-07-21] implementation | Runtime class-internal cleanup

- Added focused characterization for WorldGraph edge/components, Director output, successful
  character reflection, MemorySystem payloads, and Annotator precedence.
- Added an RAII TurnExecutor execution guard and replaced vague internal names and dead parameters.
- Decomposed Story summarization, lifecycle context, memory synchronization, and off-stage
  advancement without changing orchestration order.
- Deduplicated WorldGraph edge/component mechanics and clarified World, Weaver, CharacterMemory,
  MemorySystem, and Annotator helpers without adding manager classes.
- Recorded behavior-sensitive findings separately rather than changing game semantics in a
  readability refactor.
- Verified 52 native tests in Release and Debug, 36 Python tests, 7 frontend tests, frontend
  lint/build, and the production native/Python build. Nothing was pushed.

## [2026-07-21] implementation | Runtime mutation boundary cleanup

- Replaced narrator/scheduler read callbacks that re-entered Story with C++ `ReadToolContext`
  dispatch over bounded const World/history borrows.
- Changed Python access to Story-owned graph, roster, character, and character-memory data to
  detached snapshots; invariant-affecting mutation remains behind explicit Story commands.
- Preserved the manual `/weave` diagnostic by routing it through the Story-owned Weaver rather than
  mutating a Python graph alias.
- Added ordered Story move assignment so destination borrowers are destroyed before their old
  World, with native coverage of the transferred runtime and read-tool configuration.
- Verified 46 native tests in Release and Debug, 36 Python tests, 7 frontend tests, frontend
  lint/build, and changed-file Ruff checks; work remains local and nothing was pushed.

## [2026-07-21] implementation | Runtime architectural decoupling complete

- Story now owns stable Director and Weaver services and injects them into a synchronous
  TurnExecutor; TurnResult exposes only generic node effects.
- Removed World filesystem, external-memory, stored-callback, query, death-scan, and scenario
  parsing responsibilities while retaining invariant-preserving domain operations.
- Extracted plain scenario-bootstrap, World-analysis, storyline-policy, scene-history, and
  text-downsampling functions without adding manager or interface hierarchies.
- Replaced persistent Python Story captures with expiring call-scoped read tools.
- Made SceneData a behavior-free aggregate, removed History/TextDownsampler classes, sealed Python
  mutation, and validated active-scene changes.
- Preserved the save schema, prompt hash, transaction order, rollback, retry, lifecycle, query, and
  post-turn failure behavior under native, Python, and integration tests.
- Work was committed locally only; nothing was pushed to a remote.

## [2026-07-21] plan | Runtime architectural decoupling

- Audited the implemented ownership refactor and recorded the remaining responsibility,
  infrastructure, result-type, temporal, lifetime, callback, and mutation coupling.
- Proposed Story-owned Director and Weaver objects injected once into a synchronous TurnExecutor,
  sealing its result behind generic effects, and moving pure narrator algorithms out without
  adding manager classes.
- Proposed reducing World to domain state and invariant operations, moving MemorySystem lifetime,
  filesystem I/O, queries, death scanning, and scenario parsing to explicit boundaries.
- Proposed reducing Story to aggregate coordination, extracting plain storyline-policy functions,
  and replacing persistent Python-to-Story callback captures with call-scoped read functions.
- Proposed behavior-free SceneData value state and read-only Python invariant boundaries.
- Defined phased preservation gates, stop conditions, commit boundaries, and the no-push rule.

## [2026-07-21] implementation | Runtime coupling reduction complete

- Replaced Scene with a World-free SceneData aggregate; Story now exclusively owns World,
  SceneData records, and SceneLoop.
- Made SceneLoop own Director and optional Weaver, removed retained-scene and submit/drain result
  APIs, and made every background borrow call-scoped and synchronously joined.
- Privatized World containers, moved roster/membership/memory mutation behind World operations,
  and removed the World lifecycle queue; Story now applies accepted lifecycle verdicts directly.
- Consolidated scenario loading, complete persistence, and runtime configuration through Story.
  Production Python now owns Story, MemorySystem, and Annotator only.
- Removed Scene and SceneLoop from the production Python surface, blocked direct Python membership
  and death mutation, and migrated maintained diagnostics.
- Verified 38 C++ tests, 23 Python tests, 7 frontend tests, production builds, both linters, 10
  focused Debug tests, and the offline lifecycle diagnostic. Nothing was pushed.

## [2026-07-20] plan | Runtime coupling reduction

- Audited the complete runtime graph after the dependency-safety refactor and separated ownership,
  mutation, callback, async, and compatibility edges.
- Proposed reducing production session ownership to Story, MemorySystem, and Annotator; Story owns
  SceneLoop, while SceneLoop owns Director and optional Weaver.
- Planned removal of the retained-Scene submit/drain compatibility path, temporal result caches,
  standalone loop persistence, and Python-composed runtime wiring.
- Planned private World containers, narrow roster/membership/memory operations, and explicit graph
  access for Director and Weaver without introducing managers, interfaces, or dependency-injection
  infrastructure.
- Revised the target to replace Scene with a World-free SceneData aggregate owned exclusively by
  Story; lifecycle coordination, scenario loading, undo, and complete persistence move to Story.
- Removed the planned Scene-to-World edge and the unnecessary World lifecycle queue from the target
  graph; SceneLoop borrows World by lifetime and SceneData only within a synchronous turn.
- Recorded the first plan's missing acceptance and verification gates as Phase 0 requirements.

## [2026-07-20] implementation | Runtime dependency refactor complete

- Executed the audited Story/SceneLoop dependency plan as local, verified commits; nothing was
  pushed.
- Added explicit scene-turn results, self-contained background results, World/Scene mutation
  boundaries, graph-affinity checks, binding lifetime policies, and World-owned reflection setup.
- Made Story the sole Story-backed persistence, load, and undo boundary without rebuilding
  SceneLoop.
- Split SceneLoop into foreground, narrator, and background implementations; removed
  `scene_loop_support`; split Story into ownership, advance, and serialization implementations.
- Made Python World/Scene graph, roster, and memory-map properties read-only against whole-container
  replacement while preserving their read API.
- Rewrote the SceneLoop and C++ runtime data-model architecture pages. Final verification: 52 C++
  tests, 23 Python tests, 7 frontend tests, both linters, and all production builds pass.
- The two rewritten pages have no page-specific wiki-lint findings; the existing repository-wide
  baseline remains 50 errors and 129 warnings elsewhere.

## [2026-07-20] plan | Runtime dependency plan consistency review

- Reviewed the runtime dependency plan against current C++ ownership, async behavior, compatibility
  APIs, binding lifetimes, and the broader Phase 2 roadmap.
- Restricted Phase 0 to tests that characterize current behavior and moved future acceptance tests
  to the phases that make them pass.
- Defined scoped Scene detachment and single-consumer turn-result ownership, required reflection
  callback reapplication after load, narrowed the narrator implementation boundary, and made binding
  mutation hardening a completion requirement.
- Standardized terminology: SceneLoop executes scene turns; Story coordinates a Story advance that
  may contain a player scene turn and one off-stage scene turn. The planned file is
  `story_advance.cpp`; Story save/load implementation will live in `story_serialization.cpp`.
- Marked the older roadmap's immediate Story/World/Scene split order as superseded by the audited
  dependency migration.

## [2026-07-20] plan | Runtime dependency and ownership refactor

- Audited Story, SceneLoop, Scene, World, Director, Weaver, Annotator, pybind11, and server session
  wiring as one runtime dependency graph.
- Recorded a behavior-preserving, test-gated migration in
  `episodes/2026-07-20-runtime-dependency-refactor-plan.md`.
- The plan introduces explicit per-scene turn results, removes background dependence on mutable loop
  configuration, restores World/Scene mutation boundaries, validates graph/lifetime contracts, and
  establishes Story as the Story-backed persistence and undo boundary before implementation files
  are split.
- Known behavior questions remain separate, and the repository must never be pushed.

## [2026-07-20] implementation | Phase 2 Weaver work queue split

- Characterized priority ordering, supersession timing, missing-callback behavior, and cooperative
  stop behavior before moving code.
- Extracted the existing 147-line queue block text-identically into `weaver_work_queue.cpp`, without
  renaming the expiry API or changing `weaver.h`.
- Verified 43 C++ tests, 12 Python tests, 7 frontend tests, both linters, and the production build.

## [2026-07-20] implementation | Phase 2 CharacterMemory reflection split

- Added characterization coverage for successful, missing, throwing, and malformed reflection
  callbacks before moving implementation.
- Extracted only the existing reflection pipeline into `character_memory_reflection.cpp`; the moved
  279-line pipeline and 19-line label helper were text-identical to the original implementation.
- Kept queries/rendering in `character_memory.cpp`, preserved the public header and serialized format,
  and verified 41 C++ tests, 12 Python tests, 7 frontend tests, both linters, and the production build.

## [2026-07-20] implementation | Phase 2 WorldGraph split

- Extracted DOT rendering and JSON/legacy serialization into separate implementation files and
  separate commits, without changing the `WorldGraph` header or serialized schema.
- Added characterization tests for DOT escaping/state styling and serialized edge/legacy metadata.
- Verified 40 C++ tests, 12 Python tests, 7 frontend tests, both linters, and the production build.

## [2026-07-20] implementation | Phase 2 pybind11 split

- Replaced the monolithic binding translation unit with a single module entry point and story,
  graph, runtime, and memory registration units.
- Preserved registration order and added an exact public-module-symbol characterization test.
- Verified 38 C++ tests, 12 Python tests, 7 frontend tests, both linters, and the production build.

## [2026-07-20] implementation | Code quality phases 0 and 1

- Audited the synchronized `origin/main` tree across C++, pybind11, Python, Vue, build scripts, tests,
  and wiki health.
- Recorded the verified baseline and phased cleanup plan in
  `episodes/2026-07-20-code-structure-quality-plan.md`.
- Prioritized background/persistence ordering, failed-turn transaction semantics, and rendered-model
  text safety before mechanical file splitting.
- Defined responsibility-based extraction seams for Story, CharacterMemory, Weaver, WorldGraph,
  World, Scene, and the pybind11 module.
- Completed the verification foundation: repaired Windows scripts, CMake/CTest presets, explicit
  Python discovery, scoped compiler warnings, Windows CI, and one `verify.bat` entry point.
- Made SceneLoop submissions transactional and background mutation join before scene switching,
  lifecycle/memory sync, shutdown, and persistence; fixed Weaver's dangling sampled-node pointers.
- Added collected Python tests and Ruff, Vitest/ESLint frontend coverage, escaped model-supplied
  inline markup, sanitized annotation categories, and hardened WebSocket connection/message handling.
- Verified 38 C++ tests, 11 Python tests, 7 frontend tests, both linters, the production build, and a
  zero-vulnerability production npm audit.

## [2026-06-24] plan | Parallel scenes — practical design (rewrite of the 06-20 episode)

- Rewrote `wiki/episodes/2026-06-20-parallel-scenes-lifecycle-as-authored-judgment.md` after reading the
  actual substrate (`scene.h`, `scene_loop.cpp`, `world_graph.h`, `node.h`, `character_memory.h`,
  `director.cpp`). The rewrite **closes the three breaks** the prior draft left open after its *War and
  Peace* test, by removing machinery rather than adding it.
- **Key correction from source:** cross-mind propagation is already gated by `audience` /
  `route_perception` (`scene_loop.cpp:242-266`) and the narrator's graph context is entity-scoped
  (`director.cpp:145-210`) — so the "instant shared-state" break is mostly already prevented.
- **Resolved design:** binary LOD (player scene fine / off-scene coarse, drops the importance metric);
  a **mechanical scheduler** = cadence + budget + staleness round-robin, no LLM call (kills charge
  saturation / "second axis"); sequential ticking keeps the existing whole-graph rollback valid (drops
  the per-plan transactional gate); cross-scene latency modeled as a **carrier scene** + authored
  reconciliation at convergence (drops the global story-clock / transmission-as-thread).
- Re-ran the soirée / wolf hunt / Borodino chapters to verify all three breaks dissolve.

## [2026-06-11] research | Subplot lifecycle — craft research (WHEN and HOW to start/end threads)

- Created and expanded `wiki/research/subplot-lifecycle-craft-research.md` — comprehensive research
  into the dramaturgical theory governing subplot (thread) lifecycle decisions: when/how to start,
  advance, and end off-stage storylines in Rhapsode.
- **Human writing craft** (Part 1—): McKee (four subplot functions), Truby (scene weave), Snyder
  (Save the Cat beat sheet — B Story at 22%, payload at 75%), Bell (two doorways + mirror moment),
  Weiland (Lie/Ghost thematic mirrors), Kress (subplot as secondary war front), Sanderson
  (Promise/Progress/Payoff), Film Crit Hulk (independent beat structures per thread). Parker &
  Stone's "but/therefore" rule as a mechanical test for beat readiness. Laws' nine beat types and
  emotional oscillation as the pacing signal. Rule of Threes (setup-eminder-ayoff) for timing.
  Story Spine as minimal generative template for subplot chains.
- **RPG design** (Part 3): Apocalypse World fronts and countdown clocks (the purest model for
  ticking-clock threads — descriptive/prescriptive advancement, hidden from player, past-threshold
  irreversibility). DramaSystem petitioner/granter token economy (structural model for what a
  "dramatic beat" actually is). Sly Flourish's ten secrets (facts fluid until revealed, abstracted
  from delivery). Alexandrian's Three Clue Rule + proactive nodes (three discovery paths per
  subplot, proactive push as safety net). TV writers' room A/B/C story structure.
- **Interactive narrative systems** (Part 4): Emily Short's QBN/salience taxonomy (Rhapsode is
  inherently salience-based — system selects, not player). Kreminski's storylet design space
  taxonomy (four dimensions). Felt/Starfreighter story sifting. Valve's fuzzy pattern matching
  (most-specific-match-wins over a fact database — shipped in L4D). RimWorld storyteller pacing
  (Cassandra's on/off cycles, Randy's 13-day backstop, wealth-based scaling). Façade beat
  sequencing (precondition + tension-arc fit).
- **Convergent principles** (Part 5): subplot = causal chain of 3— beats; start triggers (main
  plot needs contrast, emotional oscillation, thematic mirror, maximum interval); end signals
  (question answered, convergence with main plot, premise superseded, dropped). The "pre-plan the
  chain, improvise the delivery" principle as the consensus resolution.
- **Implications for Rhapsode** (Part 6): the 06-10 episode's autonomous narrator generates beats
  with no pre-planned chain (violates plant/payoff); importance (graph centrality) misses dramatic
  need; what the research supports building: authored subplot chains, salience+oscillation beat
  selection, countdown clocks for ticking threats, three discovery paths, plant/reminder/payoff
  tracking, min/max intervals, convergence as the preferred ending.
- No code changed. Page registered in `index.md`.

## [2026-06-10] design | Parallel scenes — shared pool, LOD by character importance, scene-thread lifecycle

- Turned 06-09's direction (1) into an implementable, phased build plan in
  `wiki/episodes/2026-06-10-parallel-scenes-shared-pool-lod-lifecycle.md`. Model: one shared pool
  (graph + minds) hoisted out of `Scene`; scenes are storyline loops/lenses over it, one thread each;
  per-scene **LOD = player-anchored graph-centrality of the present cast**; an autonomous scene is a
  narrator call with no player input emitting the same JSON plan; the Validator becomes a per-commit
  transactional gate (the serialized writer); the Weaver stitches loci on convergence. Spawn/destroy is
  top-K importance admission + Dormant state + hysteresis, decided by the
  `scan_death_candidates`/`confirm_deaths` propose-then-confirm idiom.
- Corrected three wrong claims inherited from an earlier search-agent map, by reading source: the
  `Validator` **is** wired and live (`app.py:480-484`, `director.cpp:362`); the narrator is the **sole**
  world-author and the Director only *applies* its plan (`tick()` is unused live); the Weaver computes
  **no** importance today (edges + supersession only). Also surfaced the real concurrency blocker: the
  gate's whole-graph `to_json`/`from_json` rollback (`scene_loop.cpp:574,579`) must become per-plan.
- No code changed. Phases 0— tracked as tasks; survey grounding (Generative Agents, Dwarf Fortress
  verified; L4D/RimWorld from prior knowledge) recorded in the episode.

## [2026-06-08] design + bugfix | Two-axes model; subjective self_state; reflection echo

- Design episode from an 18-turn `siege` analysis: the engine is a *consistency* engine with no
  *tension* layer. Captured the model — coherence vs tension axes, freedom as the complement of the
  live constraint set (`valid_until` as the throttle), intent-vs-outcome, validator `supersede`
  verdict, and the Director-as-tension-layer — in `wiki/episodes/2026-06-08-freedom-tension-and-the-two-axes.md`
  and the findings in `wiki/research/narrator-and-mind-weakpoints.md`.
- Bug: `self_state` was folded from `build_scene_context()` (the shared omniscient narration), so
  off-stage minds absorbed events they never witnessed (Voss "tasting" the tent-cure potion).
  `update_self_state(turn)` now folds only the character's *own* perception nodes from `beliefs_`;
  a character who perceived nothing keeps its prior state. (`character_memory.{h,cpp}`,
  `scene_loop.cpp` `build_inner_states`, bindings.)
- Bug: reflected beliefs stored the prompt's echoed label (`"My belief: — `) and ran to purple prose.
  `reflect_perceptions` now strips the echoed label (`strip_echoed_label`), asks for one terse
  first-person sentence, and caps output at 240 chars. (`core/src/character_memory.cpp`.)
- Verified headless against the built `_core.pyd`: stored belief drops the label; off-stage mind's
  self_state stays put while a perceiving mind updates from its own perceptions.

## [2026-06-06] architecture | Validator: one live-state predicate, contradiction-only

- Fixed the continuity `Validator` rejecting foreshadowed payoffs (e.g. Maren's worsening wound) as
  contradictions. Cause was duplicate logic: `gather_context` filtered context to `state == Active`
  on top of the graph's own `valid_until`/`dominated()` notion, hiding Foreshadowed nodes from the
  gate that judges their own activation.
- `core/src/validator.cpp` only. Collapsed the filter to the system's existing live definition
  (Active or Foreshadowed, not superseded) — matching the Director (`director.cpp:191`). Annotated
  chain lines with the real `to_string(state)` instead of guessing `[active]`/`[ended]`.
- Narrowed the prompt to contradiction-only: dropped the "every world change must appear in the
  chain" doctrine and the causal-progression framing that taught novelty-as-contradiction; collapsed
  the two footers into one; re-aimed examples (dead acts, kill-living, reuse-destroyed, foreshadowed
  payoff). Net deletion of code.
- Verified headless against Qwen3-8B: wound-worsening node accepted (foreshadowed wound now in
  context), dead-character-acts still rejected.

## [2026-06-06] architecture | New page: character-system

- Added `wiki/architecture/character-system.md` — end-to-end documentation of the character system:
  the static `Character` persona, the per-character `CharacterMemory` mind (graph, intake, three-signal
  retrieval, reflection, persistent first-person self-state, persona grounding), the stage lifecycle,
  the per-turn pipeline, and the two prompts (decision + actor) that render dialogue.
- Includes ASCII diagrams (two-layer split, turn pipeline, reflection pipeline, self-state fold) and
  critical code excerpts for each subsystem.
- Registered in `index.md` and `graph.yaml` (`CharacterMemory` entity + Scene/SceneLoop edges).
  Health score 5.0, lint clean.

## [2026-06-06] architecture | Persistent first-person self-state for characters

- Addresses §1 (character is a render target, not an agent) and §5 (POV corruption) from
  `wiki/research/character-agent-maren-analysis.md`. Plan: `abstract-scribbling-rain.md`.
- **`CharacterMemory` now carries a persistent first-person `self_state_`** — a running "who I am
  right now" folded forward each turn via `update_self_state(recent_events, turn)`
  (`new_state = LLM(previous_state + what just happened)`), rather than re-derived from a per-turn
  memory query. This keeps an emotional thread (e.g. Maren's hidden wound) alive across topic shifts.
  Serialized in `to_json`/`from_json` (survives save/load).
- **Seeded from authored interiority:** `Scene::load_json` now sets `self_state_` from
  `initial_memory.context` (already first-person), instead of only burying it in the memory stream.
- **§5 POV:** `briefing()`, `reflect()` (focal + insight), `distill()`, `score_importance()` prompt
  strings rewritten from third-person ("what X knows and feels") to first person ("I — ).
- **Persona grounding:** `CharacterMemory` carries a `persona_` (the `Character.description`), injected
  as a "Who I am: —  preamble into the first-person prompts so the local model keeps the right
  identity/pronouns (fixes a misgendering bug — Maren was rendered as "a man"). Not serialized;
  re-attached from `Character.description` on every load (single source of truth).
- Also fixed a latent `briefing()` bug: output was capped at `raw.size()`, discarding any summary
  longer than the bare memory list; now bounded at a fixed 1200 chars.
- **Decision prompt now reads interiority:** the merged narrator/director prompt is the real
  decision-maker (no separate Director LLM). `scene_loop.cpp::build_inner_states()` advances + emits
  an `### Inner states` block (per on-stage NPC) threaded through the `PromptCallback` (new
  `inner_states` arg). The actor prompt also gains an `Inner state` section. Python (`app.py`,
  `prompt.py`) is pure plumbing.
- Deferred: §2 (`source_fact` edges), §3 (unify wound representations), §4 (retrieval ratchet),
  §6 (reconcile actor-prompt signals).

## [2026-05-23] wiki | Align cpp-data-model and memory-system with current code

- **`cpp-data-model.md` rewritten** to match the actual codebase as of 2026-05-23:
  - `Character` struct: expanded from 3 fields to full 9 fields (`dialogue_instructions`, `example_dialogue`, `role`, `on_stage`, `dead`, `created_at`).
  - `Node` struct: removed `known_by` (dropped 2026-05-20), added `trigger` and `arc_position` fields.
  - `WorldGraph`: removed `RelationKind` enum. `EdgeData` now `{weight, created_at, active}`. Updated `add_relation()` signature, added `set_edge_active()`, `set_edge_weight()`, `all_edges()`, `thread_containing()`, `all_threads()`, `revert_to_turn()`, `to_dot()`.
  - `Director`: added `set_validator()`, `focus_payload_text()`, `Rejection` struct, `rejections` in `DirectorOutput`.
  - `MemorySystem`: removed `LemmatizeCallback`, `store_fact()`, `retrieve()`, `retrieve_for_injection()`. New API: `store_node()`, `search_nodes()`, `delete_nodes()`. Single collection `{scene_id}_nodes`.
  - `SceneLoop`: added `Weaving` state, `set_narrator_llm_callback()` (system+user pair), `set_actor_llm_callback()`, `set_weaver()`, `last_weave_result()`. `PromptCallback` now returns `pair<string,string>`.
  - `Scene`: added `character_memories`, `downsampler`, `enter_character()`, `find_on_stage()`, `exit_character()`, `exit_stale_characters()`, `revert_turns()`.
  - **New types documented**: `CharacterMemory`+`MemoryNode`, `Weaver`+`WeaveOp`+`WeaveResult`+`GraphAnalysis`, `Validator`+`Verdict`, `Annotator`+`EntitySpan`, `TextDownsampler`+`Snippet`+`MipLevel`.
  - pybind11 table fully rewritten — 24 bound types (was 16), all methods listed.
- **`memory-system.md` rewritten** from scratch:
  - Removed all references to BM25, entity boosting, lemmatization, spaCy, MD5 hashing, dual collections, distill/quality/conflict pipeline — none of this exists in the current code.
  - Part 1: MemorySystem documented as simplified node index (store/search/delete + ChromaDB metadata sync).
  - Part 2: CharacterMemory documented with full Generative Agents cycle — belief graph, observation intake, importance scoring, three-signal retrieval (formula + process), reflection, meta-reflection.
  - Updated callback signatures table, configuration table, storage layout.

## [2026-05-22] research | Summaryception + Generative Agents code analysis

- Created `wiki/research/summaryception-analysis.md` — full architecture report of Summaryception v5.5.2 (SillyTavern recursive summarization extension). Documents: layered memory model, 4-backend connection routing, prompt isolation, ghosting system, branch repair, retry logic. Includes theoretical framing as recursive IIR filter.
- Created `wiki/research/generative-agents-code-analysis.md` — full architecture report of Stanford's Generative Agents implementation. Documents: memory stream (AssociativeMemory), ConceptNode structure, three-factor retrieval scoring, importance-triggered reflection, hierarchical planning, social reactions, simulation loop.
- Comparative analysis: Summaryception = recursive filter (bounded, lossy); Generative Agents = append-only log + triggered compression (unbounded, implicit forgetting).
- Renamed wiki section to "External Extensions & Reference Implementations".

## [2026-05-20] architecture | Unified memory redesign

- **Humean edges**: Removed `RelationKind` enum. `EdgeData` simplified to `{weight, created_at, active}`. Edge direction enforced temporally (older node = source).
- **Dropped `known_by`**: Removed from `Node` struct, all scenario seeds, prompt schema, and MemorySystem.
- **New Node fields**: Added `trigger` and `arc_position` for Foreshadow-Trigger-Payoff tracking.
- **Thread queries**: Added `thread_containing()` and `all_threads()` using `boost::connected_components`.
- **MemorySystem refactored**: ChromaDB is now a semantic index. `store_node()` stores by `node_id`; `search_nodes()` returns node IDs. Heavy pipeline (BM25, quality scoring, entity extraction, conflict detection) removed.
- **CharacterMemory**: New per-character subjective memory system (Generative Agents-inspired). Boost graph of `MemoryNode` beliefs + text context buffer + ChromaDB index. Core retrieval, prompt building, and reflection logic in C++.
- **Python thin wrapper**: `character_agent.py` rewritten to call C++ `CharacterMemory` methods. `memory.py` refactored with shared ChromaDB client and lazy collection creation.
- **Scenario migration**: All scenarios updated — `known_by` removed, `initial_memory` added per character with `beliefs` and `context`.
- Updated `wiki/architecture/plot-graph.md` with full unified memory architecture docs.

## [2026-05-19] research | Third-wave cognitive simulation papers

- Created three new detailed paper pages for the "cognitive simulation" wave (late 2025--2026):
  - `wiki/research/papers/her-dual-layer-thinking.md` -- HER (Fudan+MiniMax, arXiv 2026): dual-layer thinking (hidden system planning + visible role monologue) trained with RL and a specialized generative reward model. State-of-the-art: +30.2 over Qwen3-32B base on CoSER. Full model/RM/data release.
  - `wiki/research/papers/humanllm.md` -- HumanLLM (Fudan+JHU, ACL 2026 Main): 244 psychological patterns (Big Five traits + cognitive biases + social influence) as interacting causal forces. Key finding: 8B beats 32B on multi-pattern dynamics -- cognitive process simulation is more parameter-efficient than behavior memorization. Negative transfer ablation shows standard SFT degrades pattern fidelity by 53%.
  - `wiki/research/papers/character-r1.md` -- Character-R1 (HIT-SZ+Baidu, arXiv 2026): 10-dimension cognitive focus framework with GRPO verifiable rewards. Validated on Qwen2.5-7B (our target family). Character-conditioned normalization for multi-NPC training.
- Updated `wiki/research/llm-roleplay-survey.md`:
  - Added "third wave" to Landscape section (approaches 8-10: role-aware reasoning, dual-layer thinking, cognitive pattern simulation)
  - Added new "Cognitive simulation methods" paper table and comparison table
  - Expanded architecture from four layers to five (added L+: Cognitive Reasoning)
  - Updated NPC tier composition table with L+ column
  - Revised "What to adopt" priority list (cognitive focus at #2, psychological patterns at #4, dual-layer thinking at #6)
  - Updated rankings: HER #1, HumanLLM #2, Character-R1 #6
  - Updated code completeness ranking, evaluation landscape, and "What to watch"
- Updated `wiki/index.md` with new "cognitive simulation (third wave)" section listing all three papers.

## [2026-05-18] research | RAR paper page + experiment setup

- Created `wiki/research/papers/rar-thinking-in-character.md` -- detailed analysis of "Thinking in Character" (Tang et al., NeurIPS 2025).
- Covers the two-stage RAR pipeline (RIA SFT + RSO DPO), practical analysis for Rhapsode, reproducibility issues found during hands-on testing, hardware requirements, and comparison table against Neeko and OCT.
- **Experiment in progress**: Stage 1 (RIA SFT) training running on RunPod RTX PRO 4500 (32GB) with Qwen 2.5 7B. Adapted configs to use Qwen template, absolute paths, gradient checkpointing. Fixed hardcoded relative paths in dataset_info.json and generated missing contrastive SFT data file.
- Updated `wiki/index.md` with new paper entry.

## [2026-05-17] architecture | Protagonist companion system design

- Created `wiki/architecture/companion-system.md` -- design document for a single protagonist companion.
- **L3 first**: LoRA identity via OCT-derived constitution -> DPO -> introspection SFT pipeline. Constitution authored by designer, re-baked between sessions.
- **L2 second**: Generative Agents observe-reflect-plan memory. Companion-specific ChromaDB collection, importance scoring, composite retrieval (recency + importance + relevance), reflection generation.
- **Deferred**: L1 (activation steering -- Qwen reliability caveat) and L1.5 (codified profiles -- more valuable for minor NPCs).
- **Key design choices**: narrative-driven presence (WorldGraph, not hardcoded), emergent goals/relationships (via reflections, not structured tracking), dynamic evolution (no pre-defined arc phases).
- **Honest unknowns flagged**: 7B DPO data quality, 7B reflection quality, automated constitution revision (unpublished), LoRA drift after multiple re-bakings, latency budget.
- Quality gate between L3 and L2: test companion voice before investing in memory infrastructure.

## [2026-05-17] research | Quality-lens re-search — new papers, quality assessment, full survey rewrite

- **Re-searched** the literature with strict quality criteria: institution reputation, venue tier, citation count, code completeness.
- **Added 3 new paper pages**:
  - `generative-agents.md`: Park et al. (Stanford+Google, UIST 2023, **4,781 citations**, 21K stars). Seminal NPC memory architecture -- observe-reflect-plan cycle. Directly applicable to Rhapsode's memory system.
  - `representation-engineering.md`: Zou et al. (CAIS+CMU+Berkeley+Stanford, **988 citations**). Foundation for ALL activation steering methods (PERSONA, CAST, ControlLM).
  - `codified-profiles.md`: Peng & Shang (UCSD, NeurIPS 2025). Converts character descriptions into executable Python functions. Even 1B models can role-play. Novel approach for minor NPCs.
- **Added quality assessment** (A/B/C/D ratings) to all 16 papers in the survey based on institution, venue, citations, and code quality.
- **Downgraded** PERSONA (D: HIT weak in AI, zero community validation) and RoleRAG (D: brand new arXiv, no validation).
- **Upgraded** OCT (A: Cambridge+AI2+Anthropic authors are the authority) and CharacterGLM (B: Tsinghua+Zhipu is top-tier Chinese AI).
- **Revised architecture** from three-layer to **four-layer**: added Layer 1.5 (Codified Profiles) for behavioral rules. Updated Layer 2 to include Generative Agents memory (observe-reflect-plan) alongside RAG.
- **Re-ranked** papers by Rhapsode relevance: Generative Agents #1 (memory architecture), OCT #2 (adversarial robustness), Codified Profiles #3 (novel, practical). PERSONA dropped from #1 to #13.
- **Added evaluation resources** section: CharacterBox (RUC+MSRA+PKU, NAACL 2025) and PersonaGym (Princeton+CMU+GT+UMD, EMNLP 2025).
- **Rewrote survey sections**:
  - "Comparative analysis" now covers all 14 papers in a unified table (training-free vs. training-based), not just the original 6 SFT papers.
  - "What to adopt" now lists 8 prioritized items mapping to specific papers and layers, not just the first-wave papers.
  - Removed stale "two-tier architecture" section (superseded by four-layer).
  - "Code completeness ranking" expanded from 6 to 10 papers.
  - "Evaluation landscape" expanded to include PersonaGym and CharacterBox as dedicated frameworks, plus Codified benchmark.
  - "Scaling properties" rewritten to map NPC tiers to specific layers and papers.
  - Removed duplicate layer descriptions left over from the three-layer → four-layer transition.
- Updated `index.md` with new paper pages and quality ratings.

## [2026-05-17] research | Character evolution — the unsolved problem

- Added new section to `llm-roleplay-survey.md`: "Character evolution: the unsolved problem."
- Analysis of how each layer handles change: L1 (steering) and L2 (RAG) evolve naturally; L3 (LoRA) is static after training.
- Four approaches evaluated: phase-based LoRA switching, invariant-core L3 + L1/L2 evolution, LoRA interpolation, periodic offline retraining.
- Recommended design: invariant-core constitutions + 2-3 phase adapters for major arcs + L1/L2 for gradual drift.
- Open questions flagged: LoRA interpolation coherence, L1-only personality change limits, player perception of phase transitions, automated constitution revision.

## [2026-05-17] research | Three-method comparison and OCT deep-dive; wiki corrections

- Added **three-method comparison** section to `llm-roleplay-survey.md`: verified performance data for activation steering vs. RAG vs. Constitutional AI LoRA, robustness hierarchy, cost comparison, and Qwen-specific findings.
- **Deep-dive on Open Character Training** in `open-character-training.md`: full training pipeline (3 stages with hyperparameters), 11 personas table, all experimental results (revealed preferences, adversarial robustness Table 2/Figure 5, prefill attack F1, coherence Table 3, general capabilities Table 4), Qwen-specific implications.
- **Corrected errors**:
  - `persona-steering.md`: removed fabricated PersonalityBench scores (5.32 and 7.83 never appeared in the paper). Replaced with actual Table 4 data (7 methods compared on LLaMA-3-8B-Instruct).
  - `persona-steering.md`: fixed misleading claim that trait compositions "don't interfere" -- paper's Appendix A.11 shows ~18% cross-trait secondary effects.
  - `open-character-training.md`: fixed model size error (paper uses Qwen 2.5 **7B**, not 72B). Fixed robustness hierarchy (paper compared 3 methods, not 4 -- removed interpolated "LoRA fine-tuning" position).

## [2026-05-17] research | Second wave — activation steering, RAG, Constitutional AI for characters

- Discovered three new families of modular character approaches beyond LoRA fine-tuning.
- **Activation steering** (Layer 1: Personality): PERSONA (ICLR 2026) achieves fine-tuning-quality personality control with zero training via vector algebra. CAST (ICLR 2025, IBM) adds conditional context-dependent steering.
- **RAG-based character memory** (Layer 2: Knowledge): ChatHaruhi (2023, 2K stars) is the most validated system, already supports local Qwen. RoleRAG (2025) adds graph-guided retrieval with cognitive boundary awareness.
- **Constitutional AI** (Layer 3: Deep Identity): Open Character Training (2025) uses human-readable constitutions + DPO, most adversarially robust method.
- Revised architecture from two-tier to **three-layer**: Personality (steering vectors) + Knowledge (RAG) + Identity (LoRA). Most NPCs need only L1+L2 (zero training).
- Created 5 new paper pages in `research/papers/`.
- Updated `llm-roleplay-survey.md` with new families, three-layer architecture diagram, revised ranking (11 papers).
- Updated `index.md` with new paper section.

## [2026-05-17] research | Refine role-playing survey for Rhapsode constraints

- Added design constraints to `llm-roleplay-survey.md`: local small LLMs only (Qwen/LLaMA 7-8B), modular architecture (no per-character full SFT).
- Added two-tier architecture recommendation: Tier 1 (prompt-based, all NPCs) + Tier 2 (LoRA-enhanced, important NPCs).
- Ranked papers by Rhapsode relevance: Neeko #1 (serving architecture), DITTO #2 (data generation), CoSER #3 (base model), RoleLLM #4 (fallback/eval), CharacterGLM #5 (profile schema), Character-LLM #6 (data method reference).
- Updated all 6 paper pages with Rhapsode-specific fit assessments and rank explanations.
- Ruled out Character-LLM (per-char SFT too expensive) and CharacterGLM (wrong model family) as direct adoption targets.
- Identified key engineering task: reimplement DITTO's self-alignment data generation pipeline on local hardware.

## [2026-05-17] research | LLM role-playing survey — 6 papers on tuning LLMs for game characters

- Created `research/llm-roleplay-survey.md`: survey of methods for fine-tuning LLMs for character role-playing.
- Reviewed 6 papers: Character-LLM (EMNLP 2023), RoleLLM (ACL 2024), DITTO (ACL 2024), CoSER (ICML 2025), Neeko (EMNLP 2024), CharacterGLM (EMNLP 2024).
- 3 fully qualify (30+ citations, open-source code): Character-LLM, RoleLLM, DITTO.
- 3 honorable mentions (below citation threshold but high relevance): CoSER, Neeko, CharacterGLM.
- Individual paper pages in `research/papers/`.
- Key takeaways: Experience Reconstruction for data generation, self-alignment over distillation, dynamic LoRA for multi-character serving, attribute/behavior decomposition for character design.
- Updated `index.md` with new research section.

## [2026-05-17] wiki | Align wiki with May 16—7 refactors

- **NodePool replaced by WorldGraph**: flat `unordered_map` replaced by Boost.Graph `adjacency_list` with typed directed edges (`Related`, `Supersedes`, `Contradicts`, `CausedBy`). `EdgeData` carries confidence, created_at, active flag. BFS neighbor queries (`neighbors_within`) for 2-hop context. Legacy save compat via `from_legacy_node_pool_json`. Updated all pages referencing NodePool.
- **Director redesigned**: no longer makes its own LLM call. `focus_payload_json()` provides graph context for the merged prompt. `apply_planned_turn()` applies transitions/new_nodes from parsed JSON. `enforce_invariants()` auto-resolves superseded/contradicted nodes via typed edges. `RetrievalCallback` removed.
- **Merged narrator prompt (single LLM call)**: `prompt.py:build_merged_prompt()` combines narrative frame, graph rules, speech cue rules, active characters, established facts, plot pressures, graph snapshot, and history backlog. LLM response split at `<<<RHAPSODE_JSON>>>` sentinel in C++ `scene_loop.cpp`.
- **SceneLoop expanded**: 4-arg `PromptCallback` (added `director_focus_json`), `CharacterSynthCallback` for NPC dialogue synthesis, `take_last_turn_outputs()` for multi-message turn output, history window defaults 8/12 (was 3/10).
- **Multi-provider LLM**: new `llm.py` with `RHAPSODE_PROVIDER` selecting Gemini or DeepSeek. `gemini.py` superseded.
- **Character synthesis**: new `character_agent.py` — local llama.cpp generates NPC dialogue from speech cues.
- **Frontend redesigned**: panel-based layout (`StatusPanel`, `StoryPanel`, `ConversationPanel`), layout store (Pinia), `sceneTextParser.ts` (markdown-it with custom dialogue/bracket/paren rules), `scene_message` protocol with `scene_kind` and `speaker`.
- **WebSocket protocol**: `scene_message` replaces `assistant_message`; multiple messages per turn (narrator + character lines); `memory.sync_resolved()` post-turn.
- **README.md**: repo layout no longer says "(future)" for core/server/frontend.
- Updated: `plot-graph.md`, `cpp-data-model.md`, `scene-loop.md`, `python-server.md`, `vue-frontend.md`, `system-overview.md`, `rhapsode-overview.md`, `index.md`, `graph.yaml`, `concepts.yaml`, `memory-system.md`, `AGENTS.md`, `README.md`.

## [2026-05-12] wiki | Full wiki rewrite — align docs with implementation

- **Problem**: The entire wiki was deleted from disk (all pages showed as `D` in git). Content had diverged significantly from the actual codebase — the old wiki described planned structures that were never built (Session layer, PlotGraph DAG, llm/ subpackage, ws.py, session.py) while missing implemented features (MemorySystem, Director, save/load, local LLM integration).
- **Rewrote all pages** to accurately reflect the working system:
  - `concepts/rhapsode-overview.md` — updated architecture summary, current state, what is and is not built
  - `concepts/narrative-philosophy.md` — added implementation status notes to each principle
  - `architecture/system-overview.md` — rewritten from aspirational to actual; clear diagram of current architecture; implementation status table separating built from planned
  - `architecture/stack.md` — actual repo layout, actual dependencies, actual build commands
  - `architecture/cpp-data-model.md` — all 10 C++ types documented (was 4)
  - `architecture/scene-loop.md` — Director integration, history windowing, resume support
  - `architecture/plot-graph.md` — split into "implemented" (Node/NodePool) and "planned" (DAG with edges)
  - `architecture/python-server.md` — actual flat structure, not the planned llm/ subpackage
  - `architecture/vue-frontend.md` — actual component tree and store implementation
  - `architecture/mvp-v0.md` — retrospective: what was built, what deviated from plan
  - `architecture/memory-system.md` — **new page** covering the full C++/Python memory pipeline
  - `decisions/ownership-split.md` — updated with MemorySystem callback boundary
  - `decisions/callback-vs-pull.md` — added retrospective
  - `decisions/coding-guidelines.md` — added retrospective
- Restored `research/literature-review.md` and `raw/sources.md` from git.
- Updated `index.md` to reflect the new page set.

## [2026-05-06] arch | Session layer — multi-scene asynchronous architecture (Option A)

- **Decision**: `PlotGraph`, `GitStore`, and `Director` are now owned by a new top-level **`Session`** class, not by `Scene`.
- `Scene` becomes a **local view** — it holds only `History` and `Characters` for one scene context.
- Multiple `SceneLoop` instances share one `Session`; each loop has a **`resolution`** parameter (default 1). Director ticks once every `resolution` turns of that loop.
- Python manages the async scheduling of multiple `SceneLoop`s (asyncio); C++ core has no async knowledge.
- **Note (2026-05-12)**: Session layer was designed but not implemented. Current architecture has Scene owning NodePool directly, with a single SceneLoop per connection.

## [2026-05-06] wiki | Literature review — 8 papers from awesome-llm-story-generation

- Created `research/literature-review.md`: formal literature review of 8 papers.
- Papers: IBSEN (ACL 2024), StoryVerse (FDG 2024), CFPG (ArXiv 2026), RecurrentGPT (2023), Generative Agents (UIST 2023), FACTTRACK (NAACL 2025), Suspenseful Stories (EACL 2024), EvoSpark (ACL 2026).
- 19 concrete ideas adopted. 4 confirmed novelty gaps. 5 key warnings extracted.
- Added Intra (Ian Bicking, 2025) practitioner design log.

## [2026-05-06] wiki | Read/write actions, input mode spectrum, multi-dimensional graph

- Added principle 6 to `narrative-philosophy.md`: read actions vs write actions.
- Added player traversal model and multi-dimensional problem to `plot-graph.md`.
- **Note (2026-05-12)**: Not implemented. Current UI is freeform-only.

## [2026-05-06] wiki | Track `wiki/` in git

- Removed `wiki/` from `.gitignore`; Obsidian vault is now version-controlled with the repo.

## [2026-05-06] wiki | Director's five rules for interesting worlds

- Added five mechanical rules to `system-overview.md` Director section.
- **Note (2026-05-12)**: Design intent only. Rules require the full DAG architecture to implement.

## [2026-05-06] wiki | Plot graph auto-merge + revert, memory importance scoring

- Added auto-merging, revert, and importance scoring designs.
- **Note (2026-05-12)**: Not implemented. Requires DAG edges.

## [2026-05-06] wiki | System overview — engineering synthesis

- Created `architecture/system-overview.md`: four subsystems, control flow, engineering constraints.

## [2026-05-06] wiki | Director as Rhapsode, generation pipeline, fortune tracker rejected

- Reframed the Director as the rhapsode. Added generation pipeline. Rejected fortune tracker.

## [2026-05-06] wiki | Plot graph architecture

- Created `architecture/plot-graph.md`. The Prophet is the graph. VCS analogy. Two loops.

## [2026-05-06] wiki | Narrative philosophy — foundational design beliefs

- Created `concepts/narrative-philosophy.md`: five principles.

## [2026-05-05] wiki | Adopted Karpathy coding guidelines

- Added ADR `coding-guidelines.md`.

## [2026-05-05] wiki | Implementation-ready detail pass

- Fleshed out `stack.md`, `mvp-v0.md`, `scene-loop.md`, `cpp-data-model.md`, `python-server.md`, `vue-frontend.md`.
- Created ADR `callback-vs-pull.md`.

## [2026-05-05] wiki | Renamed to Rhapsode, moved to Rhapsode repo

- Renamed all references from DigitalDream to Rhapsode.

## [2026-05-05] wiki | Initial LLM Wiki scaffold

- Created `AGENTS.md`, `raw/sources.md`, `wiki/index.md`, starter pages.
- Adopted Karpathy LLM Wiki pattern.

## [2026-09-03] experiment | Reader-authored character studies

- Added an isolated experiment that continues each successful blind reader into a separate candidate study. The character name is injected from configuration, no gender is supplied, and the two studies are never synthesized.
- Candidate 1 produced 3,595 words and pursued the reader's interests in bodily visibility, nobility, sacrifice, and absence. It is vivid but sometimes promotes interpretation into a total causal account.
- Candidate 2 produced 2,189 words and stayed more tightly centered on the character, especially agency, exposure, usefulness, and belonging. It is more controlled but still risks making one distinction explain too much.
- Neither candidate used volume chronology, case headings, citations, or a merged consensus. Both remain experimental checkpoint artifacts; Pass 1 and the reader letters are unchanged.
- Selected Candidate 1 for a narrow manual revision. Added Candidate 2's observations about the party as a place to be a comrade and Kazuma understanding what the character hates better than what the character wants; softened causal and totalizing claims without restructuring the study. Preserved both originals.
