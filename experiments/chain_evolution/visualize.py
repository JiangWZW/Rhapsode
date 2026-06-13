"""
Chain Evolution Visualizer — compares shares_entity (current) vs chain topology (proposed)
Renders step-by-step Graphviz PNGs showing graph growth.

Output: experiments/chain_evolution/step_XX_{chain,clique}.png
"""

import os
import sys
import graphviz

os.environ["PATH"] += os.pathsep + r"C:\Program Files\Graphviz\bin"

OUT_DIR = os.path.dirname(os.path.abspath(__file__))
os.makedirs(OUT_DIR, exist_ok=True)

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

ENTITY_COLORS = {
    "lord harren": "#e74c3c",
    "ashenmoor": "#3498db",
    "warden elara voss": "#9b59b6",
    "thornfield": "#27ae60",
    "father aldric": "#f39c12",
    "sergeant maren": "#1abc9c",
    "duke": "#8e44ad",
    "player": "#2c3e50",
}


def entity_color(entities):
    for e in entities:
        c = ENTITY_COLORS.get(e.lower())
        if c:
            return c
    return "#7f8c8d"


def shares_entity(a, b):
    ea = {e.lower() for e in a["entities"]}
    eb = {e.lower() for e in b["entities"]}
    return bool(ea & eb)


def build_chain_edges(existing, new_node):
    """Return list of (from_id, to_id, shared_entity) for chain topology."""
    most_recent = {}
    for prior in existing:
        for e in prior["entities"]:
            most_recent[e.lower()] = prior["id"]

    edges = []
    connected = set()
    for e in new_node["entities"]:
        target = most_recent.get(e.lower())
        if target and target not in connected:
            edges.append((target, new_node["id"], e.lower()))
            connected.add(target)
    return edges


def build_clique_edges(existing, new_node):
    """Return list of (from_id, to_id, shared_entity) for current shares_entity."""
    edges = []
    connected = set()
    for prior in existing:
        if not shares_entity(new_node, prior):
            continue
        if prior["id"] in connected:
            continue
        shared = {e.lower() for e in new_node["entities"]} & {e.lower() for e in prior["entities"]}
        edge_entity = sorted(shared)[0]
        from_id = min(prior["id"], new_node["id"])
        to_id = max(prior["id"], new_node["id"])
        edges.append((from_id, to_id, edge_entity))
        connected.add(prior["id"])
    return edges


def render_graph(nodes, edges, step, mode, new_node_id=None, stats=None):
    """Render graph as PNG via graphviz."""
    g = graphviz.Digraph(
        f"step_{step:02d}_{mode}",
        format="png",
        graph_attr={
            "rankdir": "LR",
            "bgcolor": "#1a1a2e",
            "fontcolor": "white",
            "fontname": "Consolas",
            "label": f"Step {step} | {mode.upper()} | {len(nodes)} nodes, {len(edges)} edges"
                     + (f" | +node {new_node_id}" if new_node_id else ""),
            "labelloc": "t",
            "fontsize": "14",
            "pad": "0.5",
            "nodesep": "0.4",
            "ranksep": "0.6",
        },
        node_attr={
            "shape": "box",
            "style": "filled,rounded",
            "fontname": "Consolas",
            "fontsize": "9",
            "margin": "0.15,0.08",
        },
        edge_attr={
            "color": "#555555",
            "arrowsize": "0.6",
        },
    )

    for n in nodes:
        color = entity_color(n["entities"])
        is_new = (n["id"] == new_node_id)
        label = f"{n['id']}| {n['fact']}"
        g.node(
            str(n["id"]),
            label=label,
            fillcolor=color if not is_new else "#ffff00",
            fontcolor="white" if not is_new else "black",
            penwidth="3" if is_new else "1",
        )

    for (from_id, to_id, ent) in edges:
        color = ENTITY_COLORS.get(ent, "#555555")
        g.edge(str(from_id), str(to_id), color=color, penwidth="1.5")

    filepath = os.path.join(OUT_DIR, f"step_{step:02d}_{mode}")
    g.render(filepath, cleanup=True)
    return filepath + ".png"


def main():
    chain_edges = []
    clique_edges = []
    existing = []

    print(f"Generating chain evolution graphs in: {OUT_DIR}")
    print(f"{'Step':<5} {'Node':<5} {'Chain edges':<13} {'Clique edges':<14} {'Ratio'}")
    print("-" * 55)

    for i, node in enumerate(FACTS):
        new_chain = build_chain_edges(existing, node)
        new_clique = build_clique_edges(existing, node)

        chain_edges.extend(new_chain)
        clique_edges.extend(new_clique)

        existing.append(node)
        step = i + 1

        ratio = f"{len(clique_edges)/max(len(chain_edges),1):.1f}x" if chain_edges else "n/a"
        print(f"{step:<5} {node['id']:<5} {len(chain_edges):<13} {len(clique_edges):<14} {ratio}")

        nodes_so_far = existing[:]
        render_graph(nodes_so_far, chain_edges[:], step, "chain",
                     new_node_id=node["id"])
        render_graph(nodes_so_far, clique_edges[:], step, "clique",
                     new_node_id=node["id"])

    print(f"\n{'='*55}")
    print(f"FINAL: Chain={len(chain_edges)} edges, Clique={len(clique_edges)} edges")
    print(f"Reduction: {100*(1 - len(chain_edges)/len(clique_edges)):.0f}%")
    print(f"\nOutput: {OUT_DIR}")


if __name__ == "__main__":
    main()
