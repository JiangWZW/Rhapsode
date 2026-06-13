"""
Chain Edge Similarity Test — evaluates whether the embedding similarity gate
would kill important chain edges.

For each of the 43 chain edges, computes cosine similarity between connected
facts using BAAI/bge-base-en-v1.5 (same as production).

Output: which edges survive vs. get killed at various thresholds.
"""

import os
import sys
import json
import numpy as np
from sentence_transformers import SentenceTransformer

MODEL_NAME = "BAAI/bge-base-en-v1.5"

FACTS = [
    {"id": 1, "fact": "Harren's army approaches from east",
     "entities": ["Lord Harren", "Ashenmoor"], "turn": 0},
    {"id": 2, "fact": "Voss sealed the eastern gate",
     "entities": ["Warden Elara Voss", "Ashenmoor"], "turn": 0},
    {"id": 3, "fact": "Thornfield refugees stranded",
     "entities": ["Thornfield"], "turn": 0},
    {"id": 4, "fact": "Aldric knows Harren seeks relic",
     "entities": ["Father Aldric", "Lord Harren", "Ashenmoor"], "turn": 0},
    {"id": 5, "fact": "Maren conceals festering wound",
     "entities": ["Sergeant Maren"], "turn": 0},
    {"id": 6, "fact": "Voss has Duke's secret orders",
     "entities": ["Warden Elara Voss", "Duke"], "turn": 0},
    {"id": 7, "fact": "Player confronted Voss about gate",
     "entities": ["Player", "Warden Elara Voss", "Ashenmoor"], "turn": 1},
    {"id": 8, "fact": "Maren's wound: blackrot infection",
     "entities": ["Sergeant Maren"], "turn": 1},
    {"id": 9, "fact": "Harren's scouts near western ridge",
     "entities": ["Lord Harren"], "turn": 2},
    {"id": 10, "fact": "Aldric visits catacombs at night",
     "entities": ["Father Aldric", "Ashenmoor"], "turn": 2},
    {"id": 11, "fact": "Duke expects Ashenmoor to fall",
     "entities": ["Duke", "Ashenmoor"], "turn": 3},
    {"id": 12, "fact": "Voss ordered water rationing",
     "entities": ["Warden Elara Voss", "Ashenmoor"], "turn": 4},
    {"id": 13, "fact": "Tunnel beneath east wall passable",
     "entities": ["Ashenmoor"], "turn": 4},
    {"id": 14, "fact": "Maren collapsed on night watch",
     "entities": ["Sergeant Maren"], "turn": 5},
    {"id": 15, "fact": "Harren's war drums from battlements",
     "entities": ["Lord Harren", "Ashenmoor"], "turn": 5},
    {"id": 16, "fact": "Player offers sortie for refugees",
     "entities": ["Player", "Thornfield"], "turn": 6},
    {"id": 17, "fact": "Voss admits garrison cannot hold",
     "entities": ["Warden Elara Voss"], "turn": 6},
    {"id": 18, "fact": "Aldric warns Player about relic",
     "entities": ["Father Aldric", "Player"], "turn": 7},
    {"id": 19, "fact": "Three soldiers deserted overnight",
     "entities": ["Ashenmoor"], "turn": 7},
    {"id": 20, "fact": "Player found Duke's letter in Voss's room",
     "entities": ["Player", "Duke", "Warden Elara Voss"], "turn": 8},
    {"id": 21, "fact": "Harren's vanguard at eastern tree line",
     "entities": ["Lord Harren", "Ashenmoor"], "turn": 8},
    {"id": 22, "fact": "Maren asks Player to lead if she falls",
     "entities": ["Sergeant Maren", "Player"], "turn": 9},
    {"id": 23, "fact": "Voss suspects Aldric hiding something",
     "entities": ["Warden Elara Voss", "Father Aldric"], "turn": 9},
    {"id": 24, "fact": "Relic: bone fragment grants visions",
     "entities": ["Father Aldric", "Ashenmoor"], "turn": 10},
    {"id": 25, "fact": "Thornfield survivors say Harren showed mercy",
     "entities": ["Thornfield", "Lord Harren"], "turn": 10},
    {"id": 26, "fact": "Player convinced Maren to accept treatment",
     "entities": ["Player", "Sergeant Maren", "Father Aldric"], "turn": 11},
    {"id": 27, "fact": "Voss placed guards on catacombs",
     "entities": ["Warden Elara Voss", "Ashenmoor"], "turn": 11},
    {"id": 28, "fact": "Raven: Harren's surrender terms arrive",
     "entities": ["Lord Harren", "Ashenmoor"], "turn": 12},
]

FACT_BY_ID = {f["id"]: f for f in FACTS}


def build_chain_edges():
    """Reproduce chain topology — same logic as visualizer."""
    edges = []
    existing = []
    for node in FACTS:
        most_recent = {}
        for prior in existing:
            for e in prior["entities"]:
                most_recent[e.lower()] = prior["id"]

        connected = set()
        for e in node["entities"]:
            target = most_recent.get(e.lower())
            if target and target not in connected:
                edges.append((target, node["id"], e.lower()))
                connected.add(target)
        existing.append(node)
    return edges


def main():
    print("=" * 70)
    print("CHAIN EDGE SIMILARITY ANALYSIS")
    print("Model: BAAI/bge-base-en-v1.5 (production)")
    print("=" * 70)

    print("\nLoading model...")
    model = SentenceTransformer(MODEL_NAME)

    # Embed all facts
    texts = [f["fact"] for f in FACTS]
    embeddings = model.encode(texts, normalize_embeddings=True)
    emb_by_id = {f["id"]: embeddings[i] for i, f in enumerate(FACTS)}

    # Build chain edges
    chain_edges = build_chain_edges()
    print(f"\nChain edges: {len(chain_edges)}")

    # Compute similarity for each chain edge
    results = []
    for (from_id, to_id, entity) in chain_edges:
        sim = float(np.dot(emb_by_id[from_id], emb_by_id[to_id]))
        from_fact = FACT_BY_ID[from_id]["fact"]
        to_fact = FACT_BY_ID[to_id]["fact"]
        results.append({
            "from_id": from_id,
            "to_id": to_id,
            "entity": entity,
            "similarity": sim,
            "from_fact": from_fact,
            "to_fact": to_fact,
        })

    results.sort(key=lambda x: x["similarity"])

    # Print all edges sorted by similarity
    print(f"\n{'='*70}")
    print("ALL CHAIN EDGES (sorted by similarity, lowest first)")
    print(f"{'='*70}")
    print(f"{'Sim':>6} {'From':>4}->{'To':<4} {'Entity':<22} {'From fact':<40} {'To fact'}")
    print("-" * 140)
    for r in results:
        marker = ""
        if r["similarity"] < 0.35:
            marker = " *** KILLED at 0.35"
        elif r["similarity"] < 0.45:
            marker = " ** KILLED at 0.45"
        elif r["similarity"] < 0.55:
            marker = " * borderline"
        print(f"{r['similarity']:>6.3f} {r['from_id']:>4}->{r['to_id']:<4} "
              f"{r['entity']:<22} "
              f"{r['from_fact'][:38]:<40} {r['to_fact'][:38]}{marker}")

    # Threshold analysis
    print(f"\n{'='*70}")
    print("THRESHOLD SWEEP — how many chain edges survive?")
    print(f"{'='*70}")
    thresholds = [0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60]
    for t in thresholds:
        killed = [r for r in results if r["similarity"] < t]
        survived = len(results) - len(killed)
        pct = 100 * survived / len(results)
        print(f"  theta={t:.2f}: {survived}/{len(results)} survive ({pct:.0f}%), "
              f"{len(killed)} killed")

    # Detailed killed-edge analysis at theta=0.45
    print(f"\n{'='*70}")
    print("EDGES KILLED AT theta=0.45 — detailed analysis")
    print(f"{'='*70}")
    killed_045 = [r for r in results if r["similarity"] < 0.45]
    if not killed_045:
        print("  (none killed)")
    else:
        for r in killed_045:
            print(f"\n  [{r['from_id']}->{r['to_id']}] sim={r['similarity']:.3f} "
                  f"via entity '{r['entity']}'")
            print(f"    FROM: \"{r['from_fact']}\"")
            print(f"    TO:   \"{r['to_fact']}\"")
            # Assess importance
            from_node = FACT_BY_ID[r["from_id"]]
            to_node = FACT_BY_ID[r["to_id"]]
            shared_entities = set(e.lower() for e in from_node["entities"]) & \
                              set(e.lower() for e in to_node["entities"])
            other_entities_from = [e for e in from_node["entities"]
                                   if e.lower() != r["entity"]]
            other_entities_to = [e for e in to_node["entities"]
                                 if e.lower() != r["entity"]]
            print(f"    Shared entities: {shared_entities}")
            print(f"    From also has: {other_entities_from}")
            print(f"    To also has: {other_entities_to}")

    # Distribution statistics
    sims = [r["similarity"] for r in results]
    print(f"\n{'='*70}")
    print("DISTRIBUTION STATISTICS")
    print(f"{'='*70}")
    print(f"  Mean similarity:   {np.mean(sims):.3f}")
    print(f"  Median similarity: {np.median(sims):.3f}")
    print(f"  Std dev:           {np.std(sims):.3f}")
    print(f"  Min:               {np.min(sims):.3f}")
    print(f"  Max:               {np.max(sims):.3f}")
    print(f"  Q1 (25th pctile):  {np.percentile(sims, 25):.3f}")
    print(f"  Q3 (75th pctile):  {np.percentile(sims, 75):.3f}")

    # Entity-level analysis: which entities have the weakest chain links?
    print(f"\n{'='*70}")
    print("PER-ENTITY CHAIN LINK QUALITY")
    print(f"{'='*70}")
    entity_sims = {}
    for r in results:
        entity_sims.setdefault(r["entity"], []).append(r["similarity"])
    print(f"  {'Entity':<25} {'Count':<6} {'Mean':>6} {'Min':>6} {'Max':>6}")
    print(f"  {'-'*55}")
    for entity in sorted(entity_sims, key=lambda e: np.mean(entity_sims[e])):
        vals = entity_sims[entity]
        print(f"  {entity:<25} {len(vals):<6} {np.mean(vals):>6.3f} "
              f"{np.min(vals):>6.3f} {np.max(vals):>6.3f}")

    # Critical question: would killing edges break reachability?
    print(f"\n{'='*70}")
    print("REACHABILITY IMPACT ANALYSIS (theta=0.45)")
    print(f"{'='*70}")
    surviving_edges = [(r["from_id"], r["to_id"])
                       for r in results if r["similarity"] >= 0.45]
    # Build adjacency (undirected for reachability)
    adj = {}
    for (a, b) in surviving_edges:
        adj.setdefault(a, set()).add(b)
        adj.setdefault(b, set()).add(a)
    # Check which nodes become isolated or lose connectivity
    all_ids = set(f["id"] for f in FACTS)
    connected_ids = set(adj.keys())
    isolated = all_ids - connected_ids
    print(f"  Nodes that become FULLY ISOLATED: {sorted(isolated) if isolated else 'none'}")

    # BFS to find connected components
    visited = set()
    components = []
    for node_id in sorted(all_ids):
        if node_id in visited:
            continue
        component = set()
        queue = [node_id]
        while queue:
            current = queue.pop(0)
            if current in visited:
                continue
            visited.add(current)
            component.add(current)
            for neighbor in adj.get(current, set()):
                if neighbor not in visited:
                    queue.append(neighbor)
        components.append(sorted(component))

    print(f"  Connected components: {len(components)}")
    for i, comp in enumerate(components):
        facts_in_comp = [FACT_BY_ID[nid]["fact"][:50] for nid in comp[:5]]
        print(f"    Component {i+1} ({len(comp)} nodes): {comp[:8]}...")
        for f in facts_in_comp:
            print(f"      - {f}")


if __name__ == "__main__":
    main()
