"""Two blind readings of the same first-pass study in fresh LLM sessions."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from corpus import ROOT, load_config, load_local_env


SYSTEM = """You are an independent reader encountering a fictional person through a long first-pass study of the novels.

Read patiently. Write a letter to the eventual portraitist about the person you encountered: what continues to matter after reading, which scenes or relationships you keep returning to, what seems alive or difficult, and what remains not fully knowable.

Use the study as evidence. Do not turn the person into a list of traits, a diagnostic profile, a behavioral system, or a claim of complete knowledge. You are not writing the final portrait. Write one self-contained reading in plain prose. No process notes.
"""


def build_prompt(study: str) -> str:
    return "Read the first-pass study, then write your reading letter.\n\nFIRST-PASS STUDY:\n" + study


def _sha256(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def _experiment_config(cfg: dict[str, Any]) -> dict[str, Any]:
    exp = dict(cfg.get("reader_experiment") or {})
    critic = cfg["critic"]
    exp.setdefault("model", critic["model"])
    exp.setdefault("thinking", critic.get("thinking", False))
    exp.setdefault("temperature", 0.9)
    exp.setdefault("max_tokens", critic.get("max_tokens", 65536))
    exp.setdefault("runs", 2)
    exp.setdefault("request_limit", 2)
    exp.setdefault("seeds", [90101, 90102])
    return exp


def make_reader(cfg: dict[str, Any], *, seed: int) -> Any:
    from pydantic_ai import Agent
    from pydantic_ai.models.openai import OpenAIChatModel
    from pydantic_ai.providers.deepseek import DeepSeekProvider
    from pydantic_ai.providers.openai import OpenAIProvider
    from pydantic_ai.settings import ModelSettings

    exp = _experiment_config(cfg)
    key_name = cfg["critic"]["api_key_env"]
    key = os.environ.get(key_name, "")
    if not key:
        raise SystemExit(f"missing env {key_name}")

    base_name = str(exp.get("api_base_env") or "").strip()
    base_url = os.environ.get(base_name, "").strip() if base_name else ""
    provider = (
        OpenAIProvider(base_url=base_url, api_key=key)
        if base_url
        else DeepSeekProvider(api_key=key)
    )
    model = OpenAIChatModel(exp["model"], provider=provider)
    settings: dict[str, Any] = {
        "temperature": float(exp["temperature"]),
        "max_tokens": int(exp["max_tokens"]),
        "seed": seed,
    }
    if exp.get("thinking"):
        settings["thinking"] = True
        settings["extra_body"] = {"thinking": {"type": "enabled"}}
    return Agent(
        model,
        system_prompt=SYSTEM,
        model_settings=ModelSettings(**settings),
    )


def _output_dir(cfg: dict[str, Any], requested: str) -> Path:
    if requested:
        raw = Path(requested)
        return raw if raw.is_absolute() else ROOT / raw
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    return ROOT / cfg["paths"]["checkpoints"] / f"reader-baseline-{stamp}"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run blind readers with identical input in independent LLM sessions"
    )
    parser.add_argument("--config", default="config.yaml")
    parser.add_argument("--out-dir", default="", help="default: timestamped checkpoint folder")
    parser.add_argument("--runs", type=int, default=0, help="default: reader_experiment.runs")
    parser.add_argument("--dry-run", action="store_true", help="validate inputs without calling the model")
    args = parser.parse_args()

    load_local_env()
    cfg = load_config(args.config)
    exp = _experiment_config(cfg)
    runs = args.runs or int(exp["runs"])
    if runs < 2:
        raise SystemExit("reader experiment requires at least two sessions")
    seeds = [int(seed) for seed in exp.get("seeds", [])]
    if len(seeds) < runs:
        raise SystemExit(f"reader_experiment.seeds needs at least {runs} values")

    study_path = ROOT / cfg["paths"]["study"]
    if not study_path.is_file():
        raise SystemExit(f"missing first-pass study {study_path}")
    study = study_path.read_text(encoding="utf-8")
    prompt = build_prompt(study)
    out_dir = _output_dir(cfg, args.out_dir)

    manifest: dict[str, Any] = {
        "experiment": "identical-prompt-independent-seeded-readers",
        "character": cfg.get("character"),
        "model": exp["model"],
        "temperature": float(exp["temperature"]),
        "thinking": bool(exp.get("thinking")),
        "custom_api_base": bool(
            str(exp.get("api_base_env") or "").strip()
            and os.environ.get(str(exp.get("api_base_env")), "").strip()
        ),
        "runs": runs,
        "seeds": seeds[:runs],
        "first_pass": str(study_path),
        "first_pass_chars": len(study),
        "first_pass_sha256": _sha256(study),
        "system_sha256": _sha256(SYSTEM),
        "prompt_sha256": _sha256(prompt),
        "session_isolation": "new Agent and run_sync call; no message history",
        "outputs": [],
    }

    if args.dry_run:
        print(json.dumps(manifest, ensure_ascii=False, indent=2))
        return 0

    out_dir.mkdir(parents=True, exist_ok=False)
    manifest_path = out_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")

    for index in range(1, runs + 1):
        from pydantic_ai.usage import UsageLimits

        # A new Agent instance and a new call guarantee that no conversation
        # history from another reader is supplied to this reader.
        reader = make_reader(cfg, seed=seeds[index - 1])
        result = reader.run_sync(
            prompt,
            usage_limits=UsageLimits(request_limit=int(exp["request_limit"])),
        )
        filename = f"reader-{index}.md"
        (out_dir / filename).write_text(result.output.strip() + "\n", encoding="utf-8")
        manifest["outputs"].append(filename)
        manifest_path.write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        print(f"reader {index}/{runs} -> {out_dir / filename}")

    print(f"manifest -> {manifest_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
