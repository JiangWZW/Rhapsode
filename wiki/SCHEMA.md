# Rhapsode Wiki Schema

> This file defines the conventions, workflows, and structure for maintaining the Rhapsode knowledge base.
> It is the instruction manual for both human editors and LLM agents operating on this wiki.
>
> Based on [Karpathy's LLM Wiki](https://gist.github.com/karpathy/442a6bf555914893e9891c11519de94f) pattern, extended with lifecycle, graph, and automation patterns adapted from the BRG wiki.

## 1. Directory Structure

```
wiki/
  SCHEMA.md              ← You are here (the most important file)
  index.md               ← Master catalog of all pages
  log.md                 ← Chronological record of operations
  graph.yaml             ← Knowledge graph (entities + typed relationships)
  concepts.yaml          ← Concept index (concept → wiki pages, for query routing)
  supersessions.yaml     ← Page-level revision log (old claim → new claim)

  architecture/          ← System-level understanding
    stack.md
    system-overview.md
    mvp-v0.md
    scene-loop.md
    cpp-data-model.md
    python-server.md
    vue-frontend.md
    plot-graph.md
    memory-system.md

  concepts/              ← Foundational design beliefs and patterns
    rhapsode-overview.md
    llm-wiki-pattern.md
    narrative-philosophy.md

  decisions/             ← Architecture Decision Records
    ownership-split.md
    callback-vs-pull.md
    coding-guidelines.md

  research/              ← Literature and external analysis
    literature-review.md
    memory-systems-survey.md
    memory-systems-internals.md

  talemate/              ← External reference analysis of Talemate
    _index.md
    memory-architecture.md
    retrieval-pipeline.md
    summarization.md
    reinforcements.md
    context-assembly.md
    comparison.md
```

## 2. Page Template

Every wiki page MUST have this YAML frontmatter:

```yaml
---
sources:
  - core/include/rhapsode/scene_loop.h
  - core/src/scene_loop.cpp
last_updated: 2026-05-07
confidence: verified
tier: semantic
related:
  - "[[architecture/system-overview]]"
  - "[[architecture/cpp-data-model]]"
tags:
  - cpp-core
---
```

Followed by:

```markdown
# Page Title

One-paragraph summary: what this subsystem does, then its central organizing
insight in one sentence. State non-goals if they are surprising.

## (Mechanism sections)

What it does and how. Code, data structures, pipeline.
Mark key decisions vs. implementation details.

## Design Rationale

Goals, constraints, and non-goals that shaped the design.
For each key decision: what alternative was considered, why it was rejected.

## Impact

Cross-cutting issues: how this module's decisions constrain or enable other
modules, and vice versa. Name specific modules and specific effects.

## Limitations

Accidental weaknesses (not non-goals). Concrete failure cases.

## See Also

- [[related-page-1]]
- [[related-page-2]]
```

The four dimensions (What/How, Why, Impact, Limitations) do not require four literal sections. A small page may weave rationale into mechanism sections. But every page must address all four somewhere.

### Frontmatter Field Reference

| Field | Required | Values | Description |
|-------|----------|--------|-------------|
| `sources` | yes | list of paths | Source files this page documents (repo-relative for Rhapsode, `talemate:` prefix for external) |
| `last_updated` | yes | ISO 8601 date | When content was last modified |
| `confidence` | yes | `verified` \| `likely` | Verification depth (see Memory Lifecycle) |
| `tier` | yes | `working` \| `episodic` \| `semantic` \| `procedural` | Knowledge consolidation level |
| `related` | yes | list of wikilinks | Cross-references to other pages |
| `tags` | yes | list of layer tags | `cpp-core`, `python-server`, `vue-frontend`, `cross-layer`, `design`, `third-party-analysis` |

### Rules

- **Frontmatter is mandatory.** Every page has all fields listed above.
- **Summary first.** The opening paragraph must be self-contained.
- **Cross-reference aggressively.** Use `[[wiki-links]]`. On first mention of a key concept, link it.
- **See Also section.** Every page ends with links to related pages.
- **Nothing below "likely."** Content that cannot reach at least `likely` confidence must be clarified against source code or removed.

## 3. Memory Lifecycle

### Confidence Labels

| Label | Meaning | When to assign |
|-------|---------|----------------|
| `verified` | Claims checked line-by-line against source code | After direct code review with exact citations |
| `likely` | Claims checked at function/architecture level, or inferred from verified patterns | After reading relevant code but not line-by-line verification |

Content below `likely` (speculative, uncertain, single-source guesses) is **not permitted** in the wiki. Clarify it or remove it.

### Consolidation Tiers

| Tier | Description | Lifetime | Example |
|------|-------------|----------|---------|
| `working` | Raw observations from a single session | Short — promote or discard within days | Debug log output, quick notes |
| `episodic` | Structured session summaries or decisions | Medium — kept as historical record | `decisions/ownership-split.md` |
| `semantic` | Cross-session facts verified against source | Long — the backbone of the wiki | `architecture/scene-loop.md` |
| `procedural` | Workflows and patterns seen across 3+ subsystems | Permanent | `architecture/mvp-v0.md` |

### Tier Promotion

- `working` → `episodic`: when raw notes are written up into a structured page
- `episodic` → `semantic`: when claims are verified against source code and integrated into subsystem pages
- `semantic` → `procedural`: when a pattern is observed across 3+ subsystems and abstracted into a workflow

### Decay and Staleness

- **Source-file change**: when a source file is modified, all affected wiki pages decay from `verified` to `likely`.
- **Time-based decay**: pages not re-verified within one working week decay from `verified` to `likely`.
- **Staleness callout**: any page at `confidence: likely` SHOULD display a visible warning:

```markdown
> [!warning] This page has not been verified since YYYY-MM-DD. Source code may have changed.
```

- **Re-verification**: reading the source files and confirming claims resets `last_updated` to today and `confidence` back to `verified`.

## 4. Knowledge Graph

The file `graph.yaml` is a structured entity-relationship registry. It is a machine-navigable index for agents to traverse when answering queries.

### Entity Types

| Type | Description |
|------|-------------|
| `class` | C++ or Python class |
| `struct` | C++ struct |
| `concept` | Abstract concept (e.g., "plot graph", "resolved memory") |
| `module` | Python module or Vue component |

### Relationship Types

| Relation | Meaning | Example |
|----------|---------|---------|
| `owns` | A contains or manages B | Session owns PlotGraph |
| `calls` | A invokes B at runtime | SceneLoop calls Director |
| `depends-on` | A requires B to function | Director depends-on ResolvedMemory |
| `feeds` | A's output is B's input | Director feeds prompt builder |
| `informs` | External analysis A shaped design of B | talemate/retrieval-pipeline informs ResolvedMemory |
| `supersedes` | A replaced B | (recorded in supersessions.yaml) |

### Maintenance

- When adding a new entity to the wiki, add it to `graph.yaml`.
- When discovering a new relationship, add it with the source page.
- Keep ~20-40 key entities, not every minor variable or function.

## 5. Concept Index

The file `concepts.yaml` maps concepts to wiki pages for query routing.

### Entry Format

```yaml
- name: memory-retrieval
  aliases: [ChromaDB, vector search, embedding, ResolvedMemory, RAG]
  desc: How resolved plot nodes are stored and retrieved via semantic similarity
  pages: [python-server, system-overview]
```

### Fields

| Field | Required | Description |
|-------|----------|-------------|
| `name` | yes | Canonical concept slug (lowercase, hyphen-separated) |
| `aliases` | yes | Code identifiers, informal terms, abbreviations |
| `desc` | yes | One-line description for semantic matching |
| `pages` | yes | Wiki page slugs (without directory or `.md`) |

### Maintenance Rules

- When adding a new wiki page, add concepts for its key topics.
- When a question doesn't match any concept, add a new entry.
- Aliases should include both code identifiers and natural language terms.

## 6. Supersession Protocol

When new knowledge replaces old knowledge, record it in `supersessions.yaml`.

### Format

```yaml
- date: 2026-05-07
  page: architecture/python-server
  summary: Embedding model changed from Alibaba-NLP/gte-base-en-v1.5 to BAAI/bge-base-en-v1.5
  reason: Custom model code caused IndexError with recent torch/transformers versions
```

### Process

1. Update the wiki page with corrected information.
2. Add a supersession entry to `supersessions.yaml`.
3. If the old claim existed in `graph.yaml`, update the relationship.
4. Set `last_updated` on the affected page.

## 7. Crystallization

Crystallization distills a completed investigation or debugging session into a structured wiki page.

### Episode Template

```yaml
---
sources:
  - (files touched during the session)
last_updated: 2026-05-07
confidence: verified
tier: episodic
related:
  - "[[relevant-page]]"
tags:
  - cross-layer
crystallized_from: "description of the session or chat ID"
---
```

```markdown
# Episode: [Title]

## Question
What was being investigated?

## Findings
What was discovered? (with code citations)

## Lessons
Standalone facts that could be promoted to semantic-tier pages.

## Wiki Impact
Pages created or updated as a result of this episode.

## See Also
- [[related-page]]
```

## 8. Code Citation Conventions

### Embedded Snippets

Use fenced code blocks with a `source:` annotation:

````markdown
```cpp
// source: core/include/rhapsode/scene_loop.h:12-28
class SceneLoop {
public:
    void load_scene(Scene& scene);
    // ...
};
```
````

### Inline References

Use backtick citations: `core/src/scene_loop.cpp:45-67` or `server/rhapsode/memory.py:83-128`.

### External Source Citations

For files outside this repo, prefix with the project name:

```
// source: talemate:src/talemate/agents/memory/__init__.py:530-609
```

### When to Embed vs. Reference

| Embed | Reference Only |
|-------|---------------|
| Struct/class definitions | Function bodies > 30 lines |
| Key algorithms | Boilerplate |
| Constants and schemas | Error handling paths |

## 9. Source File Registry

Maps source files to their primary wiki pages.

| Source File | Primary Wiki Pages |
|---|---|
| `core/include/rhapsode/scene_loop.h` | [[architecture/scene-loop]], [[architecture/cpp-data-model]] |
| `core/include/rhapsode/director.h` | [[architecture/system-overview]], [[architecture/cpp-data-model]] |
| `core/include/rhapsode/node.h`, `node_pool.h` | [[architecture/plot-graph]], [[architecture/cpp-data-model]] |
| `core/include/rhapsode/scene.h`, `history.h` | [[architecture/cpp-data-model]] |
| `core/include/rhapsode/memory_system.h` | [[architecture/memory-system]], [[architecture/cpp-data-model]] |
| `core/src/memory_system.cpp` | [[architecture/memory-system]] |
| `server/rhapsode/app.py` | [[architecture/python-server]] |
| `server/rhapsode/memory.py` | [[architecture/memory-system]], [[architecture/python-server]], [[talemate/comparison]] |
| `server/rhapsode/validator.py` | [[architecture/memory-system]], [[architecture/python-server]] |
| `server/rhapsode/lemmatization.py` | [[architecture/memory-system]], [[architecture/python-server]] |
| `server/rhapsode/gemini.py` | [[architecture/python-server]] |
| `server/rhapsode/prompt.py` | [[architecture/python-server]], [[talemate/comparison]] |
| `frontend/src/` | [[architecture/vue-frontend]] |
| `CMakeLists.txt` | [[architecture/stack]] |
| `server/pyproject.toml` | [[architecture/stack]] |
| `bindings/bind_rhapsode.cpp` | [[architecture/cpp-data-model]] |

## 10. Workflows

### Ingest (adding new source material)

1. Read the source file fully.
2. Consult the Source File Registry to identify affected wiki pages.
3. **Integrate** new knowledge into existing pages — do not just append.
4. If new knowledge contradicts existing claims, follow the Supersession Protocol.
5. If the source covers a topic not yet in the wiki, create a new page.
6. Add new entities/relationships to `graph.yaml`.
7. Update `index.md` with any new pages.
8. Append an entry to `log.md`.

### Query (answering questions using the wiki)

1. Read `index.md` and `graph.yaml` to find relevant pages.
2. Read those pages.
3. Synthesize an answer with `[[wiki-link]]` citations.
4. If the answer reveals a gap, create or update a page.

### Lint (periodic health check)

- Broken `[[wiki-links]]` pointing to pages that don't exist.
- Orphan pages with no inbound links.
- Stale code snippets whose source has changed.
- Missing `See Also` sections.
- Confidence decay past the verification window.
- Graph consistency: entities referenced in pages but missing from `graph.yaml`.

## 11. Automation Hooks

Agent instructions (no Python tooling yet — future work).

### on-source-change

1. Look up affected wiki pages in the registry.
2. Set `confidence: likely` on each affected page.
3. Add the staleness callout.
4. Log the change in `log.md`.

### on-session-end

1. Crystallize the session into an episode page if it produced new insight.
2. Extract standalone facts and update relevant semantic-tier pages.
3. Add new entities/relationships to `graph.yaml`.
4. Update `index.md` and `log.md`.

### on-contradiction

1. Determine which claim is more likely correct.
2. Update the wiki page with corrected information.
3. Add a supersession entry.

### on-wiki-session-start

1. Read `index.md` and `graph.yaml` to orient.
2. Check for pages past the decay window.
3. Report stale pages and offer to re-verify.

## 12. Quality and Self-Correction

### Minimum Quality Bar

- No content below `likely` confidence.
- Every claim traceable to a source file via `sources` frontmatter.
- Code snippets include `source:` annotations with file and line numbers.

### Contradiction Resolution

When a contradiction is detected:

1. Identify both claims and their source pages.
2. Check which claim has stronger evidence (more recent source, more sources, direct code citation vs. inference).
3. Update the weaker claim's page with the correct information.
4. Record the supersession in `supersessions.yaml`.

### Self-Healing Lint

The lint process should automatically fix what it can:

- Broken cross-references → search for the correct page name and fix the link.
- Missing `See Also` entries → add based on `related` frontmatter.
- Stale `last_updated` → add staleness callout (do not change content without re-reading source).

## 12.5 Page Health Scoring & Rewrite Protocol

Iterative maintenance degrades page quality through "patch stacking" — corrections inserted into otherwise coherent text that break the writing's flow. This section defines how to detect degraded pages and how to restore them.

### Health Score

Each page has a computed health score from 1 to 5, generated by `wiki_lint.py --health`. The score is based on automated detection of eleven quality signals (seven structural + four readability):

| Signal | Penalty | What it detects |
|--------|---------|-----------------|
| Patch scars | -0.5 each (max -2.0) | "Note:", "Update:", "Current status:", "was later changed", correction callouts |
| Temporal layers | -0.3 each (max -1.5) | Dates in prose, "previously", "currently", "as of", "no longer" |
| Blockquote ratio | -1.0 if >15% | Excessive callouts/asides relative to content |
| Parenthetical density | -0.5 if >5 per 100 lines | Long `(...)` constructions indicating inline caveats |
| Supersession load | -0.3 each (max -1.5) | Number of entries in `supersessions.yaml` for this page |
| Appendix sections | -0.5 each (max -1.0) | "Additional Notes", "Updates", "Historical Context" headings |
| Cross-page contradiction | -1.0 each | Symbol described as active in one page, removed in another |
| Long sentences | -0.1 each (max -1.5) | Sentences exceeding 26 words (see Readability Lint) |
| Passive voice ratio | -0.5 if >25%, -1.0 if >40% | High ratio of passive-voice sentences in prose |
| Dense paragraphs | -0.2 each (max -1.0) | Paragraphs exceeding 5 sentences or ~120 words |
| Wall-of-text sections | -0.3 each (max -1.0) | Sections >300 prose words without structural breaks |

### Thresholds

| Score | Status | Action |
|-------|--------|--------|
| >= 4.0 | OK | Patch maintenance is safe |
| 3.0–3.9 | CAUTION | Review before patching; prefer section-level rewrite when touching this page |
| < 3.0 | REWRITE NEEDED | Do not patch. Follow the rewrite procedure below |

### `health_score` Frontmatter Field

Pages may carry an optional `health_score` field in their YAML frontmatter. This is a cached value written by the linter (`wiki_lint.py --health --fix`) so that status reports can show page health without re-running the full scoring pipeline.

```yaml
health_score: 4.2  # optional, computed by linter
```

### Content Coverage

Every page must address four dimensions. A page that only covers "What / How" is a feature list, not a wiki page.

| Dimension | What to write | What to avoid |
|-----------|---------------|---------------|
| **What / How** | Mechanism, data structures, code. Mark **key decisions** vs. **implementation details**. | Mixing the two without distinction. |
| **Why** | Goals and constraints that shaped the solution space. **Non-goals**. For each key decision: what alternative was considered and why it was rejected. | Guessing rationale. Listing decisions without alternatives. |
| **Impact** | Cross-cutting issues: how this module's decisions constrain or enable other modules. Name specific modules and specific effects. | Vague claims. Isolated bullet lists. |
| **Limitations** | Accidental weaknesses — things that should work better but don't. Separate from non-goals. Concrete failure cases. | Hiding problems. Mixing non-goals with weaknesses. |

### Writing Style

Write as if explaining to a peer developer. Start with technical content immediately — no preamble, no generalizations. Focus on the how and why of implementations.

**Banned words** (without substantive explanation): "crucial", "ideal", "key", "robust", "enhance", "powerful", "efficient", "elegant". These are filler. Say what is actually happening instead.

**Banned patterns:**

- Starting sentences with "By" ("By doing X, we achieve Y"). State the result directly.
- Transitions between sections ("With sort keys defined...", "Now that we understand..."). If sections are ordered logically, transitions are unnecessary.
- Narrating code. Name the function, cite the file, show the code. The reader can read.
- Patch scars ("Note:", "Update:", "Current status:"). Rewrite instead.
- Preambles ("This page covers...", "In this section we will..."). Start with the content.

**Vary sentence structure.** If three paragraphs follow the same pattern, rewrite some of them.

**No temporal layers.** Describe the current state as if writing from scratch. History goes in `supersessions.yaml` or a single "History" subsection at the end.

**Self-review** — after writing wiki prose, re-read and ask:

- Can this sentence be cut without losing information?
- Am I explaining something the code/diagram already shows?
- Is the abstraction level right for where the reader is on the page?
- Did I use a filler word that I can replace with a concrete statement?
- Do three consecutive paragraphs have the same sentence structure?
- Does the opening state the central insight, or just describe what the system does?
- For each design choice: did I say what alternative was considered and why it lost?
- Did I mark which decisions are key (system-shaping) vs. implementation details (swappable)?
- Does the Impact section name specific modules and specific cross-cutting effects?
- Are my limitations accidental weaknesses, or did I mix in deliberate non-goals?

### Readability Lint (automated checks)

The following are enforced by `wiki_lint.py` as a mechanical safety net. They catch the worst offenses but do not make writing good — that requires judgment.

- **Sentence length**: 26 words max. Backtick tokens count as 1 word.
- **Passive voice**: warn if >25% of sentences are passive.
- **Paragraph size**: 5 sentences or 120 words max.
- **Section size**: 300 prose words max without a structural break.

### Rewrite Procedure

When a page's health score drops below 3.0, or when it has 3+ patch scars AND a supersession count >= 2:

1. **Extract facts**: Read the current page and list every factual claim, stripping away the prose.
2. **Verify against source**: Read every source file in the page's `sources:` frontmatter. Confirm each fact. Discard outdated claims. Note new facts.
3. **Write from scratch**: Thread verified facts through a logical structure. Do not copy-paste from the old page.
4. **Consolidate history**: Move superseded claims to `supersessions.yaml`.
5. **Validate**: Run `wiki_lint.py --health <page-slug>` and confirm the score is >= 4.0.
6. **Update metadata**: Set `last_updated` to today, `confidence: verified`.

### Patch vs. Rewrite Decision

| Situation | Action |
|-----------|--------|
| Single fact changed, page score >= 4.0 | Patch: update the fact in place, maintain surrounding flow |
| Multiple facts changed, page score >= 3.0 | Section rewrite: rewrite the affected section(s) from scratch |
| Page score < 3.0 | Full page rewrite following the procedure above |
| Cross-page contradiction involving this page | Fix the contradiction; if this page is the stale one, rewrite the affected section |

## 13. Naming Conventions

- **File names**: lowercase, hyphen-separated (`scene-loop.md`).
- **Wiki links**: match the file name without extension (`[[scene-loop]]`).
- **Dates**: ISO 8601 (`2026-05-07`).
- **YAML files**: lowercase, hyphen-separated (`graph.yaml`).
- **Tags**: lowercase, hyphen-separated (`cpp-core`, `python-server`).
- **Episode files**: descriptive, hyphen-separated (`flicker-bug.md`).
- **External sources**: prefixed with project name (`talemate:src/talemate/...`).
