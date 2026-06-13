"""
Test: Does embedding cosine similarity predict meaningful narrative connections?

We take the siege scenario's fact graph (seed + synthetic turn-generated nodes)
and compute pairwise similarity. Then we compare against a hand-labeled ground
truth of which pairs SHOULD be connected vs which are spurious entity overlaps.

Model: BAAI/bge-base-en-v1.5 (same as production)
"""

import json
import numpy as np
from sentence_transformers import SentenceTransformer

MODEL_NAME = "BAAI/bge-base-en-v1.5"

# All facts from the siege scenario — 6 seeds + 22 synthetic nodes matching
# the 28-node graph observed in actual gameplay sessions.
FACTS = [
    # --- Seed nodes (from siege.json) ---
    {"id": 1, "fact": "Lord Harren's army approaches Ashenmoor from the east.",
     "entities": ["Lord Harren", "Ashenmoor"]},
    {"id": 2, "fact": "Warden Voss sealed the eastern gate.",
     "entities": ["Warden Elara Voss", "Ashenmoor"]},
    {"id": 3, "fact": "Thornfield refugees are stranded on the eastern road.",
     "entities": ["Thornfield"]},
    {"id": 4, "fact": "Father Aldric knows Lord Harren seeks a relic hidden beneath Ashenmoor.",
     "entities": ["Father Aldric", "Lord Harren", "Ashenmoor"]},
    {"id": 5, "fact": "Sergeant Maren conceals a festering wound.",
     "entities": ["Sergeant Maren"]},
    {"id": 6, "fact": "Warden Voss has secret orders from the Duke.",
     "entities": ["Warden Elara Voss", "Duke"]},

    # --- Synthetic turn-generated nodes (representative of observed gameplay) ---
    {"id": 7, "fact": "The Player confronted Warden Voss about the sealed gate.",
     "entities": ["Player", "Warden Elara Voss", "Ashenmoor"]},
    {"id": 8, "fact": "Sergeant Maren's wound is infected with blackrot.",
     "entities": ["Sergeant Maren"]},
    {"id": 9, "fact": "Lord Harren's scouts were spotted near the western ridge.",
     "entities": ["Lord Harren"]},
    {"id": 10, "fact": "Father Aldric visits the catacombs at night alone.",
     "entities": ["Father Aldric", "Ashenmoor"]},
    {"id": 11, "fact": "The Duke expects Ashenmoor to fall within a week.",
     "entities": ["Duke", "Ashenmoor"]},
    {"id": 12, "fact": "Warden Voss ordered rationing of water supplies.",
     "entities": ["Warden Elara Voss", "Ashenmoor"]},
    {"id": 13, "fact": "A tunnel beneath the east wall may still be passable.",
     "entities": ["Ashenmoor"]},
    {"id": 14, "fact": "Sergeant Maren collapsed during the night watch.",
     "entities": ["Sergeant Maren"]},
    {"id": 15, "fact": "Lord Harren's war drums can be heard from the battlements.",
     "entities": ["Lord Harren", "Ashenmoor"]},
    {"id": 16, "fact": "The Player offered to lead a sortie to rescue the refugees.",
     "entities": ["Player", "Thornfield"]},
    {"id": 17, "fact": "Warden Voss privately admitted the garrison cannot hold.",
     "entities": ["Warden Elara Voss"]},
    {"id": 18, "fact": "Father Aldric warned the Player about the relic's danger.",
     "entities": ["Father Aldric", "Player"]},
    {"id": 19, "fact": "Three soldiers deserted overnight through the western postern.",
     "entities": ["Ashenmoor"]},
    {"id": 20, "fact": "The Player found the Duke's sealed letter in Voss's quarters.",
     "entities": ["Player", "Duke", "Warden Elara Voss"]},
    {"id": 21, "fact": "Harren's vanguard reached the eastern tree line at dusk.",
     "entities": ["Lord Harren", "Ashenmoor"]},
    {"id": 22, "fact": "Sergeant Maren asked the Player to lead if she falls.",
     "entities": ["Sergeant Maren", "Player"]},
    {"id": 23, "fact": "Warden Voss suspects Father Aldric is hiding something.",
     "entities": ["Warden Elara Voss", "Father Aldric"]},
    {"id": 24, "fact": "The relic is a bone fragment said to grant visions of the future.",
     "entities": ["Father Aldric", "Ashenmoor"]},
    {"id": 25, "fact": "Thornfield survivors report Harren's men showed them mercy.",
     "entities": ["Thornfield", "Lord Harren"]},
    {"id": 26, "fact": "The Player convinced Maren to let Aldric treat her wound.",
     "entities": ["Player", "Sergeant Maren", "Father Aldric"]},
    {"id": 27, "fact": "Warden Voss placed guards on the catacombs entrance.",
     "entities": ["Warden Elara Voss", "Ashenmoor"]},
    {"id": 28, "fact": "A raven arrived bearing Lord Harren's terms of surrender.",
     "entities": ["Lord Harren", "Ashenmoor"]},
]

# Ground truth: pairs that SHOULD be connected (strong narrative relationship)
# and pairs that share entities but SHOULD NOT be connected (spurious overlap).
SHOULD_CONNECT = [
    (1, 9),    # Harren approaches + Harren scouts spotted (same military advance)
    (1, 15),   # Harren approaches + war drums heard (progression of same event)
    (1, 21),   # Harren approaches + vanguard reached tree line (same advance)
    (1, 28),   # Harren approaches + surrender terms (consequence)
    (2, 7),    # Voss sealed gate + Player confronted Voss (direct cause-effect)
    (2, 3),    # Gate sealed + refugees stranded (direct cause-effect)
    (3, 16),   # Refugees stranded + Player offers rescue (consequence)
    (3, 25),   # Refugees + Thornfield survivors report (same population)
    (4, 10),   # Aldric knows relic + visits catacombs at night (elaboration)
    (4, 24),   # Aldric knows relic + relic description (elaboration)
    (4, 18),   # Aldric knows relic + warns Player about relic (consequence)
    (5, 8),    # Maren wound concealed + wound is blackrot (elaboration)
    (5, 14),   # Maren wound + Maren collapsed (consequence)
    (5, 22),   # Maren wound + asks Player to lead if she falls (consequence)
    (6, 11),   # Duke's orders + Duke expects fall (same political thread)
    (6, 20),   # Duke's orders + Player found the letter (discovery)
    (6, 17),   # Duke's orders + Voss admits cannot hold (consequence)
    (8, 14),   # Blackrot + collapsed (cause-effect)
    (8, 26),   # Blackrot + Player convinced Maren to get treatment (consequence)
    (9, 21),   # Scouts near ridge + vanguard reached tree line (progression)
    (10, 23),  # Aldric visits catacombs + Voss suspects Aldric (cause-effect)
    (10, 27),  # Aldric visits catacombs + Voss guards catacombs (consequence)
    (12, 19),  # Rationing + soldiers deserted (cause-effect)
    (17, 6),   # Voss admits cannot hold + secret orders to surrender (connection)
    (23, 27),  # Voss suspects Aldric + guards catacombs (cause-effect)
    (25, 28),  # Harren showed mercy + surrender terms (characterization)
]

# Pairs that share entities but should NOT have an edge (spurious).
SHOULD_NOT_CONNECT = [
    (1, 4),    # Harren army approaches + Aldric knows relic (share Harren+Ashenmoor, but unrelated plots)
    (2, 12),   # Voss sealed gate + Voss rationed water (share Voss+Ashenmoor, but different decisions)
    (2, 27),   # Voss sealed gate + Voss guards catacombs (different security measures)
    (5, 26),   # Maren conceals wound + Player convinced treatment (actually... this IS related)
    (1, 12),   # Harren approaches + water rationing (share Ashenmoor, different threads)
    (13, 15),  # Tunnel in east wall + war drums (share Ashenmoor, unrelated)
    (13, 19),  # Tunnel + soldiers deserted (share Ashenmoor, unrelated)
    (7, 12),   # Player confronted Voss + Voss rationed water (share Voss, different events)
    (11, 13),  # Duke expects fall + tunnel passable (share Ashenmoor, unrelated)
    (15, 19),  # War drums + soldiers deserted (share Ashenmoor — actually this could be causal!)
    (9, 25),   # Scouts spotted + survivors showed mercy (share Harren, weak connection)
    (12, 17),  # Rationing + cannot hold (share Voss — actually related to desperation)
    (10, 24),  # Visits catacombs + relic description (share Aldric+Ashenmoor — ACTUALLY RELATED)
    (19, 21),  # Soldiers deserted + vanguard arrived (share Ashenmoor, temporal coincidence only)
    (2, 11),   # Sealed gate + Duke expects fall (share Ashenmoor, different threads)
    (3, 13),   # Refugees stranded + tunnel passable (share nothing obvious, different subplots)
]


def main():
    print(f"Loading model: {MODEL_NAME}")
    model = SentenceTransformer(MODEL_NAME)

    facts_text = [f["fact"] for f in FACTS]
    print(f"Computing embeddings for {len(facts_text)} facts...")
    embeddings = model.encode(facts_text, normalize_embeddings=True)

    # Pairwise cosine similarity (normalized embeddings → dot product = cosine)
    sim_matrix = embeddings @ embeddings.T

    # Compute statistics for SHOULD_CONNECT pairs
    connect_sims = []
    for (a, b) in SHOULD_CONNECT:
        idx_a = a - 1
        idx_b = b - 1
        sim = float(sim_matrix[idx_a, idx_b])
        connect_sims.append(sim)

    # Compute statistics for SHOULD_NOT_CONNECT pairs
    no_connect_sims = []
    for (a, b) in SHOULD_NOT_CONNECT:
        idx_a = a - 1
        idx_b = b - 1
        sim = float(sim_matrix[idx_a, idx_b])
        no_connect_sims.append(sim)

    # Compute shares_entity-based pairs for reference
    entity_overlap_sims = []
    entity_overlap_pairs = []
    for i in range(len(FACTS)):
        for j in range(i+1, len(FACTS)):
            ent_i = {e.lower() for e in FACTS[i]["entities"]}
            ent_j = {e.lower() for e in FACTS[j]["entities"]}
            if ent_i & ent_j:
                sim = float(sim_matrix[i, j])
                entity_overlap_sims.append(sim)
                entity_overlap_pairs.append((FACTS[i]["id"], FACTS[j]["id"], sim))

    # All pairs (for baseline distribution)
    all_sims = []
    for i in range(len(FACTS)):
        for j in range(i+1, len(FACTS)):
            all_sims.append(float(sim_matrix[i, j]))

    print("\n" + "="*70)
    print("EMBEDDING SIMILARITY AS EDGE QUALITY SIGNAL -- RESULTS")
    print("="*70)

    print(f"\n{'Category':<35} {'Mean':>6} {'Median':>7} {'Min':>6} {'Max':>6} {'Std':>6}  N")
    print("-"*80)
    for label, data in [
        ("All pairs (baseline)", all_sims),
        ("Entity-overlap pairs (current)", entity_overlap_sims),
        ("SHOULD connect (ground truth +)", connect_sims),
        ("Should NOT connect (ground truth -)", no_connect_sims),
    ]:
        arr = np.array(data)
        print(f"{label:<35} {arr.mean():.3f}  {np.median(arr):.3f}  "
              f"{arr.min():.3f}  {arr.max():.3f}  {arr.std():.3f}  {len(arr)}")

    # Separation analysis
    print("\n" + "-"*70)
    print("SEPARATION ANALYSIS")
    print("-"*70)
    connect_mean = np.mean(connect_sims)
    no_connect_mean = np.mean(no_connect_sims)
    print(f"Mean similarity (should connect):     {connect_mean:.4f}")
    print(f"Mean similarity (should NOT connect): {no_connect_mean:.4f}")
    print(f"Separation (delta):                   {connect_mean - no_connect_mean:.4f}")

    # Threshold sweep
    print("\n" + "-"*70)
    print("THRESHOLD SWEEP -- Precision/Recall at various cosine thresholds")
    print("-"*70)
    print(f"{'Threshold':>9} {'TP':>4} {'FP':>4} {'FN':>4} {'TN':>4} "
          f"{'Precision':>9} {'Recall':>6} {'F1':>6}")
    for threshold in [0.30, 0.35, 0.40, 0.45, 0.50, 0.55, 0.60, 0.65, 0.70, 0.75, 0.80]:
        tp = sum(1 for s in connect_sims if s >= threshold)
        fn = sum(1 for s in connect_sims if s < threshold)
        fp = sum(1 for s in no_connect_sims if s >= threshold)
        tn = sum(1 for s in no_connect_sims if s < threshold)
        precision = tp / (tp + fp) if (tp + fp) > 0 else 0
        recall = tp / (tp + fn) if (tp + fn) > 0 else 0
        f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0
        print(f"   {threshold:.2f}    {tp:4d} {fp:4d} {fn:4d} {tn:4d} "
              f"   {precision:.3f}  {recall:.3f}  {f1:.3f}")

    # Show individual pair scores for ground truth sets
    print("\n" + "-"*70)
    print("SHOULD-CONNECT pairs sorted by similarity (lowest first -- hardest cases)")
    print("-"*70)
    scored = sorted(zip(SHOULD_CONNECT, connect_sims), key=lambda x: x[1])
    for (a, b), sim in scored[:10]:
        fact_a = next(f for f in FACTS if f["id"] == a)["fact"]
        fact_b = next(f for f in FACTS if f["id"] == b)["fact"]
        print(f"  [{a:2d}<->{b:2d}] sim={sim:.3f}")
        print(f"     A: {fact_a[:70]}")
        print(f"     B: {fact_b[:70]}")

    print("\n" + "-"*70)
    print("SHOULD-NOT-CONNECT pairs sorted by similarity (highest first -- false positives)")
    print("-"*70)
    scored_no = sorted(zip(SHOULD_NOT_CONNECT, no_connect_sims), key=lambda x: -x[1])
    for (a, b), sim in scored_no[:10]:
        fact_a = next(f for f in FACTS if f["id"] == a)["fact"]
        fact_b = next(f for f in FACTS if f["id"] == b)["fact"]
        print(f"  [{a:2d}<->{b:2d}] sim={sim:.3f}")
        print(f"     A: {fact_a[:70]}")
        print(f"     B: {fact_b[:70]}")

    # Entity overlap analysis: how many current edges would pass various thresholds?
    print("\n" + "-"*70)
    print(f"ENTITY-OVERLAP EDGE FILTERING (total entity-overlap pairs: {len(entity_overlap_sims)})")
    print("-"*70)
    for threshold in [0.40, 0.45, 0.50, 0.55, 0.60]:
        passing = sum(1 for s in entity_overlap_sims if s >= threshold)
        print(f"  threshold={threshold:.2f}: {passing}/{len(entity_overlap_sims)} edges survive "
              f"({100*passing/len(entity_overlap_sims):.0f}%)")


if __name__ == "__main__":
    main()
