"""Update the Darkness study one section at a time. Three nav tools."""

from __future__ import annotations

import argparse
import json
import os
import sys
from dataclasses import dataclass

from pydantic_ai import Agent, RunContext
from pydantic_ai.models.openai import OpenAIChatModel
from pydantic_ai.providers.deepseek import DeepSeekProvider
from pydantic_ai.providers.openai import OpenAIProvider
from pydantic_ai.settings import ModelSettings
from pydantic_ai.usage import UsageLimits

from corpus import ROOT, load_config, load_lines, load_local_env, load_volumes
from nav import format_brief, format_lines, format_sources

SYSTEM = """You are an experienced writer that is analysing this character {char_name} in {book_name}.
This is the first pass, where you scan through volume {vol_beg} to {vol_end}, and read the content related to the character, study the character in depth, and write your analysis, for example:
the character's identity/background;
physical traits;
experiences and consequential choices;
directional relationships;
goals, fears and deep conflicts;
knowledge, beliefs, secrets and uncertainties;
personality hypothese supported by behavior in the book;
dialogue and interation styles;
Note that the analysis is way more than the things I noted above  - a character is a human being and can be very deep and complicated and that's why characters with depth are so attractive.
The analysis MUST be reliable and backed by the existing text, do NOT invent/over-think of non-existing traits or descriptions.

The section is not the whole story. Read related text until you understand what is happening in the book — the volume brief, lines before and after this section, and any other stretch you need. Then write the study.
When you add or keep a claim, attach a text pointer: volume and novel line range, like (v1 L1402-1431). Quotes get the same pointer. Keep pointers already in the study.
Then return the full updated study as markdown. Start at the # {char_name} heading. No process notes.
"""


@dataclass
class Nav:
    lines: list[str]
    sections: list[dict]
    current_id: int
    cap: int


def make_agent(cfg: dict, *, vol_beg: int, vol_end: int) -> Agent[Nav, str]:
    llm = cfg["critic"]
    key = os.environ.get(llm["api_key_env"], "")
    if not key:
        raise SystemExit(f"missing env {llm['api_key_env']}")
    name = str(cfg.get("character") or "Darkness")
    book = str(cfg.get("book") or "KonoSuba")
    base_name = str(llm.get("api_base_env") or "").strip()
    base_url = os.environ.get(base_name, "").strip() if base_name else ""
    provider = (
        OpenAIProvider(base_url=base_url, api_key=key)
        if base_url
        else DeepSeekProvider(api_key=key)
    )
    model = OpenAIChatModel(llm["model"], provider=provider)
    settings: dict = {"temperature": 0.9}
    if llm.get("max_tokens"):
        settings["max_tokens"] = int(llm["max_tokens"])
    if llm.get("thinking"):
        settings["thinking"] = True
        settings["extra_body"] = {"thinking": {"type": "enabled"}}
    agent: Agent[Nav, str] = Agent(
        model,
        deps_type=Nav,
        system_prompt=SYSTEM.format(
            char_name=name,
            book_name=book,
            vol_beg=vol_beg,
            vol_end=vol_end,
        ),
        model_settings=ModelSettings(**settings),
    )

    @agent.tool
    def list_sources(ctx: RunContext[Nav]) -> str:
        """Volume ids, line ranges, and nearby section ids. Metadata only."""
        return format_sources(load_volumes(), ctx.deps.sections, ctx.deps.current_id)

    @agent.tool
    def read_lines(ctx: RunContext[Nav], start: int, end: int) -> str:
        """Read contiguous novel lines (1-based). Capped; call again for the next span."""
        return format_lines(ctx.deps.lines, start, end, ctx.deps.cap)

    @agent.tool
    def read_brief(ctx: RunContext[Nav], volume: int) -> str:
        """Human plot paragraph for one volume."""
        return format_brief(volume)

    return agent


def _study_body(text: str) -> str:
    text = text.strip()
    if text.startswith("#"):
        return text
    at = text.find("\n# ")
    if at != -1:
        return text[at + 1 :].strip()
    return text


def load_sections(path) -> list[dict]:
    rows = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.strip():
                rows.append(json.loads(line))
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description="Critic pass over character sections")
    parser.add_argument("--config", default="config.yaml")
    parser.add_argument("--max-sections", type=int, default=0, help="0 = all")
    parser.add_argument("--from-id", type=int, default=1)
    parser.add_argument("--until-volume", type=int, default=0, help="0 = no volume cap")
    args = parser.parse_args()

    load_local_env()
    cfg = load_config(args.config)
    lines = load_lines(cfg)
    sec_path = ROOT / cfg["paths"]["sections"]
    if not sec_path.is_file():
        raise SystemExit(f"run extract.py first ({sec_path} missing)")
    sections = load_sections(sec_path)

    study_path = ROOT / cfg["paths"]["study"]
    ck_dir = ROOT / cfg["paths"]["checkpoints"]
    ck_dir.mkdir(parents=True, exist_ok=True)
    study_path.parent.mkdir(parents=True, exist_ok=True)
    name = str(cfg.get("character") or "Darkness")
    study = (
        study_path.read_text(encoding="utf-8")
        if study_path.is_file()
        else f"# {name}\n\n(empty)\n"
    )

    cap = int(cfg["critic"]["read_lines_cap"])
    limits = UsageLimits(request_limit=int(cfg["critic"]["max_turns"]))

    todo = [s for s in sections if s["id"] >= args.from_id]
    if args.until_volume:
        todo = [s for s in todo if s["volume"] <= args.until_volume]
    if args.max_sections:
        todo = todo[: args.max_sections]
    vols = [s["volume"] for s in todo]
    vol_beg, vol_end = (min(vols), max(vols)) if vols else (1, 1)
    agent = make_agent(cfg, vol_beg=vol_beg, vol_end=vol_end)

    for sec in todo:
        prompt = (
            f"Volume {sec['volume']}. Section {sec['id']} lines {sec['line_start']}-{sec['line_end']}.\n\n"
            f"CURRENT STUDY:\n{study}\n\n"
            f"SECTION:\n{sec['text']}\n"
        )
        result = agent.run_sync(
            prompt,
            deps=Nav(lines=lines, sections=sections, current_id=sec["id"], cap=cap),
            usage_limits=limits,
        )
        study = _study_body(result.output) + "\n"
        ck = ck_dir / f"{sec['id']:04d}.md"
        ck.write_text(study, encoding="utf-8")
        study_path.write_text(study, encoding="utf-8")
        print(f"section {sec['id']} vol={sec['volume']} -> {ck}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
