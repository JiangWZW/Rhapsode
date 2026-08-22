# Obligation capability evaluation — agent execution plan

## Mission

Build and run an offline benchmark that answers two questions separately:

1. Can a cloud LLM author and register explicit commitments accurately?
2. Can it preserve their unresolved status across sequential turns without false closure, disappearance, or unsupported mutation?

Also measure whether exposing obligations to a story generator causes fixation or unwanted pressure.

This is a capability evaluation, not an obligation-system implementation. Do not change production authority, prompts, `World`, `SceneData`, `TurnExecutor`, or save formats.

## Required reading

Read these files before changing anything:

- `AGENTS.md`
- `wiki/SCHEMA.md`
- `wiki/research/frontier-llm-long-horizon-orchestration.md`
- `wiki/episodes/2026-08-10-long-run-storyline-and-character-collapse.md`
- `wiki/architecture/long-horizon-turn-transaction-plan.md`
- `experiments/session_pipeline/run.py`
- `experiments/session_pipeline/config.toml`

Use the existing experiment infrastructure where it fits. Keep this benchmark isolated under `experiments/obligation_capability/`.

## Non-negotiable constraints

- Do not use an LLM judge as ground truth.
- Do not treat fluent prose, task completion, or enjoyment as reliability.
- Do not let summaries or graph records substitute for raw attributed evidence.
- Do not infer 300-turn reliability from 20- or 100-turn results.
- Do not pool results across model versions.
- Preserve every prompt, raw response, parsed response, retry, latency, token count, and error.
- Freeze the model identifier, provider, date, decoding settings, prompt revision, and dataset revision in every run manifest.
- Report malformed output as failure. Do not silently repair semantic fields.
- A player assertion alone cannot satisfy, waive, or fail an obligation.
- Keep authoring errors separate from lifecycle errors.

## Terms under test

An **explicit commitment** is an accepted speech act or action that identifies:

- a responsible party;
- promised terms or outcome;
- an optional counterparty or beneficiary;
- an activation or due condition when one exists; and
- exact accepted evidence.

An aspiration, prediction, threat, joke, reported promise, rejected offer, or hedged attempt is not automatically a commitment.

The benchmark evaluates three operations:

1. **Authoring:** produce character dialogue/action and its intended commitment together.
2. **Registration:** decide whether accepted player and character events create an obligation.
3. **Lifecycle:** emit only justified changes to an existing obligation ledger.

The benchmark does not test whether obligations should drive the story.

## Required repository artifacts

Create this layout:

```text
experiments/obligation_capability/
  agent-plan.md
  README.md
  schemas/
    formation-case.schema.json
    formation-output.schema.json
    lifecycle-case.schema.json
    lifecycle-output.schema.json
  cases/
    formation.jsonl
    lifecycle.jsonl
    contamination.jsonl
  prompts/
    joint-authoring.md
    registration.md
    post-hoc-extraction.md
    lifecycle-delta.md
    lifecycle-full-rewrite.md
    contamination.md
  tests/
    test_schemas.py
    test_scoring.py
    test_survival.py
    test_stub_run.py
  run.py
  score.py
  make_review_packet.py
  runs/
  reports/
```

Keep generated run artifacts out of source files. Follow the repository's existing policy for large or secret-bearing outputs.

## Experiment A: commitment authoring and registration

### Dataset

Create 200 paired cases:

- 100 positive cases containing a commitment;
- 100 negative controls containing no accepted commitment.

Use four genres and five variants per speech-act family. Use these ten families:

| Positive family | Negative-control family |
|---|---|
| Direct unilateral promise | Aspiration or hope |
| Accepted bilateral agreement | Rejected or unanswered offer |
| Conditional commitment | Hedged attempt such as “I’ll try” |
| Deadline or diegetic-time commitment | Prediction, warning, or threat |
| Collective or delegated commitment | Joke, sarcasm, or reported speech |

Vary character voice, power relationship, number of participants, word order, implicit subjects, and whether the commitment appears in dialogue or action.

Each case must contain:

- stable case ID and dataset revision;
- genre and speech-act family;
- character profile;
- attributed pre-turn transcript;
- current player input or situation;
- intended character decision: commit, refuse, counteroffer, or no commitment;
- gold commitment presence;
- gold responsible party, terms, counterparty, condition, and evidence IDs;
- explicit ambiguity notes for human reviewers.

Do not use generated model outputs as gold labels. Two humans must review the gold cases before confirmatory results are reported.

### Conditions

Run three paired conditions on the same semantic cases:

1. **Joint authoring:** one model call produces exact dialogue/action and either a matching commitment or `none`.
2. **Registration:** the model receives fixed, human-authored accepted events and emits either an obligation opening or `no_change`.
3. **Post-hoc extraction:** the model first produces dialogue/action, then a separate call attempts to extract the commitment.

The comparison between joint authoring and post-hoc extraction is an architecture test. Do not assume either is superior.

### Formation scoring

Compute per model and condition:

- commitment decision precision, recall, and F1;
- false-commitment rate on negative controls;
- exact responsible-party accuracy;
- exact counterparty accuracy when applicable;
- normalized terms accuracy;
- due-condition accuracy;
- evidence-ID validity;
- complete-record accuracy: every required field correct;
- duplicate obligation rate;
- parse and schema failure rate.

Export a blinded human-review packet for:

- whether the dialogue actually expresses the structured commitment;
- whether the commitment is explicit without relying on unstated inference;
- whether dialogue and structured terms contradict each other;
- whether the prompt makes dialogue unnaturally legalistic or out of character.

Use two independent raters and a third adjudicator. Report agreement.

## Experiment B: obligation lifecycle

### Dataset

Create 40 campaigns of exactly 20 sequential player turns. Each campaign contains one to five concurrent obligations.

Start with 12 reviewed pilot campaigns. Expand only after their gold transitions and evidence IDs pass manual review.

Every campaign must include a balanced subset of:

- unrelated conversation;
- a due condition becoming true;
- partial fulfilment;
- fulfilment by the wrong actor;
- fulfilment of only one clause;
- unsupported player claims of completion;
- another character repeating the unsupported claim;
- false retrospective events;
- renegotiation requests;
- beneficiary waiver;
- refusal without waiver;
- missed deadline;
- genuine satisfaction;
- correction of a previously misstated fact;
- two similar obligations that must not be conflated.

Represent every turn as attributed accepted events. Distinguish at minimum:

- `player_said`;
- `actor_said`;
- `actor_action_accepted`;
- `world_event_accepted`;
- `time_event_accepted`.

Each turn must have a gold delta: `no_change`, open, due, satisfied, waived, or failed, with exact evidence IDs.

### Conditions

Run these conditions in order:

1. **Teacher-forced target:** gold ledger every turn; delta-only output; raw cited evidence.
2. **Free-running target:** the model receives its own previous ledger; delta-only output; raw cited evidence.
3. **Full-rewrite ablation:** gold ledger every turn; model rewrites the complete ledger.
4. **Summary ablation:** gold ledger every turn; opening evidence is replaced by a human-authored summary.

Run a smoke subset before any full condition. Do not run expensive ablations until schema, parsing, and scoring tests pass with a deterministic stub model.

### Lifecycle scoring

Compute:

- exact delta accuracy;
- transition precision, recall, and macro-F1;
- false-opening rate;
- false-terminal-transition rate;
- unsupported player-claim closure rate;
- missed-satisfaction rate;
- incorrect due-transition rate;
- wrong-obligation mutation rate;
- evidence-ID validity;
- ledger corruption after malformed output;
- first-failure turn by failure class;
- `S(20)`, the fraction of campaigns reaching turn 20 without an obligation error;
- teacher-forced versus free-running error amplification;
- residual contamination after the first wrong transition.

Plot Kaplan–Meier survival for the free-running condition. Treat campaigns reaching turn 20 without failure as censored at the cap only when calculating time-to-failure statistics.

Do not combine formation and lifecycle scores. Lifecycle experiments begin from gold obligations.

## Experiment C: prompt contamination

Create 100 paired turns where an unresolved obligation exists but is unrelated to the player's current action.

Run three conditions:

1. no obligation information;
2. complete open-obligation ledger;
3. original attributed commitment evidence without a system instruction to advance it.

Use the same model, character profile, player input, and decoding settings across paired conditions.

Export blinded responses for human labeling of:

- unnecessary obligation mention;
- repeated pressure;
- refusal override;
- unrelated scene intrusion;
- dialogue becoming legalistic;
- obligation-driven thematic monopoly;
- damage to character voice.

Do not use lexical mention alone as the final label. A natural reference can be appropriate. Human raters must judge contextual intrusion.

## Run protocol

The harness must support three levels:

```text
smoke: 10 formation cases, 2 lifecycle campaigns, 10 contamination cases
pilot: 50 formation cases, 12 lifecycle campaigns, 30 contamination cases
full:  200 formation cases, 40 lifecycle campaigns, 100 contamination cases
```

Use the provider's lowest deterministic temperature for the primary result. Repeat a pre-registered 10% sample three times to measure output instability.

Run every candidate model on identical case IDs. Record models separately. A failed or rate-limited call remains visible in the report.

Provide commands with this interface in `README.md`:

```powershell
python -m pytest experiments/obligation_capability/tests -q
python experiments/obligation_capability/run.py --level smoke --model <model> --config <config>
python experiments/obligation_capability/score.py --run <run-directory>
python experiments/obligation_capability/make_review_packet.py --run <run-directory>
```

Adapt the flags to existing repository conventions only when necessary. Document the final commands exactly.

## Pre-registered engineering gates

These gates decide whether obligation generation may proceed to an authoritative implementation experiment. They do not establish 300-turn reliability.

### Formation gate

- creation precision at least 0.95;
- creation recall at least 0.90;
- complete-record accuracy at least 0.90;
- all cited evidence IDs exist;
- no statistically material loss of character quality against the paired control.

### Lifecycle gate

- zero player-claim-only terminal transitions in the dedicated trap set;
- exact delta accuracy at least 0.95 in the teacher-forced target condition;
- free-running `S(20)` at least 0.90;
- no silent ledger repair after parse or schema failure;
- delta-only output must not perform worse than full rewriting on false terminal transitions.

### Contamination gate

- unrelated-intrusion rate must not increase by more than five percentage points against the no-obligation condition;
- refusal override must remain zero in the dedicated refusal cases;
- report repeated pressure and thematic monopoly separately even if the aggregate gate passes.

Report confidence intervals. If the sample cannot support a gate, label it underpowered rather than passed.

## Statistical reporting

- Use paired comparisons because every condition shares case IDs.
- Report effect sizes and confidence intervals, not only p-values.
- Use Wilson intervals for proportions unless the analysis justifies another method.
- Report survival curves and first-failure distributions.
- Keep model, prompt, dataset, and decoding revisions separate.
- At least 29 independent zero-failure campaigns are required before claiming a one-sided 95% lower bound near 0.90 for campaign survival.
- Do not extrapolate `S(300)` from per-turn accuracy or `S(20)`.

## Required tests for the harness

Before paid runs, automated tests must prove that:

- invalid JSON is counted as failure;
- missing required fields are counted as failure;
- invented evidence IDs are rejected;
- a player statement cannot serve as terminal evidence;
- teacher-forced and free-running state propagation differ correctly;
- first-failure turn is stable and one-indexed;
- survival calculations handle capped campaigns correctly;
- duplicate obligation IDs are detected;
- scoring does not read gold labels while producing model outputs;
- run manifests contain prompt, dataset, and model revisions;
- secrets never enter stored prompts, manifests, or reports.

## Execution order

1. Read the required sources and record the current experiment conventions.
2. Create schemas and deterministic stub fixtures.
3. Implement parsing, scoring, manifests, and automated tests.
4. Draft the 12-campaign lifecycle pilot and 50 formation pilot cases.
5. Obtain human review of gold labels.
6. Run the smoke level with one configured cloud model.
7. Fix harness defects; never alter gold labels to improve model scores.
8. Run the pilot level and generate a blinded review packet.
9. Complete human adjudication and freeze dataset revision `v1`.
10. Run the full level for each candidate model.
11. Produce the final report and machine-readable metrics.
12. Update the wiki only with completed, human-validated results.

## Stop conditions

Stop and report rather than improvising when:

- cloud credentials or exact model versions are unavailable;
- human-reviewed gold labels are unavailable for a confirmatory claim;
- a provider silently substitutes a model version;
- prompts or outputs expose secrets;
- parsing repairs could change semantic answers;
- formation and lifecycle errors cannot be separated;
- a requested run would exceed an agreed cost limit.

The harness and smoke tests may still be completed when a paid full run is blocked.

## Final deliverables

The executing agent must leave:

- all schemas, cases, prompts, harness code, and tests;
- a frozen dataset revision and prompt hashes;
- raw run directories with manifests;
- a blinded human-review packet and adjudicated labels;
- `reports/obligation-capability-report.md`;
- `reports/metrics.json`;
- exact commands needed to reproduce each result;
- a short list of first failures with raw evidence;
- a clear decision: reject, keep shadow-only, or advance to an authority experiment.

The report must state that passing these tests demonstrates bounded commitment capability only. It does not demonstrate story progression, player-agency preservation beyond the contamination test, or 300-turn reliability.
