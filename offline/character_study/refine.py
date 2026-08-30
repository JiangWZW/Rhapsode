"""One Pro + thinking pass over the cited Flash study. Same three nav tools."""

from __future__ import annotations

import os
import sys

from pydantic_ai import Agent, RunContext
from pydantic_ai.models.openai import OpenAIChatModel
from pydantic_ai.providers.deepseek import DeepSeekProvider
from pydantic_ai.settings import ModelSettings
from pydantic_ai.usage import UsageLimits

from corpus import ROOT, load_config, load_lines, load_local_env, load_volumes
from critic import Nav, _study_body, load_sections
from nav import format_brief, format_lines, format_sources

SYSTEM = """You finish the character study of {name}.

This page will be used by a narrator who has to speak as them, act as them, and think as them on a live turn. The study has to be complete enough to do that in a scene that is not in the notes. A trait with their name on it is not the person. A recap of scenes is not the person.

Write one study of the whole person, the way an actor or a writer would keep it: comprehensive, collected, and deep.

Comprehensive means the person as they live in the books. How they talk. What they do when they speak and when they do not. How they are with each person around them. How they carry themselves. What a line of theirs sounds like — quote a little, so the page can be heard.

Collected means the person those scenes share. The notes are a walk through the novels. The study is the one figure who is in all of them.

Deep means psychological analysis of the inner life. Who they believe they are. What is going on in them that they know, and what is going on that they do not. Desire, shame, the person they are trying to be, the wish they will not name. How that mind is heard in speech and seen in the body. What a line or an action means from inside them. Go into that mind. The surface of the books is not enough.

The notes below are first-pass material from walking the novels, with line pointers. Open the cited lines. Open nearby lines when a clip starts mid-scene. Open a volume brief when you need the plot around a scene. The analysis stands on those pages. Put (vN Lstart-end) on what you keep.

Plain sentences. Start at the # {name} heading.
"""


def make_refine_agent(cfg: dict) -> Agent[Nav, str]:
    llm = cfg["refine"]
    key = os.environ.get(cfg["critic"]["api_key_env"], "")
    if not key:
        raise SystemExit(f"missing env {cfg['critic']['api_key_env']}")
    name = str(cfg.get("character") or "Darkness")
    model = OpenAIChatModel(
        llm["model"],
        provider=DeepSeekProvider(api_key=key),
    )
    agent: Agent[Nav, str] = Agent(
        model,
        deps_type=Nav,
        system_prompt=SYSTEM.format(name=name),
        model_settings=ModelSettings(
            temperature=0.2,
            max_tokens=int(llm["max_tokens"]),
            thinking=True,
            extra_body={"thinking": {"type": "enabled"}},
        ),
    )

    @agent.tool
    def list_sources(ctx: RunContext[Nav]) -> str:
        """Volume ids, line ranges, and nearby section ids. Metadata only."""
        return format_sources(load_volumes(), ctx.deps.sections, ctx.deps.current_id)

    @agent.tool
    def read_lines(ctx: RunContext[Nav], start: int, end: int) -> str:
        """Read contiguous novel lines (1-based). At most 40 lines."""
        return format_lines(ctx.deps.lines, start, end, ctx.deps.cap)

    @agent.tool
    def read_brief(ctx: RunContext[Nav], volume: int) -> str:
        """Human plot paragraph for one volume."""
        return format_brief(volume)

    return agent


def main() -> int:
    load_local_env()
    cfg = load_config()
    if "refine" not in cfg:
        raise SystemExit("config.yaml missing refine:")
    study_path = ROOT / cfg["paths"]["study"]
    cited = study_path.with_name(study_path.stem + ".cited.md")
    if not cited.is_file():
        raise SystemExit(f"run cite.py first ({cited} missing)")
    draft = cited.read_text(encoding="utf-8")
    flash = study_path.with_name(study_path.stem + ".flash.md")
    if not flash.is_file():
        flash.write_text(draft, encoding="utf-8")

    lines = load_lines(cfg)
    sections = load_sections(ROOT / cfg["paths"]["sections"])
    vol2 = [s for s in sections if s.get("volume", 0) <= 2]
    current_id = vol2[-1]["id"] if vol2 else 1
    name = str(cfg.get("character") or "Darkness")

    result = make_refine_agent(cfg).run_sync(
        "The notes, then write the study. When you have what you need, write it.\n\n"
        f"CURRENT STUDY:\n{draft}\n",
        deps=Nav(
            lines=lines,
            sections=sections,
            current_id=current_id,
            cap=int(cfg["critic"]["read_lines_cap"]),
        ),
        usage_limits=UsageLimits(request_limit=int(cfg["refine"]["max_turns"])),
    )
    out = _study_body(result.output) + "\n"
    if not out.lstrip().startswith(f"# {name}"):
        raise SystemExit("refine output missing heading; study not overwritten")
    ck = ROOT / cfg["paths"]["checkpoints"] / "refine.md"
    ck.parent.mkdir(parents=True, exist_ok=True)
    ck.write_text(out, encoding="utf-8")
    study_path.write_text(out, encoding="utf-8")
    print(f"refine -> {study_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
