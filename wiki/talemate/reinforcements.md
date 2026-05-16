---
sources:
  - "talemate:src/talemate/agents/world_state/__init__.py"
  - "talemate:src/talemate/world_state/__init__.py"
  - "talemate:src/talemate/prompts/templates/world_state/update-reinforcements.jinja2"
last_updated: 2026-05-08
confidence: verified
tier: semantic
related:
  - "[[talemate/_index]]"
  - "[[talemate/context-assembly]]"
  - "[[talemate/memory-architecture]]"
  - "[[talemate/comparison]]"
tags:
  - third-party-analysis
---

# Talemate — reinforcements

Reinforcements are periodic LLM-driven Q&A pairs that refresh canonical facts about the world and characters. They operate orthogonally to vector memory. Where retrieval finds semantically similar past events, reinforcements maintain explicitly tracked world-state assertions re-validated against evolving narrative.

## Reinforcement Model

The `Reinforcement` Pydantic model — from `world_state/__init__.py:37`:


| Field            | Type | Default        | Description                                                                 |
| ---------------- | ---- | -------------- | --------------------------------------------------------------------------- |
| `question`       | str  | —              | The fact to be maintained, phrased as a question                            |
| `answer`         | str  | None           | None                                                                        |
| `interval`       | int  | 10             | Turns between re-validation                                                 |
| `due`            | int  | 0              | Turns remaining until next run                                              |
| `character`      | str  | None           | None                                                                        |
| `instructions`   | str  | None           | None                                                                        |
| `insert`         | str  | `"sequential"` | Insertion mode: `sequential` or other values for non-sequential persistence |
| `require_active` | bool | True           | Skip update if the scoped character is inactive                             |


The `as_context_line` property (line 48) formats a reinforcement for injection: `"{character}: {question} {answer}"` for character-scoped, or `"{question}: {answer}"` for global.

## Update Cycle

`WorldStateAgent.update_reinforcements` (line 568):

```python
# source: talemate:src/talemate/agents/world_state/__init__.py:568-587
async def update_reinforcements(self, ...):  # force, reset kwargs
    for reinforcement in self.scene.world_state.reinforce:
        need_active = reinforcement.require_active
        has_char = reinforcement.character
        inactive = (
            need_active and has_char
            and not self.scene.character_is_active(reinforcement.character)
        )
        if inactive:
            continue
        if reinforcement.due <= 0 or force:
            kwargs = dict(reset=reset)
            q, c = reinforcement.question, reinforcement.character
            await self.update_reinforcement(q, c, **kwargs)
        else:
            reinforcement.due -= 1
```

## `update_reinforcement` Flow (line 590)

1. Find the reinforcement by question/character.
2. Prompt the LLM via `Prompt.request`, template key `"world_state.update-reinforcements"`, with vars `scene`, `max_tokens`, `question`, `instructions`, `character`, `answer`, `reinforcement`.
3. Extract response using `AnchorExtractor` with tags `"<ANSWER>"` / `"</ANSWER>"` and `fallback_to_full=True` — lines 648–652.
4. Set `reinforcement.answer = answer` and reset `reinforcement.due = reinforcement.interval` — lines 659–660.

## The Template

The `update-reinforcements.jinja2` template:

1. Renders context: `extra-context-static.jinja2` + `extra-context-dynamic.jinja2` + character description.
2. Builds volatile context via `memory-query.jinja2`, supplying `scene.snapshot()` as the memory query.
3. Computes budget: calls `scene.context_history` with `budget` equal to `max_tokens` minus `200`, `rendered_context_tokens`, and `volatile_context_tokens`.
4. Constructs numbered scene history.
5. Asks the LLM to answer the question in context of the final dialogue line.
6. Primes response with `{{ set_prepared_response("<ANSWER>") }}` (line 98).

For `sequential` insert mode, the template instructs: "YOUR ANSWER MUST BE SHORT AND TO THE POINT. YOUR ANSWER MUST BE A SINGLE SENTENCE."

## Two Persistence Paths

### Sequential (`insert == "sequential"`)

- Builds a `ReinforcementMessage` with the answer.
- Pops any prior `ReinforcementMessage` with the same `meta_hash` from history — lines 665–667; see `max_iterations=10`.
- Calls `scene.push_history(message)` — the reinforcement lives in scrollable history (line 673).
- Uses shorter response profile — `analyze_freeform_medium_short`, line 625.

### Non-sequential (any other `insert` value)

- No `push_history` — the reinforcement does not appear in scrollback.
- If `reinforcement.character` is set: `await character.set_detail` — args `reinforcement.question`, `answer` — line 678.
- If global: `await world_state_manager.save_world_entry` — args `reinforcement.question`, `reinforcement.as_context_line`, `{}` — lines 682–686.

Sequential mode maintains a living timeline of evolving facts. Non-sequential mode treats reinforcements as stable world-state assertions that update in-place.

## Context Pins

`ContextPin` model — from `world_state/__init__.py:68`:


| Field                 | Type                 | Description                           |
| --------------------- | -------------------- | ------------------------------------- |
| `entry_id`            | str                  | ID of the pinned entry                |
| `condition`           | str                  | None                                  |
| `condition_state`     | bool                 | Current condition evaluation result   |
| `gamestate_condition` | list[ConditionGroup] | None                                  |
| `active`              | bool                 | Whether the pin is currently injected |
| `decay`               | int                  | None                                  |
| `decay_due`           | int                  | None                                  |


### Pin Lifecycle — `check_pin_conditions`, line 693

1. **Skip** gamestate-controlled pins — managed externally; lines 710–711.
2. **Initialize**: if active with decay but no `decay_due` → set `decay_due = decay` (line 713-714).
3. **Skip stable**: if active with decay and `decay_due > 1` → decrement without LLM check (line 721-726).
4. **Evaluate**: near expiry (`decay_due == 1`) or inactive, evaluate condition via LLM.
5. **Activate**: condition true → `active = True`, `decay_due = decay`.
6. **Deactivate**: `decay_due <= 0` → `active = False`, `decay_due = None`.

## Design Rationale

Reinforcements solve the "fact drift" problem in long-running sessions. Vector memory retrieves by semantic similarity. This fails for facts that should always be present regardless of topic — character relationships, world rules, established canon. The interval-based countdown balances freshness against LLM cost.

Two persistence paths serve different needs. Sequential reinforcements track evolving state — emotions, relationships. Non-sequential ones maintain stable reference facts — character backstory, world physics.

Context pins complement reinforcements by providing time-limited focus on specific topics — useful for maintaining narrative throughlines without permanent world-state entries.

## See Also

- [[talemate/context-assembly]]
- [[talemate/memory-architecture]]
- [[talemate/comparison]]

