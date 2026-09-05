"""Continue each saved blind reader into an independent character study."""

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
from critic import _study_body
from reader_experiment import build_prompt as build_reader_prompt


PROMPT_VERSION = "reader-authored-study-v1"

FINAL_TEMPLATE = """You have read the full first-pass study and written a letter about the character you encountered. Now write the character study itself.

Keep {character_name} as the living center of the piece, and let its form grow from your understanding. Draw on scenes, relationships, patterns, and implications wherever they help the character come into view. Follow your interpretation as far as the text and your judgment can sustain it, while allowing uncertainty where you genuinely encounter it.

Write a unified work of prose rather than a letter or a set of notes. Begin with this heading:

# {character_name}
"""


def build_study_prompt(character_name: str) -> str:
    name = character_name.strip()
    if not name:
        raise ValueError("character name must not be empty")
    return FINAL_TEMPLATE.format(character_name=name)


def _sha256(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def _experiment_config(cfg: dict[str, Any]) -> dict[str, Any]:
    exp = dict(cfg.get("study_experiment") or {})
    reader = cfg["reader_experiment"]
    critic = cfg["critic"]
    exp.setdefault("api_base_env", reader.get("api_base_env", ""))
    exp.setdefault("max_tokens", reader.get("max_tokens", critic.get("max_tokens", 65536)))
    exp.setdefault("request_limit", reader.get("request_limit", 2))
    return exp


def make_writer(
    cfg: dict[str, Any], reader_manifest: dict[str, Any], *, seed: int
) -> Any:
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
    settings: dict[str, Any] = {
        "temperature": float(reader_manifest["temperature"]),
        "max_tokens": int(exp["max_tokens"]),
        "seed": seed,
    }
    if reader_manifest.get("thinking"):
        settings["thinking"] = True
        settings["extra_body"] = {"thinking": {"type": "enabled"}}
    return Agent(
        OpenAIChatModel(str(reader_manifest["model"]), provider=provider),
        model_settings=ModelSettings(**settings),
    )


def build_history(study: str, letter: str, model_name: str) -> list[Any]:
    from pydantic_ai.messages import ModelRequest, ModelResponse, TextPart, UserPromptPart

    return [
        ModelRequest(parts=[UserPromptPart(build_reader_prompt(study))]),
        ModelResponse(parts=[TextPart(letter)], model_name=model_name),
    ]


def _latest_reader_dir(cfg: dict[str, Any]) -> Path:
    root = ROOT / cfg["paths"]["checkpoints"]
    for candidate in sorted(root.glob("reader-baseline-*"), reverse=True):
        manifest_path = candidate / "manifest.json"
        if not manifest_path.is_file():
            continue
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        outputs = manifest.get("outputs") or []
        if len(outputs) == 2 and all((candidate / item).is_file() for item in outputs):
            return candidate
    raise SystemExit(f"no complete two-reader experiment under {root}")


def _resolve_reader_dir(cfg: dict[str, Any], requested: str) -> Path:
    if not requested:
        return _latest_reader_dir(cfg)
    raw = Path(requested)
    return raw if raw.is_absolute() else ROOT / raw


def _output_dir(cfg: dict[str, Any], requested: str) -> Path:
    if requested:
        raw = Path(requested)
        return raw if raw.is_absolute() else ROOT / raw
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    return ROOT / cfg["paths"]["checkpoints"] / f"study-candidates-{stamp}"


def _load_inputs(
    cfg: dict[str, Any], reader_dir: Path
) -> tuple[Path, str, dict[str, Any], list[Path], list[str]]:
    study_path = ROOT / cfg["paths"]["study"]
    if not study_path.is_file():
        raise SystemExit(f"missing first-pass study {study_path}")
    study = study_path.read_text(encoding="utf-8")

    manifest_path = reader_dir / "manifest.json"
    if not manifest_path.is_file():
        raise SystemExit(f"missing reader manifest {manifest_path}")
    reader_manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if reader_manifest.get("first_pass_sha256") != _sha256(study):
        raise SystemExit("reader experiment and current first-pass study do not match")

    names = list(reader_manifest.get("outputs") or [])
    if len(names) != 2:
        raise SystemExit("study experiment requires exactly two reader letters")
    letter_paths = [reader_dir / name for name in names]
    missing = [str(path) for path in letter_paths if not path.is_file()]
    if missing:
        raise SystemExit("missing reader output(s): " + ", ".join(missing))
    letters = [path.read_text(encoding="utf-8") for path in letter_paths]
    return study_path, study, reader_manifest, letter_paths, letters


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Continue each blind reader into its own candidate character study"
    )
    parser.add_argument("--config", default="config.yaml")
    parser.add_argument("--reader-dir", default="", help="default: latest complete reader baseline")
    parser.add_argument("--out-dir", default="", help="default: timestamped checkpoint folder")
    parser.add_argument("--dry-run", action="store_true", help="validate and describe without calling the model")
    args = parser.parse_args()

    load_local_env()
    cfg = load_config(args.config)
    exp = _experiment_config(cfg)
    reader_dir = _resolve_reader_dir(cfg, args.reader_dir)
    study_path, study, reader_manifest, letter_paths, letters = _load_inputs(cfg, reader_dir)
    character_name = str(cfg.get("character") or "").strip()
    final_prompt = build_study_prompt(character_name)
    out_dir = _output_dir(cfg, args.out_dir)

    seeds = [int(seed) for seed in reader_manifest.get("seeds") or []]
    if len(seeds) < len(letters):
        raise SystemExit("reader manifest does not contain a seed for each letter")

    manifest: dict[str, Any] = {
        "experiment": "reader-authored-independent-character-studies",
        "prompt_version": PROMPT_VERSION,
        "character": character_name,
        "model": reader_manifest["model"],
        "temperature": float(reader_manifest["temperature"]),
        "thinking": bool(reader_manifest.get("thinking")),
        "seeds": seeds[: len(letters)],
        "custom_api_base": bool(
            str(exp.get("api_base_env") or "").strip()
            and os.environ.get(str(exp.get("api_base_env")), "").strip()
        ),
        "session_design": (
            "two isolated reconstructed continuations; each contains the original Pass-1 user "
            "prompt and only that reader's saved assistant letter"
        ),
        "first_pass": str(study_path),
        "first_pass_chars": len(study),
        "first_pass_sha256": _sha256(study),
        "reader_dir": str(reader_dir),
        "letters": [
            {"path": str(path), "chars": len(letter), "sha256": _sha256(letter)}
            for path, letter in zip(letter_paths, letters)
        ],
        "template_sha256": _sha256(FINAL_TEMPLATE),
        "final_prompt_sha256": _sha256(final_prompt),
        "outputs": [],
    }

    if args.dry_run:
        print(json.dumps(manifest, ensure_ascii=False, indent=2))
        return 0

    from pydantic_ai.usage import UsageLimits

    out_dir.mkdir(parents=True, exist_ok=False)
    manifest_path = out_dir / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )

    for index, (letter, seed) in enumerate(zip(letters, seeds), start=1):
        writer = make_writer(cfg, reader_manifest, seed=seed)
        result = writer.run_sync(
            final_prompt,
            message_history=build_history(study, letter, str(reader_manifest["model"])),
            usage_limits=UsageLimits(request_limit=int(exp["request_limit"])),
        )
        candidate = _study_body(result.output).strip() + "\n"
        if not candidate.lstrip().startswith(f"# {character_name}"):
            raise SystemExit(
                f"candidate {index} missing heading; completed earlier outputs are preserved"
            )
        filename = f"candidate-{index}.md"
        (out_dir / filename).write_text(candidate, encoding="utf-8")
        manifest["outputs"].append(filename)
        manifest_path.write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        print(f"candidate {index}/{len(letters)} -> {out_dir / filename}")

    print(f"manifest -> {manifest_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
