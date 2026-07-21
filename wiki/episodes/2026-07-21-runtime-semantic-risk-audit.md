---
title: Runtime semantic-risk audit
date: 2026-07-21
status: audited
confidence: verified
tier: episodic
tags:
  - cpp-core
  - correctness
  - audit
sources:
  - core/src/turn_executor_narrator.cpp
  - core/src/character_memory_reflection.cpp
  - core/src/world.cpp
  - core/src/world_graph.cpp
  - core/src/weaver.cpp
  - core/src/weaver_work_queue.cpp
  - core/src/story_serialization.cpp
  - core/src/memory_system.cpp
related:
  - "[runtime architectural decoupling plan](2026-07-21-runtime-architectural-decoupling-plan.md)"
  - "[C++ runtime data model](../architecture/cpp-data-model.md)"
---

# Runtime semantic-risk audit

This audit records behavior-sensitive findings discovered during the class-internal readability
refactor. They were deliberately not changed: that refactor promised to preserve prompt bytes,
iteration order, failure behavior, save behavior, and game semantics.

Each item requires a separate decision, a focused reproducer, and its own local commit if accepted.

## Findings

### Named player speech resolution

`is_player_speech_name()` asks `resolve_cast_name()` to resolve a named player, but that resolver
explicitly skips player characters. The literal string `Player` is recognized; an authored player
name may not be. A fix would change narrator rejection behavior and therefore requires a dedicated
retry/output test.

### Reflection failure consumes perceptions

`CharacterMemory::reflect_perceptions()` clears a failed callback response, then still consolidates
the pending perceptions and applies decay. Retrying the same perceptions later is therefore
impossible. Changing this affects memory evolution and must define whether callback failure means
rollback, retain-for-retry, or consume-without-thought.

### Reflection neighbourhood is directional

Reinforcement uses `WorldGraph::neighbors_within()`, which follows outgoing edges only. Graph
comments describe a local neighbourhood, but changing traversal to include incoming edges alters
belief weights and culling. The intended direction must be decided before implementation.

### Character identity is not uniformly canonicalized

Roster lookup is case-insensitive, while death marking, memory-map keys, reflection-description
lookup, perception audience routing, and rollback cleanup contain exact-name comparisons. A global
normalization change would affect saves and authored identities, so it is not a readability edit.

### Unordered iteration is observable

WorldGraph node iteration, reflection subject buckets, thought-chain ordering, and Weaver entity
groups use unordered containers. Their order can influence serialization, prompts, tie selection,
and which duplicate Weaver group receives priority. Determinism is desirable but would change
observable prompt and output ordering.

### Save loading is not transactional

Story replaces World before every scene and manifest field has been validated. A later parse or
file failure may leave a partially loaded Story. Transactional loading is safer but changes failure
state and memory-adapter synchronization semantics.

### Memory adapter legacy state

MemorySystem retains a public `next_id` value that is persisted but is not advanced by its current
write pipeline. Removing or redefining it affects the save schema and Python binding surface.

## Safe follow-up protocol

For any accepted correction:

1. write a focused test showing both current and intended behavior;
2. change one semantic issue only;
3. run Release and Debug native tests plus Python integration tests;
4. record any prompt, save, or output change explicitly;
5. commit locally and generate an ignored patch;
6. never push from the coding-agent workflow.
