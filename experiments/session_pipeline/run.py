"""Thin CLI for the C++ SessionEvalRunner.

Defaults come from config.toml next to this script. CLI flags override.

  .venv\\Scripts\\python.exe run.py --turns 2
  .venv\\Scripts\\python.exe run.py --config config.toml --guide guides/default.md
"""
from __future__ import annotations

import argparse
import os
import sys
import tomllib
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SERVER = ROOT / "server"
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(SERVER))
sys.path.insert(0, str(HERE))

from dotenv import load_dotenv  # noqa: E402

load_dotenv(SERVER / ".env")

from player_agent import make_player_llm  # noqa: E402
from rhapsode._core import EndReason, SessionEvalConfig, SessionEvalRunner  # noqa: E402
from rhapsode.config import SCENARIO_PATH, configure_logging  # noqa: E402
from rhapsode.llm import complete  # noqa: E402

configure_logging()


def _critique_llm(prompt: str) -> str:
    model = os.environ.get("RHAPSODE_PLAYER_MODEL")
    return complete([{"role": "user", "content": prompt}], model=model)


def _resolve(path: str | Path, base: Path = HERE) -> Path:
    p = Path(path)
    if p.is_file():
        return p
    candidate = base / p
    if candidate.is_file():
        return candidate
    return p


def _load_text(path: str | Path) -> str:
    return _resolve(path).read_text(encoding="utf-8").strip()


def _load_config(path: Path) -> dict[str, Any]:
    with path.open("rb") as f:
        return tomllib.load(f)


def main() -> int:
    pre = argparse.ArgumentParser(add_help=False)
    pre.add_argument(
        "--config",
        default=str(HERE / "config.toml"),
        help="Pipeline config TOML (default: config.toml beside run.py)",
    )
    pre_args, remaining = pre.parse_known_args()
    cfg_path = _resolve(pre_args.config)
    file_cfg = _load_config(cfg_path) if cfg_path.is_file() else {}
    player_cfg = file_cfg.get("player", {})
    run_cfg = file_cfg.get("run", {})

    default_saves = run_cfg.get("saves_dir") or str(SERVER / "saves")
    default_guide = player_cfg.get("guide", "")
    if not default_guide:
        default_guide = os.environ.get("RHAPSODE_PLAYER_GUIDE", "")

    parser = argparse.ArgumentParser(
        description="Rhapsode session eval pipeline",
        parents=[pre],
    )
    parser.add_argument("--turns", type=int, default=int(run_cfg.get("turns", 3)))
    parser.add_argument(
        "--host",
        default=os.environ.get("RHAPSODE_HOST", run_cfg.get("host", "127.0.0.1")),
    )
    parser.add_argument(
        "--port",
        default=os.environ.get("RHAPSODE_PORT", str(run_cfg.get("port", "8080"))),
    )
    parser.add_argument("--ws-path", default=str(run_cfg.get("ws_path", "/ws")))
    parser.add_argument("--saves-dir", default=str(default_saves))
    parser.add_argument(
        "--out-dir",
        default="",
        help="Run output directory (default: runs/<timestamp>)",
    )
    parser.add_argument("--spawn-cmd", default="")
    parser.add_argument(
        "--turn-timeout",
        type=int,
        default=int(run_cfg.get("turn_timeout_s", 1200)),
    )
    parser.add_argument(
        "--open-timeout",
        type=int,
        default=int(run_cfg.get("open_timeout_s", 1200)),
    )
    parser.add_argument(
        "--protocol",
        default=str(player_cfg.get("protocol", "protocol.md")),
        help="Player protocol markdown (relative to config dir or cwd)",
    )
    parser.add_argument(
        "--guide",
        default=str(default_guide),
        help="Experiment brief markdown (empty to disable)",
    )
    parser.add_argument(
        "--critique",
        action="store_true",
        help="Also run an LLM critique pass in SessionReport",
    )
    args = parser.parse_args(remaining)

    out_dir = args.out_dir
    if not out_dir:
        ts = datetime.now().strftime("%Y%m%d-%H%M%S")
        out_dir = str(HERE / "runs" / ts)

    config_dir = cfg_path.parent if cfg_path.is_file() else HERE
    protocol = _load_text(_resolve(args.protocol, config_dir))
    guide_text = ""
    if args.guide:
        guide_text = _load_text(_resolve(args.guide, config_dir))
    empty_action = str(
        player_cfg.get("empty_action", "I look around carefully.")
    )

    cfg = SessionEvalConfig()
    cfg.ws_host = args.host
    cfg.ws_port = str(args.port)
    cfg.ws_path = args.ws_path
    cfg.server_cmd = args.spawn_cmd
    cfg.saves_dir = args.saves_dir
    cfg.out_dir = out_dir
    cfg.max_turns = args.turns
    cfg.turn_timeout_s = args.turn_timeout
    cfg.open_timeout_s = args.open_timeout

    player = make_player_llm(
        args.saves_dir,
        SCENARIO_PATH,
        protocol=protocol,
        guide_text=guide_text,
        empty_action=empty_action,
    )

    runner = SessionEvalRunner(cfg)
    runner.set_player_llm(player)
    if args.critique:
        runner.set_critique_llm(_critique_llm)

    reason = runner.run()
    report_path = Path(out_dir) / "report.md"
    print(f"end_reason={reason}")
    print(f"report={report_path}")
    if report_path.exists():
        print(report_path.read_text(encoding="utf-8"))
    return 0 if reason == EndReason.MaxTurns else 1


if __name__ == "__main__":
    raise SystemExit(main())
