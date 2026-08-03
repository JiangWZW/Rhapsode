"""Thin CLI for the C++ SessionEvalRunner.

Defaults come from config.toml next to this script. CLI flags override.

By default the runner *spawns* uvicorn and tees its stdout/stderr into
``<out_dir>/console.log`` (C++ SessionEvalRunner contract). Use ``--attach``
only when you intentionally reuse an already-running server (no server log
capture). LLM hop timings also go to ``<out_dir>/llm_profile.jsonl``.

  .venv\\Scripts\\python.exe run.py --turns 2
  .venv\\Scripts\\python.exe run.py --config config.toml --guide guides/default.md
"""
from __future__ import annotations

import argparse
import os
import socket
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
    return complete(
        [{"role": "user", "content": prompt}], model=model, stage="critique")


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


def _port_open(host: str, port: str | int) -> bool:
    try:
        with socket.create_connection((host, int(port)), timeout=0.5):
            return True
    except OSError:
        return False


def _default_spawn_cmd(host: str, port: str) -> str:
    """Spawn venv uvicorn under server/. Prefer repo venv, not whatever launched run.py."""
    venv_py = SERVER / ".venv" / "Scripts" / "python.exe"
    py = str(venv_py if venv_py.is_file() else Path(sys.executable))
    server = str(SERVER)
    # cmd /c is needed because CreateProcess has no cwd today; C++ job object
    # kills the whole tree on stop so uvicorn does not orphan.
    return (
        f'cmd.exe /c "cd /d {server} && {py} -m uvicorn rhapsode.app:app '
        f'--host {host} --port {port}"'
    )


def _enable_run_profiling(out_dir: Path) -> Path:
    """Point DeepSeek hop profiling at the run folder (inherited by spawned server)."""
    out_dir.mkdir(parents=True, exist_ok=True)
    profile = out_dir / "llm_profile.jsonl"
    os.environ["RHAPSODE_LLM_PROFILE"] = "1"
    os.environ["RHAPSODE_LLM_PROFILE_PATH"] = str(profile)
    os.environ["RHAPSODE_LOG_DIR"] = str(out_dir)
    return profile


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
        "--attach",
        action="store_true",
        help=(
            "Connect to an already-running server (no spawn). "
            "Server stdout is NOT written to console.log — timing breakdown "
            "will be missing unless that server was started with profiling."
        ),
    )
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
    out_path = Path(out_dir)
    profile_path = _enable_run_profiling(out_path)

    config_dir = cfg_path.parent if cfg_path.is_file() else HERE
    protocol = _load_text(_resolve(args.protocol, config_dir))
    guide_text = ""
    if args.guide:
        guide_text = _load_text(_resolve(args.guide, config_dir))
    empty_action = str(
        player_cfg.get("empty_action", "I look around carefully.")
    )

    spawn_cmd = (args.spawn_cmd or "").strip()
    if args.attach:
        if spawn_cmd:
            print("error: --attach and --spawn-cmd are mutually exclusive",
                  file=sys.stderr)
            return 2
        spawn_cmd = ""
        print(
            "warning: --attach mode; console.log will not contain server LLM "
            f"logs. Prefer default spawn. profile still at {profile_path} "
            "only if the existing server was started with RHAPSODE_LLM_PROFILE.",
            file=sys.stderr,
        )
    elif not spawn_cmd:
        spawn_cmd = _default_spawn_cmd(args.host, str(args.port))
        if _port_open(args.host, args.port):
            print(
                f"error: {args.host}:{args.port} is already listening.\n"
                "  Free the port (stop the old uvicorn), or pass --attach to "
                "reuse it without capturing console.log.",
                file=sys.stderr,
            )
            return 2

    cfg = SessionEvalConfig()
    cfg.ws_host = args.host
    cfg.ws_port = str(args.port)
    cfg.ws_path = args.ws_path
    cfg.server_cmd = spawn_cmd
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

    print(f"out_dir={out_path}")
    print(f"spawn={'attach' if not spawn_cmd else 'uvicorn→console.log'}")
    print(f"llm_profile={profile_path}")

    reason = runner.run()
    report_path = out_path / "report.md"
    print(f"end_reason={reason}")
    print(f"report={report_path}")
    console_log = out_path / "console.log"
    if console_log.is_file():
        text = console_log.read_text(encoding="utf-8", errors="replace")
        if "attaching to existing server" in text and "elapsed_ms=" not in text:
            print(
                "warning: console.log has no server timing lines "
                "(attach mode or spawn failed to capture stdout)",
                file=sys.stderr,
            )
    if profile_path.is_file() and profile_path.stat().st_size > 0:
        print(f"llm_profile_bytes={profile_path.stat().st_size}")
    else:
        print(
            "warning: llm_profile.jsonl missing or empty "
            "(spawned server should inherit RHAPSODE_LLM_PROFILE)",
            file=sys.stderr,
        )
    if report_path.exists():
        print(report_path.read_text(encoding="utf-8"))
    return 0 if reason == EndReason.MaxTurns else 1


if __name__ == "__main__":
    raise SystemExit(main())
