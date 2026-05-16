---
sources:
  - mem0ai/mem0 GitHub commit a488e19 (v3 pipeline port)
  - mem0ai/mem0 memory/main.py (main branch, May 2026)
  - docs.mem0.ai/migration/oss-v2-to-v3
  - A-Mem arXiv:2502.12110v11 (NeurIPS 2025)
last_updated: 2026-05-09
confidence: verified
tier: semantic
related:
  - "[[research/memory-systems-survey]]"
  - "[[architecture/memory-system]]"
tags:
  - research
  - memory-architecture
---

# Memory Systems Internals: How Mem0 v3 and A-Mem Actually Work

This document explains the storage and retrieval mechanisms of Mem0 v3 and A-Mem
at the implementation level, aimed at engineers without AI/ML background.

---

## Prerequisite: What is an Embedding?

An embedding is a fixed-size array of floats that represents the *meaning* of a
text string. A neural network (the "embedding model") takes text in, outputs a
float array:

```
embed("Trader at the tavern") → float[768]
    = [0.0234, -0.1892, 0.0741, ..., 0.0412]

embed("A trader at the inn") → float[768]
    = [0.0251, -0.1847, 0.0698, ..., 0.0389]   ← very similar numbers

embed("Purple sky at dawn") → float[768]
    = [-0.1203, 0.0542, 0.2891, ..., -0.0912]  ← very different numbers
```

Texts with similar meaning produce vectors that point in similar directions.
The similarity measure is **cosine similarity**: the dot product of two
L2-normalized vectors. Identical direction = 1.0, orthogonal = 0.0.

This is identical to `dot(normalize(a), normalize(b))` in GLSL/HLSL.

## Prerequisite: What is a Vector Store (ChromaDB)?

A spatial index for high-dimensional points. Analogous to a BVH or kd-tree, but
for 768D space. Each entry has:

```
struct VectorStoreEntry {
    string   id;           // unique key
    float[]  embedding;    // position in meaning-space (768 or 1536 dims)
    string   document;     // the original text
    map      metadata;     // arbitrary key-value pairs (not used in search)
};
```

The fundamental operation is **k-nearest-neighbor**: "given this query point,
find the k entries whose embeddings are closest by cosine similarity."

The metadata is stored alongside each entry but is NOT used for the proximity
search — it's extra data you can filter by after the spatial query returns.

## Prerequisite: What is BM25?

A text-matching algorithm. Think of it as smart `grep` that scores results:

1. Tokenize query into words: `"merchant scar hand"` → `["merchant", "scar", "hand"]`
2. For each stored document, count how many query words appear in it
3. Weight by rarity — rare words (like "scar") score higher than common words
   (like "the")
4. Return documents sorted by score

BM25 is purely lexical: "trader" does NOT match "merchant" even though they
mean the same thing. It catches exact names, numbers, and specific terms that
embedding similarity might miss.

---

## Mem0 v3: Complete Architecture

### Data Structures

Mem0 v3 maintains **two collections** in the vector store:

```
MAIN COLLECTION: "{collection_name}"
┌───────────────────────────────────────────────────────────────────┐
│ Entry                                                             │
│   id:        "mem_0042"                                           │
│   document:  "Alice prefers hiking on weekends"                   │
│   embedding: float[1536] of embed("Alice prefers hiking...")      │
│   metadata:                                                       │
│     user_id:          "user_alice"                                 │
│     hash:             "a3f2b1..."  (MD5 of document, for dedup)   │
│     created_at:       1715234567                                  │
│     text_lemmatized:  "alice prefer hike weekend"  (for BM25)     │
└───────────────────────────────────────────────────────────────────┘

ENTITY COLLECTION: "{collection_name}_entities"
┌───────────────────────────────────────────────────────────────────┐
│ Entry                                                             │
│   id:        "entity_uuid_1"                                      │
│   document:  "Alice"                                              │
│   embedding: float[1536] of embed("Alice")                        │
│   metadata:                                                       │
│     data:               "Alice"                                   │
│     entity_type:        "PROPER"                                  │
│     linked_memory_ids:  ["mem_0042", "mem_0078", "mem_0156"]      │
│     user_id:            "user_alice"                               │
└───────────────────────────────────────────────────────────────────┘
```

### How Entity Extraction Works (spaCy)

spaCy is a Python NLP library that contains a pre-trained neural model
(`en_core_web_sm`, 12 MB) capable of:
- Tokenization (splitting text into words)
- Part-of-speech tagging — labeling each word as noun, verb, adjective, etc.
- Dependency parsing — determining grammatical relationships between words
- Named Entity Recognition — identifying proper nouns, organizations, etc.
- Noun chunk detection — finding multi-word noun phrases

Mem0's `extract_entities()` function uses spaCy to find four types of entities:

#### 1. PROPER noun sequences (capitalized words)

```python
# Walk through tokens. When a capitalized word tagged PROPN/NOUN/ADJ is found,
# collect consecutive capitalized words — allowing connectors like "of", "the".
# Only keep if at least one word is mid-sentence (not just sentence-start caps).

Input: "I met Sarah Chen at the New York office yesterday"

spaCy tags:
  "I"        → PRON
  "met"      → VERB
  "Sarah"    → PROPN  ← capitalized + proper noun
  "Chen"     → PROPN  ← capitalized + proper noun (continues sequence)
  "at"       → ADP
  "the"      → DET
  "New"      → PROPN  ← capitalized + proper noun
  "York"     → PROPN  ← continues
  "office"   → NOUN   ← lowercase → sequence ends
  "yesterday"→ NOUN

Extracted: [("PROPER", "Sarah Chen"), ("PROPER", "New York")]
```

The algorithm filters out generic capitalized words (like "Works", "Items") and
sentence-start capitalization using a hardcoded blocklist of ~150 generic words.

#### 2. QUOTED text (anything in quotes)

```python
# Simple regex: capture text inside "..." or '...'
Input: 'She said "trail running" is her new hobby'
Extracted: [("QUOTED", "trail running")]
```

#### 3. COMPOUND noun phrases (multi-word nouns)

```python
# spaCy identifies "noun chunks" — grammatical phrases headed by a noun.
# Mem0 extracts multi-word compounds, filtering generic heads.

Input: "The machine learning pipeline needs a database connection pool"

spaCy noun chunks: ["The machine learning pipeline",
                    "a database connection pool"]

After filtering determiners (The, a) and generic endings:
Extracted: [("COMPOUND", "ML pipeline"),
            ("COMPOUND", "db pool")]
```

The filtering removes chunks where the head noun is in a "generic" list
`_GENERIC_HEADS` — thing, stuff, way, time, experience, situation, case, etc.
— about 100 words that are too vague to be useful entities.

#### 4. Fallback: single NOUN words (uncommon)

Only used when nothing else matched; rarely produces results.

#### Full Example

```
Input text: "Alice mentioned she started trail running at Central Park"

spaCy processing:
  Alice         → PROPN (proper noun)
  mentioned     → VERB
  she           → PRON
  started       → VERB
  trail         → NOUN
  running       → NOUN
  at            → ADP
  Central       → PROPN (proper noun)
  Park          → PROPN (proper noun)

Noun chunks: ["Alice", "trail running", "Central Park"]

extract_entities() returns:
  [("PROPER", "Alice"),
   ("PROPER", "Central Park"),
   ("COMPOUND", "trail running")]
```

### Write Path: Adding a Memory

```
Input:
    text_in = "Alice mentioned she started trail running last month"
    m.add(text_in, user_id="alice")

Step 1: LLM EXTRACTION
    Send to LLM: "Extract distinct factual memories from this text"
    + Include top-10 existing similar memories (for dedup context)
    LLM returns: ["Alice started trail running last month"]
    — only ADD events; no UPDATE/DELETE.

Step 2: HASH DEDUP
    fact = "Alice started trail running last month"
    MD5(fact) = "7f3a2b..."
    Search main collection for this hash → not found → proceed
    — if duplicate hash found, skip insertion.

Step 3: EMBED
    embedding = embed(fact)
    → float[1536]

Step 4: STORE IN MAIN COLLECTION
    Insert {
        id: "mem_0156",
        document: fact,
        embedding: [0.023, -0.189, ...],
        metadata: {
            user_id: "alice",
            hash: "7f3a2b...",
            created_at: 1715234567,
            text_lemmatized: "alice start trail run last month"
        }
    }

Step 5: ENTITY EXTRACTION (spaCy, no LLM call)
    extract_entities(fact)
    → [("PROPER", "Alice"), ("COMPOUND", "trail running")]

Step 6: ENTITY LINKING
    For entity "Alice":
        a. Embed "Alice" → float[1536]
        b. Search entity collection for nearest match (threshold >= 0.95)
        c. Found existing entity "Alice" (score=0.99)
        d. Append "mem_0156" to its linked_memory_ids list:
           linked_memory_ids: ["mem_0042", "mem_0078"] → ["mem_0042", "mem_0078", "mem_0156"]

    For entity "trail running":
        a. Embed "trail running" → float[1536]
        b. Search entity collection (threshold >= 0.95)
        c. No match found → create new entity entry:
           {
               id: "entity_uuid_new",
               document: "trail running",
               embedding: embed("trail running"),
               metadata: {
                   data: "trail running",
                   entity_type: "COMPOUND",
                   linked_memory_ids: ["mem_0156"],
                   user_id: "alice"
               }
           }
```

### Read Path: Searching

```
Input:
    query = "What outdoor activities does Alice enjoy?"
    filt = {"user_id": "alice"}
    m.search(query, filters=filt, top_k=10)

Step 1: PREPROCESS QUERY
    a. Lemmatize for BM25:
       "What outdoor activities does Alice enjoy?"
       → "outdoor activity alice enjoy"  — remove stopwords; reduce to stems.

    b. Extract entities from query (spaCy):
       → [("PROPER", "Alice")]

Step 2: EMBED QUERY
    query_vec = embed(query)
    → float[1536]

Step 3: SEMANTIC SEARCH (vector nearest-neighbor)
    Search main collection: find top-20 nearest by cosine(query_vec, entry.embedding)
    Filter by metadata: user_id == "alice"

    Results — the CANDIDATE SET; nothing else can enter:
        mem_0042: "Alice prefers hiking on weekends"           cosine=0.82
        mem_0156: "Alice started trail running last month"     cosine=0.79
        mem_0067: "The team went mountain biking on Saturday"  cosine=0.71
        mem_0031: "Alice enjoys cooking Italian food"          cosine=0.65
        ... (up to 20 candidates)

Step 4: BM25 KEYWORD BOOST
    Search main collection with BM25 using lemmatized query "outdoor activity alice enjoy"
    Only score entries that are already in the candidate set.

    BM25 raw scores (before normalization):
        mem_0042: matches "alice" → raw=2.1 → normalized=0.31
        mem_0156: matches "alice" → raw=2.1 → normalized=0.31
        mem_0067: no match for key terms → normalized=0.0
        mem_0031: matches "alice" → raw=2.1 → normalized=0.31

    Normalization uses a sigmoid: x = steepness * (raw - midpoint); normalized = 1 / (1 + exp(-x))

Step 5: ENTITY BOOST
    Query entities: ["Alice"]

    a. Embed "Alice" → float[1536]
    b. Search entity collection for "Alice" (threshold >= 0.5)
    c. Found entity "Alice" with linked_memory_ids = ["mem_0042", "mem_0078", "mem_0156"]
    d. Compute boost for each linked memory:

       similarity = cosine(embed("Alice"), entity_store_match.embedding) = 0.99
       num_linked = 3
       memory_count_weight = 1.0 / (1.0 + 0.001 * (2^2)) = 0.996
       boost = 0.99 * ENTITY_BOOST_WEIGHT * 0.996

       — ENTITY_BOOST_WEIGHT is a constant, likely ~0.5 based on max boost = 0.5
       boost ≈ 0.49

       mem_0042: entity_boost = 0.49
       mem_0078: entity_boost = 0.49 — omitted; not in candidate set
       mem_0156: entity_boost = 0.49

    Note: if an entity links to MANY memories (e.g., 1000), the boost is
    attenuated by memory_count_weight to prevent common entities from
    dominating. The formula: 1/(1 + 0.001*(N-1)^2) drops sharply above ~30 links.

Step 6: SCORE FUSION
    final_score(mem) = semantic_score + bm25_boost + entity_boost

    mem_0042: 0.82 + 0.31 + 0.49 = 1.62 → normalize to [0,1] → ~0.95
    mem_0156: 0.79 + 0.31 + 0.49 = 1.59 → ~0.93
    mem_0031: 0.65 + 0.31 + 0.00 = 0.96 → ~0.56
    mem_0067: 0.71 + 0.00 + 0.00 = 0.71 → ~0.41

Step 7: FILTER BY THRESHOLD (default 0.1) AND RETURN TOP-K
    All above threshold. Sort by final score. Return top-10.

    Result: [mem_0042, mem_0156, mem_0031, mem_0067, ...]
```

### Key Design Decisions in Mem0 v3

1. **ADD-only**: Never modify or delete stored memories. If "Alice lives in
   Austin" is superseded by "Alice moved to Denver", both are stored. The newer
   one will rank higher at query time because it's more relevant to recent
   queries, and Mem0's LLM extraction avoids exact duplicates by checking
   existing memories during extraction.

2. **Entity resolution at 0.95 threshold**: When storing entity "Alice Chen",
   if an existing entity "Alice" has cosine >= 0.95 to "Alice Chen", they're
   treated as the same entity. Below 0.95 → new entity created.

3. **BM25 is boost-only**: It cannot add new candidates. Only semantic search
   determines the candidate set. This prevents keyword spam from surfacing
   irrelevant memories.

4. **Graceful degradation**: Without spaCy → no entity boost, no BM25
   lemmatization, falls back to semantic-only. Without fastembed (Qdrant-specific)
   → no BM25. The system always works, just with fewer signals.

---

## A-Mem: Complete Architecture

### Data Structure

A-Mem uses a **single collection**. Each memory note has a richer structure:

```
SINGLE COLLECTION
┌───────────────────────────────────────────────────────────────────┐
│ Note                                                              │
│   id:         "note_042"                                          │
│   document:   "Alice prefers hiking on weekends"                  │
│   embedding:  float[384] of embed(content+keywords+tags+context)  │
│   metadata:                                                       │
│     timestamp:  1715234567                                        │
│     keywords:   "hiking, outdoors, weekend, exercise, preference" │
│     tags:       "hobby, physical-activity, Alice"                 │
│     context:    "User Alice has a strong preference for outdoor   │
│                  physical activities, specifically hiking,         │
│                  suggesting she values nature and exercise.        │
│                  This aligns with her interest in trail running."  │
│     links:      ["note_078", "note_156"]                          │
└───────────────────────────────────────────────────────────────────┘
```

### Critical Difference: What Gets Embedded

In Mem0, only the fact sentence is embedded:
```
fact = "Alice prefers hiking on weekends"
embed(fact) → float[1536]
```

In A-Mem, the embedding covers ALL textual fields concatenated:
```
embed_input = "Alice prefers hiking on weekends " +
              "hiking, outdoors, weekend, exercise, preference " +
              "hobby, physical-activity, Alice " +
              "User Alice has a strong preference for outdoor physical " +
              "activities, specifically hiking, suggesting she values " +
              "nature and exercise. This aligns with her interest in " +
              "trail running."

embed(embed_input) → float[384]
```

This means the embedding "knows about" concepts not explicitly in the original
text. A query about "outdoor exercise" would match this note even if the
original content only said "hiking" — because the context description explicitly
mentions "outdoor physical activities" and "exercise."

The embedding model used is `all-minilm-l6-v2` — 384 dimensions, small and fast.

### Write Path: Adding a Memory

```
Input: Agent receives message "Alice said she started trail running"

Step 1: NOTE CONSTRUCTION (one LLM call)
    Prompt the LLM:
        "Given this interaction content, generate:
         - keywords: key concepts (3-8 words)
         - tags: categories for organization
         - context: rich contextual description connecting to broader meaning"

    Content: "Alice said she started trail running"

    LLM returns:
        keywords: "trail running, exercise, outdoors, new hobby, fitness, Alice"
        tags: "physical-activity, lifestyle-change, Alice, recreation"
        context: "User Alice recently took up trail running as a new fitness
                  hobby, indicating growing interest in outdoor endurance
                  activities and possibly an evolution from casual hiking to
                  more intense exercise routines."

Step 2: COMPUTE EMBEDDING — no LLM; just the embedding model
    embed_text = content + " " + keywords + " " + tags + " " + context
    embedding = model.encode(embed_text) → float[384]

Step 3: LINK GENERATION
    a. Search collection for top-5 nearest to new embedding:
       note_042: "Alice prefers hiking on weekends"  score=0.87
       note_078: "Alice goes to gym Tuesdays"        score=0.72
       note_103: "Bob runs marathons"                score=0.68

    b. Ask LLM (one call):
       "Should note_156 be linked to any of these notes?
        Consider shared entities, themes, and conceptual connections.
        Return which notes to link and why."

       LLM returns: "Link to note_042 — both Alice + outdoor activity;
                     link note_078 — both Alice + fitness."

    c. Update links:
       note_156.links = ["note_042", "note_078"]
       note_042.links.append("note_156")  // bidirectional
       note_078.links.append("note_156")  // bidirectional

Step 4: MEMORY EVOLUTION (one LLM call per linked note)
    For each linked note, ask LLM:
        "Given new memory about Alice starting trail running, should this
         older memory's context/keywords/tags be updated?"

    note_042 (about hiking):
        Old context: "Alice enjoys hiking as a regular outdoor activity"
        LLM says: Update.
        New context: "Alice enjoys outdoor physical activities including hiking
                      and more recently trail running, suggesting an evolution
                      toward higher-intensity exercise"
        New keywords: "hiking, outdoors, exercise, trail running, Alice, nature"

    → Re-embed note_042 with updated text:
      rebuilt = concat of new content + new keywords + tags + new context
      note_042.embedding = embed(rebuilt)

    note_078 (about gym):
        Old context: "Alice maintains fitness through regular gym visits"
        LLM says: Update.
        New context: "Alice maintains fitness through both gym visits and outdoor
                      running activities like trail running"

    → Re-embed note_078 similarly.
```

### Read Path: Searching

```
Input: query = "What outdoor activities does Alice enjoy?"

Step 1: EMBED QUERY
    query_vec = model.encode(query)
    → float[384]

Step 2: COSINE SEARCH
    Find top-k nearest notes in collection by cosine(query_vec, note.embedding)

    Results:
        note_042: score=0.91
            — matches well: CONTEXT now cites outdoor physical activities including
             hiking and trail running; this wording was evolved when note_156 arrived
        note_156: score=0.85
            — keywords include outdoors and exercise
        note_089: score=0.62
            — Bob's camping: matches outdoor but wrong person

Step 3: LINK EXPANSION (one-hop traversal)
    For each result, follow links to pull in connected notes:
        note_042.links = ["note_156", "note_031"]
            → add note_031 to results (wasn't found by direct search)
        note_156.links = ["note_042", "note_078"]
            → add note_078 — about gym; tangentially related

    Final result set (deduplicated):
        [note_042, note_156, note_078, note_031, note_089]

Step 4: RETURN
    Provide all these notes as context to the agent.
```

### Key Design Decisions in A-Mem

1. **Rich embeddings**: By embedding content+keywords+tags+context together,
   each note's vector captures far more meaning than the raw text alone. The
   LLM-generated context acts as a "semantic expansion" of the original content.

2. **Memory evolution**: Old notes are NOT static. When new related information
   arrives, old notes' context descriptions and keywords get UPDATED and
   RE-EMBEDDED. This means old notes become easier to find over time as their
   descriptions grow richer.

3. **No separate entity collection**: Entity awareness is baked into the
   keywords and tags. If a note is about "Alice", the keyword "Alice" is in
   the embedding text. A query mentioning "Alice" will match partly because
   of keyword overlap in the embedding space.

4. **Link traversal instead of multi-signal**: Rather than boosting by entity
   matching (Mem0's approach), A-Mem trusts that linked notes are relevant and
   pulls them in unconditionally. This is simpler but less precise.

5. **Works with tiny models**: Tested on Qwen 1.5B and Llama 1B for the LLM
   calls — note construction, link generation, evolution. The embedding model
   is all-minilm-l6-v2 (22M params, runs on CPU).

---

## Side-by-Side Comparison

| Aspect | Mem0 v3 | A-Mem |
|--------|---------|-------|
| Collections | 2 (main + entities) | 1 |
| What is embedded | Fact sentence only | content + keywords + tags + context |
| Embedding model | text-embedding-3-small (1536d) | all-minilm-l6-v2 (384d) |
| Entity handling | Separate collection, spaCy NER, score boost | Implicit in keywords/tags |
| Write-time LLM calls | 1 (extraction) | 3+ — construction, link, evolution per linked note |
| Read-time LLM calls | 0 | 0 |
| Retrieval mechanism | Semantic + BM25 + entity boost | Semantic + link traversal |
| Contradiction handling | Accumulate, rank by relevance | Evolve old memory descriptions |
| Works with small LLMs? | Extraction needs decent LLM | Tested on Qwen 1.5B |
| Graph DB required? | No | No |
| Memory mutability | Immutable once stored | Context/keywords/embedding mutated on evolution |

---

## Implications for Rhapsode

Current Rhapsode node stored in ChromaDB:
```python
{
    "id": "node_7",
    "document": "The merchant has a hidden scar on his left hand",  # embedded
    "metadata": {
        "type": "secret",
        "entities": "merchant",       # NOT embedded, only filterable
        "known_by": "player",         # NOT embedded, only filterable
        "resolved_at": -1
    }
}
```

### Option A: Mem0-style (add entity collection)

Add a second ChromaDB collection `nodes_entities`:
```python
{
    "id": "entity_merchant",
    "document": "merchant",
    "embedding": embed("merchant"),
    "metadata": {
        "linked_node_ids": ["node_7", "node_12", "node_23", "node_41"]
    }
}
```

At query time, extract character names from the Director's context, look them up
in the entity collection, boost linked nodes.

Cost: One extra ChromaDB collection. No LLM calls. spaCy or simple regex for
entity extraction — our entities are already known; they're game characters.

### Option B: A-Mem-style (richer embeddings)

Change what gets embedded:
```python
embed_text = f"{fact}. Characters: {', '.join(entities)}. Type: {type}. Known by: {', '.join(known_by)}."
# "The merchant has a hidden scar on his left hand. Characters: merchant.
#  Type: secret. Known by: player."
```

Cost: Zero infrastructure change. Just modify the string passed to
`model.encode()` during storage. Retrieval automatically benefits.

### Option C: Both

Use richer embeddings (Option B) AND maintain an entity index (Option A) for
explicit entity-based retrieval when the Director context mentions a character.

This is the belt-and-suspenders approach: rich embeddings make general queries
better, entity index makes character-specific queries precise.
