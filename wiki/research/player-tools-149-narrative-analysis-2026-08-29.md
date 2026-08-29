---
title: Player-tools 149 — cemetery loop and board-reading
date: 2026-08-29
tags: [session-pipeline, player-agent, tools, fork-merge, evaluation]
confidence: verified
sources:
  - experiments/session_pipeline/runs/player-tools-300turn/manifest.json
  - experiments/session_pipeline/runs/player-tools-300turn/report.json
  - experiments/session_pipeline/runs/player-tools-300turn/turns.jsonl
  - experiments/session_pipeline/runs/player-tools-300turn/llm_profile.jsonl
  - experiments/session_pipeline/runs/player-tools-300turn/console.log
  - experiments/session_pipeline/runs/player-tools-300turn/story.txt
  - experiments/session_pipeline/runs/player-tools-300turn/saves/story.json
  - experiments/session_pipeline/runs/player-tools-300turn/saves/world.json
  - experiments/session_pipeline/player_agent.py
  - experiments/session_pipeline/guides/fork_merge.md
  - experiments/session_pipeline/protocol.md
related:
  - "[[research/motif-collapse-default-guide-300]]"
  - "[[episodes/2026-08-10-long-run-storyline-and-character-collapse]]"
---

# Player-tools 149 — the cemetery loop

**Run:** `experiments/session_pipeline/runs/player-tools-300turn` — resumed the 3-turn guild save, `fork_merge` guide, new player tool loop (flash, thinking off, `NARRATOR_TOOLS` via `Story.dispatch_tool`). Target 300. Stopped at **149 / 300** on DeepSeek **402 Insufficient Balance** during a scheduler hop after fork `konosuba_f180_0`. Timeouts 0. Wall clock **10.72 h**.

Reading edition (public + minds): [player-tools-149-story-with-minds.md](player-tools-149-story-with-minds.md).

**Thesis.** The player tool loop works. It did not prevent long-run collapse. It changed the collapse mode. `default-guide-300` never forked and locked onto a candle/covenant motif. This run forked and merged for real — then reissued the same cemetery send-off six times while the player wrote board language into the action line. The alligator/marsh quest died at noon because the player left and never came back.

---

## 1. Headline numbers

| Metric | Value |
|---|---|
| Eval turns completed | 149 / 300 (`TurnError`) |
| Story clock at stop | `konosuba` turn 151; live scenes `konosuba` + `konosuba_f180_0` |
| Forks / merges / concludes | 9 / 7 / 1 |
| Graph nodes in save | 974 |
| Player turns that called a read tool | 88 / 150 clusters (58.7%) |
| Tool calls | `list_scenes` 89, `query_graph` 75, `query_mind` 13, `query_history` 6 |
| First-person actions | 149 / 149 |
| One-line actions | 28 / 149 (protocol asks for one short line) |
| Mean / median action chars | 623 / 349 (longest 5,051 on eval turn 17) |
| Board/guide leak in the action | 43 meta, 37 planning |
| Empty narrator beats | 10 (eval turns 11, 17, 43, 50, 67, 88, 94, 107, 125, 141) |
| Narrator 10-round tool cap | 29 |
| Post-turn failures | 1 |
| Ready / idle mean | 79.6 s / 255.3 s (p95 idle 517.9 s) |
| Player share of profiled API wall | **0.83%** (8.6 min vs 17.3 h of hop wall) |

The automated report flagged empty beats and `length_collapse=0.586`. Repetition 0.158 is under the old 0.45 threshold. That score does not see “same cemetery order, sixth time.”

---

## 2. The tool loop is real

`llm_profile.jsonl` has 259 `stage=player` hops, all `thinking=false`, model `deepseek-v4-flash`. Mean 1.73 tool rounds per decide (max 4). Every advertised read tool fired. The first decide of this continuation already called `list_scenes` + `query_graph`.

That is the intended difference from the old player, which stuffed `list_scenes` into the prompt and never entered `complete_with_tools`.

It is also cheap. Profiled wall by stage:

| Stage | hops | wall h | share |
|---|---:|---:|---:|
| Narrator `konosuba` | 1,345 | 8.08 | 46.6% |
| Monologue | 445 | 5.30 | 30.6% |
| Fork narrators (8 scenes) | 232 | 1.53 | 8.8% |
| Perception | 441 | 1.34 | 7.7% |
| Expiry | 921 | 0.43 | 2.5% |
| Lifecycle + scheduler | 777 | 0.34 | 2.0% |
| **Player** | **259** | **0.14** | **0.8%** |
| Weave + downsample | 192 | 0.15 | 0.9% |

The balance died on a **scheduler** call after a fork, not on the player. Topping up and resuming would still spend almost all money on narrator + monologue.

---

## 3. Fork / merge actually fired

Unlike `default-guide-300` (0 forks), lifecycle applied the coded ops.

| Scene | Cast | Intention (abridged) | Fate |
|---|---|---|---|
| `f4_0` | Aqua | Cemetery, sulk-drunk purify | merged |
| `f12_0` | Megumin, Darkness, Luna | Hold marsh net, wait until noon | **concluded** (player never returned) |
| `f69_0` | Aqua | Cemetery again, then the bottle | merged |
| `f71_0` | Megumin, Darkness | Help purify, return for bottle | merged |
| `f87_0` | Megumin, Darkness | Hold the yew shade | merged (short) |
| `f118_0` | Aqua, Darkness | Back through frost to cemetery | merged |
| `f138_0` | Aqua | Purify every row, report back | merged |
| `f157_0` | Aqua | Deeper into the cemetery | merged |
| `f180_0` | Aqua | Cemetery / Holy Aura (again) | **still live** when the run died |

The first cycle is the mechanism test working: send Aqua, travel, greet, merge `f4_0`. The marsh fork is the cost of that test: the brief said “if an off-stage storyline exists, that is the only job this turn,” so the player abandoned the alligator job to chase Aqua. Noon hit; `f12_0` concluded. The marsh is never played.

After that, the same cemetery send-off is the plot. Six Aqua (or Aqua+) cemetery forks. The brief said split once, then stop splitting; after reunion stay together. The board + `list_scenes` keep showing a cemetery residue, so the player issues the order again.

---

## 4. The new leak: the player narrates the engine

`_is_bad_action` requires first person and rejects NPC ventriloquism. Every action passed. It does not reject length, paraphrase, or engine vocabulary.

Eval turn 2 already writes: “That's the send-off moment. The fork is live.” Turn 12: “Per my brief… The mechanism test's merge condition is explicit.” Turn 149: “The plot chain shows Aqua's cemetery storyline is live… The brief says if there…”

43 / 149 actions match fork/merge/storyline/`konosuba_f`/brief language. 37 match planning (“Per my brief”, “the other thread”, “I need to”). The longest actions are not play — they are the model thinking out loud after `list_scenes` / `query_graph`, then appending a shove.

Protocol: “ONE short in-character action.” 28 / 149 are one line. Mean 3.15 lines. Turns 11–20 average **1,393** chars because the player starts dumping Situation essays; turn 17 is 5,051 chars and the narrator beat is empty.

The tools did what we asked: they grounded the player on the board. The player then performed the board.

---

## 5. Empty beats and narrator tool-cap

Ten narrator messages are empty. They are spaced (~every 14–18 eval turns), not clustered at the end. Turn 17 pairs the longest player dump with a blank assistant beat.

The main-scene narrator hit the 10-round tool cap **29** times (`console.log`). That is the same class of failure as earlier long runs: pro + thinking + read tools, no prose. One `post-turn failed` at 04:14.

Perception empty-content appears twice (reasoning tokens, no visible text). Not the stop reason.

---

## 6. What this says about the player agent

Correct:

- Flash, thinking off, `complete_with_tools`, `dispatch_tool`.
- Situation from C++ (`player_situation`), not a Python `list_scenes` stuffer.
- On-stage address only in the sampled actions; first person always.
- Tools used on a majority of turns; `list_scenes` is the default reach.

Not correct, and not caught:

- One short line.
- Do not repeat / reverse the last beat (cemetery shove is the motif).
- Do not write the brief or the board into the action.
- Split once. The guide plus a live off-stage row become a re-fork pump.

The evaluator still reports “0 cast gaps, repetition 0.16.” The interesting failure is qualitative: **obligation recycle**, not string-overlap.

---

## 7. Comparison to `default-guide-300`

| | default-guide-300 | player-tools-149 |
|---|---|---|
| Player tools | none (prompt stuffer) | 58.7% of decides |
| Forks | 0 | 9 |
| Collapse motif | covenant / candles / toasts | cemetery purify / yew / bottle |
| Quest | never played | alligator issued, then concluded off-stage |
| Stop | MaxTurns 300 | balance 402 at 149 |
| Report | 5 empty beats, length_collapse 0.72 | 10 empty, length_collapse 0.59 |

Tools and the fork/merge brief moved the engine. They did not add an exogenous plot. The closed loop now has a **storyline board** in the player prompt, so the attractor is an obligation the player can re-issue instead of a candle toast.

---

## 8. If this run is continued

Resume from `server/saves` after topping up. Do not treat 151 more turns as a second independent sample — Aqua is already forked to the cemetery again (`f180_0`). A clean 300 would need `reset.bat` and, if the question is “does the tool loop play,” a weaker or empty guide so the cemetery pump is not pre-loaded.
