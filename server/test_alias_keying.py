"""Headless test: subject-keying is exact-equality on canonical entity strings.

No LLM, no ChromaDB -- seed_belief / route_fact / view_of run purely in C++.
Verifies the "narrator as sole identity authority" model:
  1. Same canonical string  -> one merged subject (actor sees it).
  2. Different strings sharing a token -> NOT merged (no substring false-merge).
  3. A non-canonical tag     -> stays isolated (never silently fused onto a chain).

Run against the built _core.pyd:  python server/test_alias_keying.py
"""

import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

from rhapsode._core import CharacterMemory

failures = []


def check(cond, msg):
    status = "ok  " if cond else "FAIL"
    print(f"  [{status}] {msg}")
    if not cond:
        failures.append(msg)


def entities_of(mem):
    """All live belief-graph node entities, flattened."""
    out = []
    for n in mem.beliefs.all_nodes():
        out.extend(n.entities)
    return out


# -- 1. Canonical merge: same string -> one subject the actor can see ----------
print("1. canonical merge (both tagged \"Player\")")
mem = CharacterMemory("Sergeant Maren")
mem.seed_belief("The captain is my oldest friend", ["Player"], 0)
mem.route_fact("The captain rallied the garrison at the gate", ["Player"], 1)

view = mem.view_of(["Player"])
check("oldest friend" in view, "view_of(['Player']) surfaces the seeded belief")
check("rallied the garrison" in view, "view_of(['Player']) surfaces the routed fact")
check(all(e == "Player" for e in entities_of(mem)),
      "every belief-graph node is keyed on 'Player' (one subject, not split)")


# -- 2. False-merge gone: shared token does NOT merge --------------------------
print("2. no substring false-merge ('Ash' vs 'Ashenmoor')")
mem = CharacterMemory("Father Aldric")
mem.seed_belief("Ash is the keep's blacksmith", ["Ash"], 0)
mem.route_fact("Ashenmoor's eastern wall is breached", ["Ashenmoor"], 1)

view_ash = mem.view_of(["Ash"])
view_moor = mem.view_of(["Ashenmoor"])
check("blacksmith" in view_ash, "view_of(['Ash']) surfaces the Ash belief")
check("breached" not in view_ash, "view_of(['Ash']) does NOT bleed in the Ashenmoor fact")
check("breached" in view_moor, "view_of(['Ashenmoor']) surfaces the Ashenmoor fact")
check("blacksmith" not in view_moor, "view_of(['Ashenmoor']) does NOT bleed in the Ash belief")


# -- 3. Non-canonical tag stays isolated (correct failure mode) ----------------
print("3. a stray non-canonical tag does not silently merge onto 'Player'")
mem = CharacterMemory("Warden Elara Voss")
mem.seed_belief("The disgraced captain is a liability", ["Player"], 0)
mem.route_fact("Someone rallied soldiers in the yard", ["the captain"], 1)

view_player = mem.view_of(["Player"])
check("liability" in view_player, "view_of(['Player']) surfaces the canonical belief")
check("rallied soldiers" not in view_player,
      "a fact tagged 'the captain' is NOT fused onto the 'Player' chain")
check("rallied soldiers" in mem.view_of(["the captain"]),
      "the stray fact remains reachable under its own raw key")


# -- summary -------------------------------------------------------------------
print()
if failures:
    print(f"FAILED: {len(failures)} assertion(s)")
    for f in failures:
        print(f"  - {f}")
    sys.exit(1)
print("All alias-keying assertions passed.")
