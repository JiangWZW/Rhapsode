"""
Auto-test: Play several turns via the Rhapsode WebSocket and log graph state.
Tests chain topology edge creation by verifying edge counts stay low.

Requires: the Rhapsode server already running on localhost:8080

Output: experiments/chain_evolution/autotest_log.txt
"""
import sys
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

import asyncio
import json
import time
from datetime import datetime
from pathlib import Path

try:
    import websockets
    import httpx
except ImportError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install",
                          "websockets", "httpx", "--quiet"])
    import websockets
    import httpx

BASE_URL = "http://localhost:8080"
WS_URL = "ws://localhost:8080/ws"
OUT_DIR = Path(__file__).resolve().parent
OUT_DIR.mkdir(parents=True, exist_ok=True)
LOG_FILE = OUT_DIR / "autotest_log.txt"

PLAYER_INPUTS = [
    # Strategy 1: Direct action (entity propagation)
    "I approach Warden Voss at the eastern gate and demand to know why she sealed it.",
    # Strategy 2: Information gathering (new entity creation)
    "I search the barracks for any soldiers willing to talk about the siege preparations.",
    # Strategy 3: Relationship building (character memory)
    "I find Sergeant Maren and offer to help tend her wound in exchange for information about the garrison's morale.",
    # Strategy 4: Exploration (location-based facts)
    "I slip away at night to investigate the catacombs beneath the keep that Father Aldric has been visiting.",
    # Strategy 5: Confrontation (multi-entity interactions)
    "I confront Father Aldric about Lord Harren's relic. What is he hiding beneath Ashenmoor?",
]


def log(f, msg):
    line = f"[{datetime.now().strftime('%H:%M:%S')}] {msg}"
    print(line, flush=True)
    f.write(line + "\n")
    f.flush()


def get_graph_state():
    try:
        r = httpx.get(f"{BASE_URL}/analyze", timeout=10)
        return r.json()
    except Exception as e:
        return {"error": str(e)}


def get_graph_dot():
    try:
        r = httpx.get(f"{BASE_URL}/graph.dot", timeout=10)
        if r.status_code != 200:
            return None
        dot_text = r.text
        edges = [l.strip() for l in dot_text.split("\n") if " -> " in l]
        return {"edge_lines": edges, "edge_count": len(edges), "dot": dot_text}
    except Exception as e:
        return {"error": str(e)}


async def wait_for_idle(ws, timeout_s, f, label=""):
    """Consume messages until status=idle or timeout. Returns all scene messages."""
    messages = []
    start = time.time()
    while True:
        remaining = timeout_s - (time.time() - start)
        if remaining <= 0:
            log(f, f"  [{label}] timed out after {timeout_s}s")
            break
        try:
            raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
            data = json.loads(raw)
            msg_type = data.get("type", "unknown")

            if msg_type == "scene_message":
                content = data.get("content", "")[:80]
                log(f, f"  [{label}|scene] {content}")
                messages.append(data)
            elif msg_type == "status":
                state = data.get("state", "")
                log(f, f"  [{label}|status] {state}")
                if state == "idle":
                    break
            elif msg_type == "error":
                log(f, f"  [{label}|ERROR] {data.get('detail', '')[:80]}")
                break
            elif msg_type == "user_message":
                messages.append(data)
            else:
                log(f, f"  [{label}|{msg_type}] {str(data)[:60]}")
        except asyncio.TimeoutError:
            log(f, f"  [{label}] recv timeout")
            break
    return messages


async def run_test():
    with open(LOG_FILE, "w", encoding="utf-8") as f:
        log(f, "=" * 60)
        log(f, "RHAPSODE CHAIN TOPOLOGY AUTO-TEST")
        log(f, f"Started: {datetime.now().isoformat()}")
        log(f, f"Server: {BASE_URL}")
        log(f, "=" * 60)

        state = get_graph_state()
        log(f, f"\nInitial graph: {json.dumps(state)}")

        log(f, f"\nConnecting to {WS_URL}...")
        try:
            async with websockets.connect(
                WS_URL, max_size=2**20, open_timeout=30, close_timeout=10
            ) as ws:
                log(f, "Connected.")

                # Phase 1: Wait for the server to finish its opening scene generation.
                # The server sends seed messages, then processes turn 0 (opening narration)
                # and sends the result. After that it goes idle.
                log(f, "\nPhase 1: Waiting for opening scene (up to 180s)...")
                seed_msgs = await wait_for_idle(ws, 180, f, "init")
                log(f, f"Opening scene complete: {len(seed_msgs)} messages received.")

                state = get_graph_state()
                log(f, f"Graph after opening: {json.dumps(state)}")

                # Phase 2: Play turns and measure graph growth
                log(f, "\nPhase 2: Playing turns...")
                results = []

                for i, player_input in enumerate(PLAYER_INPUTS, 1):
                    log(f, f"\n{'='*60}")
                    log(f, f"TURN {i}: \"{player_input[:60]}\"")
                    log(f, f"{'='*60}")

                    # Snapshot before
                    pre_state = get_graph_state()

                    # Send player action
                    await ws.send(json.dumps({
                        "type": "player_message",
                        "content": player_input
                    }))

                    # Wait for response (LLM generation can take 30-60s per turn)
                    turn_msgs = await wait_for_idle(ws, 180, f, f"t{i}")

                    elapsed_turn = len(turn_msgs)
                    log(f, f"  Turn {i} done: {elapsed_turn} response messages")

                    # Post-turn graph analysis
                    await asyncio.sleep(1)
                    post_state = get_graph_state()
                    dot_info = get_graph_dot()

                    nodes = post_state.get("live_node_count", 0)
                    edges = dot_info["edge_count"] if dot_info and "edge_count" in dot_info else post_state.get("active_edge_count", 0)
                    orphans = post_state.get("orphan_count", 0)
                    new_nodes = nodes - pre_state.get("live_node_count", 0)
                    new_edges = edges - (pre_state.get("active_edge_count", 0))

                    ratio = edges / max(nodes, 1)

                    log(f, f"  GRAPH: nodes={nodes} (+{new_nodes}), "
                          f"edges={edges} (+{new_edges}), orphans={orphans}")
                    log(f, f"  RATIO: {ratio:.2f} edges/node "
                          f"(chain target: <2.5)")

                    results.append({
                        "turn": i,
                        "input": player_input[:40],
                        "nodes": nodes,
                        "edges": edges,
                        "new_nodes": new_nodes,
                        "new_edges": new_edges,
                        "ratio": ratio,
                        "orphans": orphans,
                    })

                    # Save DOT snapshot
                    if dot_info and "dot" in dot_info:
                        dot_path = OUT_DIR / f"turn_{i:02d}.dot"
                        with open(dot_path, "w", encoding="utf-8") as df:
                            df.write(dot_info["dot"])

                # Phase 3: Final summary
                log(f, f"\n{'='*60}")
                log(f, "FINAL SUMMARY")
                log(f, f"{'='*60}")

                log(f, "\nTurn | Nodes | Edges | +Nodes | +Edges | Ratio  | Orphans")
                log(f, "-----|-------|-------|--------|--------|--------|--------")
                for r in results:
                    log(f, f"  {r['turn']}  |  {r['nodes']:>3}  |  {r['edges']:>3}  "
                          f"|   {r['new_nodes']:>2}   |   {r['new_edges']:>2}   "
                          f"|  {r['ratio']:.2f}  |   {r['orphans']}")

                final_ratio = results[-1]["ratio"] if results else 0
                log(f, f"\nFinal edge/node ratio: {final_ratio:.2f}")

                if final_ratio < 2.5:
                    log(f, "VERDICT: PASS - Chain topology confirmed (ratio < 2.5)")
                elif final_ratio < 4.0:
                    log(f, "VERDICT: WARN - Moderate connectivity (2.5-4.0)")
                else:
                    log(f, "VERDICT: FAIL - Clique-like behavior (ratio >= 4.0)")

                # Additional chain-specific checks
                log(f, "\n--- Chain Topology Health ---")
                avg_new_edges = sum(r["new_edges"] for r in results) / max(len(results), 1)
                log(f, f"Avg new edges per turn: {avg_new_edges:.1f} "
                      f"(chain expects ~1-3 per node added)")
                max_new = max((r["new_edges"] for r in results), default=0)
                log(f, f"Max new edges in a turn: {max_new} "
                      f"(clique would show 5+ per new node)")

        except Exception as e:
            log(f, f"\nFATAL ERROR: {e}")
            import traceback
            f.write(traceback.format_exc())
            traceback.print_exc()

        log(f, f"\nLog saved to: {LOG_FILE}")


if __name__ == "__main__":
    asyncio.run(run_test())
