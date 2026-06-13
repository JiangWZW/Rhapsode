"""Reproduce ChromaDB HNSW error under conditions closer to the real app.

Test 1: Basic cross-thread (Python threading) -- already shown to NOT reproduce
Test 2: Interleaved query+upsert from background (matches reflect() pattern)
Test 3: With metadata `where` filters (matches reflect dedup pattern)
Test 4: Through the actual C++ CharacterMemory.reflect() via pybind11
Test 5: Volume stress -- many small upserts triggering HNSW resize
"""

import json
import math
import os
import sys
import tempfile
import threading
import traceback

import chromadb

print(f"chromadb {chromadb.__version__}, Python {sys.version.split()[0]}")

DIM = 768
def emb(seed: float):
    return [math.sin(seed + i * 0.01) for i in range(DIM)]

# Use a unique temp dir per run (don't try to delete while client holds files)
TEST_DIR = os.path.join(tempfile.gettempdir(), "rhapsode_chroma_test2")
os.makedirs(TEST_DIR, exist_ok=True)

# --------------------------------------------------------------------------
# Test 2: Interleaved query + upsert from background thread
# --------------------------------------------------------------------------
print("\n=== Test 2: Interleaved query+upsert from background ===")
client = chromadb.PersistentClient(path=os.path.join(TEST_DIR, "t2"))

cache = {}
def col(name="test-two"):
    if name not in cache:
        cache[name] = client.get_or_create_collection(name, metadata={"hnsw:space": "cosine"})
    return cache[name]

# Seed
for i in range(10):
    col().upsert(ids=[f"s{i}"], documents=[f"seed {i}"], embeddings=[emb(float(i))], metadatas=[{"type": 0, "turn": 0}])

# Main thread primes reader
r = col().query(query_embeddings=[emb(0.5)], n_results=3, include=["documents"])
print(f"  primed: {len(r['ids'][0])} results")

def reflect_sim():
    """Simulate reflect(): query for evidence, upsert insights, query for dedup, upsert more."""
    try:
        # Step 1: retrieve evidence (like retrieve())
        r = col().query(query_embeddings=[emb(5.0)], n_results=5, include=["documents", "metadatas", "distances"])
        # Step 2: upsert new insight
        col().upsert(ids=["insight_1"], documents=["insight about character"], embeddings=[emb(50.0)], metadatas=[{"type": 1, "turn": 1}])
        # Step 3: dedup query with where filter
        r2 = col().query(query_embeddings=[emb(50.0)], n_results=3, where={"type": 1}, include=["documents", "distances"])
        # Step 4: upsert another insight
        col().upsert(ids=["insight_2"], documents=["another insight"], embeddings=[emb(51.0)], metadatas=[{"type": 1, "turn": 1}])
        # Step 5: one more evidence query
        r3 = col().query(query_embeddings=[emb(6.0)], n_results=5, include=["documents"])
        # Step 6: upsert more insights
        for j in range(4):
            col().upsert(ids=[f"insight_{j+3}"], documents=[f"insight {j+3}"], embeddings=[emb(52.0 + j)], metadatas=[{"type": 1, "turn": 1}])
        print(f"  [bg] reflect done, count={col().count()}")
    except Exception as e:
        print(f"  [bg] ERROR: {e}")

t = threading.Thread(target=reflect_sim)
t.start()
t.join()

try:
    r = col().query(query_embeddings=[emb(1.0)], n_results=5, include=["documents"])
    print(f"  main query after reflect: {len(r['ids'][0])} results -- OK")
except Exception as e:
    print(f"  main query FAILED: {e}")

# --------------------------------------------------------------------------
# Test 3: With metadata where filters (closer to real dedup pattern)
# --------------------------------------------------------------------------
print("\n=== Test 3: where-filter queries after bg upserts ===")
cache.clear()
client3 = chromadb.PersistentClient(path=os.path.join(TEST_DIR, "t3"))
cache3 = {}
def col3(name="test-three"):
    if name not in cache3:
        cache3[name] = client3.get_or_create_collection(name, metadata={"hnsw:space": "cosine"})
    return cache3[name]

for i in range(15):
    col3().upsert(ids=[f"s{i}"], documents=[f"seed {i}"], embeddings=[emb(float(i))],
                  metadatas=[{"type": 0 if i < 10 else 1, "turn": 0, "weight": float(i)}])

r = col3().query(query_embeddings=[emb(0.5)], n_results=5, include=["documents"])
print(f"  primed: {len(r['ids'][0])} results")

def bg_with_where():
    try:
        col3().query(query_embeddings=[emb(5.0)], n_results=5, include=["documents", "metadatas", "distances"])
        for j in range(6):
            col3().upsert(ids=[f"bg_{j}"], documents=[f"bg insight {j}"], embeddings=[emb(100.0 + j)],
                          metadatas=[{"type": 1, "turn": 1, "weight": 8.0}])
        # dedup query with type filter
        col3().query(query_embeddings=[emb(100.0)], n_results=3, where={"type": 1},
                     include=["documents", "distances"])
        print(f"  [bg] done, count={col3().count()}")
    except Exception as e:
        print(f"  [bg] ERROR: {e}")

t = threading.Thread(target=bg_with_where)
t.start()
t.join()

try:
    r = col3().query(query_embeddings=[emb(1.0)], n_results=5, include=["documents"])
    print(f"  main query: {len(r['ids'][0])} results -- OK")
except Exception as e:
    print(f"  main query FAILED: {e}")

try:
    r = col3().query(query_embeddings=[emb(1.0)], n_results=3, where={"type": 1},
                     include=["documents"])
    print(f"  main query (where): {len(r['ids'][0])} results -- OK")
except Exception as e:
    print(f"  main query (where) FAILED: {e}")

# --------------------------------------------------------------------------
# Test 4: Through actual C++ CharacterMemory (pybind11 + std::async)
# --------------------------------------------------------------------------
print("\n=== Test 4: Through C++ CharacterMemory + pybind11 ===")
try:
    sys.path.insert(0, os.path.dirname(__file__))
    from rhapsode._core import CharacterMemory as CppCharMem
    from rhapsode.memory import register_character_memory_callbacks, warmup_model
    from rhapsode.validator import make_local_llm_callback

    warmup_model()

    chroma_path = os.path.join(TEST_DIR, "t4")
    cm = CppCharMem("TestChar")
    for i in range(8):
        cm.seed_from_graph(f"Test fact {i} about the world and events", i)

    register_character_memory_callbacks(cm, chroma_path=chroma_path)
    cm.set_reflection_llm_callback(make_local_llm_callback())
    cm.sync_to_chroma()

    # Prime: main thread query
    r1 = cm.retrieve("what happened recently", 5)
    print(f"  primed retrieve: {len(r1)} chars")

    # Simulate observe+speak to drain countdown
    for i in range(8):
        cm.observe(f"A dramatic event {i} happened that changed everything profoundly", i)
        cm.speak(f"TestChar said something important about event {i} with emotional weight", i)

    print(f"  needs_reflection: {cm.needs_reflection()}")

    if cm.needs_reflection():
        # Run reflect in a background thread via concurrent.futures (like std::async)
        import concurrent.futures
        with concurrent.futures.ThreadPoolExecutor(max_workers=1) as pool:
            future = pool.submit(cm.reflect)
            future.result()  # join
        print("  reflect done (from thread pool)")

        # Now query from main thread
        try:
            r2 = cm.retrieve("what does TestChar feel", 5)
            print(f"  main retrieve after reflect: {len(r2)} chars -- OK")
        except Exception as e:
            print(f"  main retrieve FAILED: {e}")
            traceback.print_exc()

        # Try briefing too
        try:
            b = cm.briefing("recent events", 5)
            print(f"  main briefing after reflect: {len(b)} chars -- OK")
        except Exception as e:
            print(f"  main briefing FAILED: {e}")
            traceback.print_exc()
    else:
        print("  countdown not drained, skipping reflect test")
        print("  (need more observe/speak calls to drain it)")

except ImportError as e:
    print(f"  SKIPPED (can't import C++ module): {e}")
except Exception as e:
    print(f"  ERROR: {e}")
    traceback.print_exc()

# --------------------------------------------------------------------------
# Test 5: Volume stress -- lots of small upserts to trigger HNSW resize
# --------------------------------------------------------------------------
print("\n=== Test 5: Volume stress (HNSW resize trigger) ===")
client5 = chromadb.PersistentClient(path=os.path.join(TEST_DIR, "t5"))
col5 = client5.get_or_create_collection("stress", metadata={"hnsw:space": "cosine"})

for i in range(50):
    col5.upsert(ids=[f"s{i}"], documents=[f"doc {i}"], embeddings=[emb(float(i))], metadatas=[{"turn": 0}])

r = col5.query(query_embeddings=[emb(0.5)], n_results=5, include=["documents"])
print(f"  primed with 50 docs: {len(r['ids'][0])} results")

results_log = []
for turn in range(1, 11):
    # bg: many small upserts + queries (like aggressive reflect)
    def stress_bg(t=turn):
        try:
            for j in range(20):
                col5.upsert(ids=[f"bg_{t}_{j}"], documents=[f"bg {t}-{j}"], embeddings=[emb(float(t * 100 + j))],
                            metadatas=[{"turn": t, "type": 1}])
                if j % 5 == 0:
                    col5.query(query_embeddings=[emb(float(t * 100 + j))], n_results=3, include=["documents"])
        except Exception as e:
            print(f"  [bg turn {t}] ERROR: {e}")

    bt = threading.Thread(target=stress_bg)
    bt.start()
    bt.join()

    try:
        r = col5.query(query_embeddings=[emb(float(turn))], n_results=5, include=["documents"])
        results_log.append(f"Turn {turn}: OK ({col5.count()} docs)")
    except Exception as e:
        results_log.append(f"Turn {turn}: FAILED -- {e}")

for line in results_log:
    print(f"  {line}")

print(f"\nTest artifacts in: {TEST_DIR}")
print("Done.")
