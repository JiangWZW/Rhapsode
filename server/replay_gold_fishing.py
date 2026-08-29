"""Regression replay: the gold-and-fishing session.

Drives the live /ws session with the fixed five-input script that exposed the
frozen-world failure (2026-08-28): dump unclaimed gold, ration Aqua, then walk
away and idle for a fictional day. Prints the full transcript so a human can
judge the pass criteria:

  1. Someone acts on the unclaimed gold by turn 2 (the world moves unprompted).
  2. The mansion is occupied when the player comes home at night.
  3. No duplicate beat when the player input is redundant (turns 3 vs 4).
  4. Darkness stays baseline on the rationing beat (no trigger-free kink line).

Usage: server must NOT be running; start it, then:
    .venv\\Scripts\\python.exe replay_gold_fishing.py [--host 127.0.0.1] [--port 8080]
"""

import argparse
import asyncio
import json
import sys
import time

import websockets

INPUTS = [
    'Take out a huge pile of pure gold. "Here it is. I sold my precious '
    'intellectual properties." "I am tired. I want to rest." Go out to my '
    'usual spot for fishing.',
    '"It\'s enough for the debt. Remaining part will be preserved for the '
    'spending from my teammates, except Aqua - only provide her most basic '
    'food and equipments and nothing else."',
    'Enjoy my own fishing there. Hope to catch some for lunch.',
    'Fishing till the night. Then bring the fhsh bucket back home.',
    'Go back home. Open the door.',
]

TURN_TIMEOUT_S = 900


def label(payload: dict) -> str:
    kind = payload.get("scene_kind", "narrator")
    if kind == "character":
        return payload.get("speaker", "???")
    return "NARRATOR"


async def wait_for_idle(ws, transcript: list[str]) -> None:
    deadline = time.monotonic() + TURN_TIMEOUT_S
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("turn did not reach idle in time")
        raw = await asyncio.wait_for(ws.recv(), timeout=remaining)
        payload = json.loads(raw)
        kind = payload.get("type")
        if kind == "scene_message":
            line = f"{label(payload)}: {payload['content']}"
            transcript.append(line)
            print(line, flush=True)
            print("-" * 40, flush=True)
        elif kind == "error":
            line = f"ERROR: {payload.get('detail')}"
            transcript.append(line)
            print(line, flush=True)
        elif kind == "status" and payload.get("state") == "idle":
            return


async def replay(host: str, port: int) -> None:
    uri = f"ws://{host}:{port}/ws"
    transcript: list[str] = []
    async with websockets.connect(uri, max_size=None) as ws:
        print("=== seed ===", flush=True)
        await wait_for_idle(ws, transcript)
        for i, text in enumerate(INPUTS):
            print(f"\n=== turn {i} ===", flush=True)
            print(f"YOU: {text}", flush=True)
            print("-" * 40, flush=True)
            started = time.monotonic()
            await ws.send(json.dumps({"type": "player_message", "content": text}))
            await wait_for_idle(ws, transcript)
            print(f"(turn {i} idle after {time.monotonic() - started:.0f}s)",
                  flush=True)
    print("\n=== replay complete ===", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()
    try:
        asyncio.run(replay(args.host, args.port))
    except TimeoutError as exc:
        print(f"TIMEOUT: {exc}", file=sys.stderr)
        sys.exit(2)
