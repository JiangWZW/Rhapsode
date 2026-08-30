"""Attach (vN Lline) after quotes that match the novel. Does not rewrite prose."""

from __future__ import annotations

import re
import sys

from corpus import ROOT, load_config, load_lines, load_volumes, volume_for_line

_QUOTE = re.compile(r"[“\"]([^”\"]{12,180})[”\"]")
_ALREADY = re.compile(r"\(v\d+\s+L\d+")


def _norm(s: str) -> str:
    return " ".join(s.replace("…", "...").replace("’", "'").split())


def find_line(quote: str, lines: list[str]) -> int | None:
    q = _norm(quote)
    if len(q) < 12:
        return None
    hits = [i for i, line in enumerate(lines, start=1) if q in _norm(line)]
    if len(hits) == 1:
        return hits[0]
    if len(hits) == 0 and len(q) >= 24:
        head = q[:40]
        heads = [i for i, line in enumerate(lines, start=1) if head in _norm(line)]
        if len(heads) == 1:
            return heads[0]
    return None


def cite_text(text: str, lines: list[str], volumes: list[dict]) -> str:
    def repl(m: re.Match[str]) -> str:
        raw = m.group(0)
        after = text[m.end() : m.end() + 20]
        if _ALREADY.match(after.lstrip()):
            return raw
        loc = find_line(m.group(1), lines)
        if loc is None:
            return raw
        vol = volume_for_line(loc, volumes)
        return f"{raw} (v{vol} L{loc})"

    return _QUOTE.sub(repl, text)


def main() -> int:
    cfg = load_config()
    src = ROOT / cfg["paths"]["study"]
    if not src.is_file():
        raise SystemExit(f"missing {src}")
    out = src.with_name(src.stem + ".cited.md")
    text = cite_text(src.read_text(encoding="utf-8"), load_lines(cfg), load_volumes(cfg))
    out.write_text(text, encoding="utf-8")
    print(f"cited -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
