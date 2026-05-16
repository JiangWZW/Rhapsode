"""
Rhapsode Wiki Lint Tool

Performs health checks defined in SCHEMA.md §12.5:
  - Frontmatter validation (required fields, valid values)
  - Broken wiki-links
  - Orphan pages (no inbound links)
  - Readability checks (sentence length, passive voice, paragraph/section size)
  - Page health scoring (patch scar density, temporal layers, readability, etc.)

Usage:
    python wiki_lint.py [--fix] [--no-readability] [--health] [--health-threshold T] [page_slug]

    --fix              Auto-fix what can be fixed (staleness callouts)
    --no-readability   Skip readability checks
    --health           Run page health scoring (Tier 1 + Tier 2 quality signals)
    --health-threshold Score below which pages are flagged (default: 3.0)
"""

from __future__ import annotations

import argparse
import re
import sys
from datetime import date
from pathlib import Path

import yaml

WIKI_ROOT = Path(__file__).resolve().parent.parent
CONTENT_DIRS = ["concepts", "architecture", "decisions", "research", "talemate"]
REQUIRED_FIELDS = ["sources", "last_updated", "confidence", "tier", "related", "tags"]
VALID_CONFIDENCE = {"verified", "likely"}
VALID_TIERS = {"working", "episodic", "semantic", "procedural"}
VALID_TAGS = {"cpp-core", "python-server", "vue-frontend", "cross-layer", "design",
              "third-party-analysis", "research", "memory-architecture"}

WIKI_LINK_RE = re.compile(r"\[\[([^\]|]+?)(?:\|[^\]]+?)?\]\]")


def iter_pages() -> list[Path]:
    pages = []
    for d in CONTENT_DIRS:
        dirpath = WIKI_ROOT / d
        if dirpath.is_dir():
            pages.extend(dirpath.glob("*.md"))
    return sorted(pages)


def parse_frontmatter(path: Path) -> tuple[dict | None, int, int]:
    try:
        text = path.read_text(encoding="utf-8")
    except Exception:
        return None, -1, -1
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")
    if not lines or lines[0].strip() != "---":
        return None, -1, -1
    end = -1
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            end = i
            break
    if end == -1:
        return None, -1, -1
    yaml_block = "\n".join(lines[1:end])
    try:
        fm = yaml.safe_load(yaml_block) or {}
    except yaml.YAMLError:
        return None, 0, end
    return fm, 0, end


def page_slug(path: Path) -> str:
    return path.stem


def collect_all_slugs(pages: list[Path]) -> set[str]:
    return {page_slug(p) for p in pages}


def extract_outbound_links(path: Path) -> list[str]:
    text = path.read_text(encoding="utf-8")
    raw = WIKI_LINK_RE.findall(text)
    slugs = []
    for link in raw:
        link = link.strip()
        if "#" in link:
            link = link.split("#")[0]
        if "/" in link:
            link = link.split("/")[-1]
        if link:
            slugs.append(link)
    return slugs


# ── Readability checks ───────────────────────────────────────────────

_REF_LINK_RE = re.compile(r"^\[.+\]:\s+\S")
_SENT_SPLIT_RE = re.compile(r'(?<=[.?!])\s+(?=[A-Z"(])')
_PASSIVE_RE = re.compile(r"\b(is|are|was|were|been|being)\s+(\w+ed)\b", re.IGNORECASE)

_MAX_SENTENCE_WORDS = 26
_MAX_PARAGRAPH_SENTENCES = 5
_MAX_PARAGRAPH_WORDS = 120
_MAX_SECTION_PROSE_WORDS = 300
_PASSIVE_WARN_RATIO = 0.25


def _extract_prose_blocks(page: Path) -> list[tuple[int, str]]:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    result: list[tuple[int, str]] = []
    in_fm = False
    in_code = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        if i == 0 and stripped == "---":
            in_fm = True
            continue
        if in_fm:
            if stripped == "---":
                in_fm = False
            continue

        if stripped.startswith("```"):
            in_code = not in_code
            continue
        if in_code:
            continue

        if (not stripped
                or stripped.startswith("#")
                or stripped.startswith("|")
                or stripped.startswith(">")
                or stripped.startswith("- [[")
                or _REF_LINK_RE.match(stripped)):
            continue

        result.append((i + 1, line))

    return result


def _count_words(text: str) -> int:
    collapsed = re.sub(r"`[^`]+`", "CODETOKEN", text)
    return len(collapsed.split())


def _split_sentences(text: str) -> list[str]:
    parts = _SENT_SPLIT_RE.split(text)
    return [s.strip() for s in parts if s.strip()]


def check_sentence_length(page: Path) -> list[tuple[int, str]]:
    prose_blocks = _extract_prose_blocks(page)
    issues: list[tuple[int, str]] = []

    for line_num, line in prose_blocks:
        sentences = _split_sentences(line)
        for sent in sentences:
            wc = _count_words(sent)
            if wc > _MAX_SENTENCE_WORDS:
                preview = " ".join(sent.split()[:8]) + "..."
                issues.append((line_num, f"sentence has {wc} words (max {_MAX_SENTENCE_WORDS}): \"{preview}\""))

    return issues


def check_passive_voice(page: Path) -> tuple[float, list[tuple[int, str]]]:
    prose_blocks = _extract_prose_blocks(page)
    total_sentences = 0
    passive_count = 0
    examples: list[tuple[int, str]] = []

    for line_num, line in prose_blocks:
        sentences = _split_sentences(line)
        for sent in sentences:
            total_sentences += 1
            if _PASSIVE_RE.search(sent):
                passive_count += 1
                if len(examples) < 3:
                    preview = sent[:80] + ("..." if len(sent) > 80 else "")
                    examples.append((line_num, preview))

    ratio = passive_count / total_sentences if total_sentences > 0 else 0.0
    return ratio, examples


def check_paragraph_length(page: Path) -> list[tuple[int, str]]:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    issues: list[tuple[int, str]] = []
    in_fm = False
    in_code = False
    para_start = -1
    para_lines: list[str] = []

    def _flush():
        nonlocal para_start, para_lines
        if not para_lines or para_start < 0:
            para_lines = []
            return
        combined = " ".join(para_lines)
        wc = _count_words(combined)
        sc = sum(len(_split_sentences(l)) for l in para_lines)
        if sc > _MAX_PARAGRAPH_SENTENCES:
            issues.append((para_start, f"paragraph has {sc} sentences (max {_MAX_PARAGRAPH_SENTENCES})"))
        elif wc > _MAX_PARAGRAPH_WORDS:
            issues.append((para_start, f"paragraph has {wc} words (max {_MAX_PARAGRAPH_WORDS})"))
        para_lines = []

    for i, line in enumerate(lines):
        stripped = line.strip()

        if i == 0 and stripped == "---":
            in_fm = True
            continue
        if in_fm:
            if stripped == "---":
                in_fm = False
            continue

        if stripped.startswith("```"):
            _flush()
            in_code = not in_code
            continue
        if in_code:
            continue

        is_numbered_list = len(stripped) > 2 and stripped[0].isdigit() and ". " in stripped[:5]
        if (not stripped
                or stripped.startswith("#")
                or stripped.startswith("|")
                or stripped.startswith(">")
                or stripped.startswith("- ")
                or is_numbered_list
                or _REF_LINK_RE.match(stripped)):
            _flush()
            continue

        if not para_lines:
            para_start = i + 1
        para_lines.append(stripped)

    _flush()
    return issues


def check_section_length(page: Path) -> list[tuple[int, str]]:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    issues: list[tuple[int, str]] = []
    in_fm = False
    in_code = False

    section_start = -1
    section_heading = ""
    prose_words = 0
    has_break = False

    def _flush_section():
        nonlocal prose_words, has_break, section_start, section_heading
        if section_start > 0 and prose_words > _MAX_SECTION_PROSE_WORDS and not has_break:
            issues.append((
                section_start,
                f"section \"{section_heading}\" has {prose_words} prose words without structural breaks (max {_MAX_SECTION_PROSE_WORDS})"
            ))
        prose_words = 0
        has_break = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        if i == 0 and stripped == "---":
            in_fm = True
            continue
        if in_fm:
            if stripped == "---":
                in_fm = False
            continue

        if stripped.startswith("```"):
            in_code = not in_code
            if in_code:
                has_break = True
            continue
        if in_code:
            continue

        if stripped.startswith("#"):
            _flush_section()
            section_start = i + 1
            section_heading = stripped.lstrip("#").strip()[:50]
            continue

        if (stripped.startswith("|") or stripped.startswith("- ") or stripped.startswith("1. ")
                or _REF_LINK_RE.match(stripped)):
            has_break = True
            continue

        if not stripped:
            continue

        prose_words += _count_words(stripped)

    _flush_section()
    return issues


# ── Page health scoring ──────────────────────────────────────────────

PATCH_SCAR_PATTERNS = [
    re.compile(r"(?:^|\s)Note:\s", re.IGNORECASE),
    re.compile(r"(?:^|\s)Update:\s", re.IGNORECASE),
    re.compile(r"Current status:", re.IGNORECASE),
    re.compile(r"Previously,", re.IGNORECASE),
    re.compile(r"was later changed", re.IGNORECASE),
    re.compile(r"however,?\s+this was reverted", re.IGNORECASE),
    re.compile(r"has since been", re.IGNORECASE),
    re.compile(r"no longer (?:used|active|valid|true)", re.IGNORECASE),
    re.compile(r"this (?:has|was) (?:been )?(?:removed|deprecated|replaced)", re.IGNORECASE),
]

CORRECTION_CALLOUT_RE = re.compile(r"^>\s*\[!(?:note|info|tip|caution)\]", re.IGNORECASE)
STALENESS_CALLOUT_RE = re.compile(r"^>\s*\[!warning\]\s*This page has not been verified")

DATE_IN_PROSE_RE = re.compile(r"\b20\d{2}-\d{2}-\d{2}\b")
TEMPORAL_WORDS = re.compile(
    r"\b(?:originally|previously|formerly|now|currently|as of|at the time|"
    r"was once|used to|no longer|since then|later changed|has been updated)\b",
    re.IGNORECASE,
)

LONG_PAREN_RE = re.compile(r"\([^)]{30,}\)")


def _get_prose_lines(page: Path) -> list[str]:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    prose: list[str] = []
    in_frontmatter = False
    in_code_block = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        if i == 0 and stripped == "---":
            in_frontmatter = True
            continue
        if in_frontmatter:
            if stripped == "---":
                in_frontmatter = False
            continue

        if stripped.startswith("```"):
            in_code_block = not in_code_block
            continue
        if in_code_block:
            continue

        if stripped.startswith("#") or stripped.startswith("|") or not stripped:
            continue
        if stripped.startswith("- [["):
            continue
        if _REF_LINK_RE.match(stripped):
            continue

        prose.append(line)

    return prose


def _count_blockquote_lines(page: Path) -> tuple[int, int]:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    bq_lines = 0
    content_lines = 0
    in_fm = False
    in_code = False

    for i, line in enumerate(lines):
        stripped = line.strip()
        if i == 0 and stripped == "---":
            in_fm = True
            continue
        if in_fm:
            if stripped == "---":
                in_fm = False
            continue
        if stripped.startswith("```"):
            in_code = not in_code
            continue
        if in_code or not stripped or stripped.startswith("#") or stripped.startswith("|"):
            continue

        content_lines += 1
        if stripped.startswith(">"):
            if not STALENESS_CALLOUT_RE.match(stripped):
                bq_lines += 1

    return bq_lines, content_lines


APPENDIX_HEADINGS = re.compile(
    r"^##\s+(?:additional\s+notes|updates?|changes?|addendum|current\s+status|"
    r"historical?\s+(?:notes?|context)|errata|patches?|fixe?s?\b)",
    re.IGNORECASE,
)


def _detect_appendix_sections(page: Path) -> int:
    text = page.read_text(encoding="utf-8")
    if text.startswith("\ufeff"):
        text = text[1:]
    lines = text.split("\n")

    structural = {"see also", "history", "references", "notes", "related", "links"}
    headings: list[str] = []
    for line in lines:
        stripped = line.strip()
        if stripped.startswith("## "):
            headings.append(stripped)

    if len(headings) < 3:
        return 0

    tail = []
    for h in reversed(headings):
        h_text = h.lstrip("#").strip().lower()
        if h_text in structural:
            continue
        tail.append(h)
        if len(tail) >= 3:
            break

    appendix_count = 0
    for h in tail:
        if APPENDIX_HEADINGS.match(h):
            appendix_count += 1

    return min(appendix_count, 2)


def load_supersessions() -> list[dict]:
    ss_path = WIKI_ROOT / "supersessions.yaml"
    if not ss_path.exists():
        return []
    try:
        data = yaml.safe_load(ss_path.read_text(encoding="utf-8"))
    except Exception:
        return []
    return data if isinstance(data, list) else []


def score_page_health(page: Path, supersessions: list[dict]) -> dict:
    slug = page_slug(page)
    prose = _get_prose_lines(page)
    prose_text = "\n".join(prose)
    prose_count = len(prose) if prose else 1

    scar_count = 0
    for line in prose:
        for pat in PATCH_SCAR_PATTERNS:
            if pat.search(line):
                scar_count += 1
                break
        if CORRECTION_CALLOUT_RE.match(line.strip()):
            scar_count += 1

    temporal_count = 0
    for line in prose:
        temporal_count += len(DATE_IN_PROSE_RE.findall(line))
        temporal_count += len(TEMPORAL_WORDS.findall(line))

    bq_lines, content_lines = _count_blockquote_lines(page)
    bq_ratio = bq_lines / content_lines if content_lines > 0 else 0

    paren_count = len(LONG_PAREN_RE.findall(prose_text))
    paren_per_100 = (paren_count / prose_count) * 100 if prose_count > 0 else 0

    ss_count = sum(1 for s in supersessions if s.get("page") == slug)

    appendix_count = _detect_appendix_sections(page)

    long_sentences = check_sentence_length(page)
    long_sent_count = len(long_sentences)
    passive_ratio, _ = check_passive_voice(page)
    dense_paragraphs = check_paragraph_length(page)
    dense_para_count = len(dense_paragraphs)
    wall_sections = check_section_length(page)
    wall_section_count = len(wall_sections)

    score = 5.0
    scar_penalty = min(scar_count * 0.5, 2.0)
    temporal_penalty = min(temporal_count * 0.3, 1.5)
    bq_penalty = 1.0 if bq_ratio > 0.15 else 0.0
    paren_penalty = 0.5 if paren_per_100 > 5 else 0.0
    ss_penalty = min(ss_count * 0.3, 1.5)
    appendix_penalty = appendix_count * 0.5

    long_sent_penalty = min(long_sent_count * 0.1, 1.5)
    if passive_ratio > 0.40:
        passive_penalty = 1.0
    elif passive_ratio > _PASSIVE_WARN_RATIO:
        passive_penalty = 0.5
    else:
        passive_penalty = 0.0
    dense_para_penalty = min(dense_para_count * 0.2, 1.0)
    wall_section_penalty = min(wall_section_count * 0.3, 1.0)

    score -= scar_penalty
    score -= temporal_penalty
    score -= bq_penalty
    score -= paren_penalty
    score -= ss_penalty
    score -= appendix_penalty
    score -= long_sent_penalty
    score -= passive_penalty
    score -= dense_para_penalty
    score -= wall_section_penalty

    score = max(1.0, min(5.0, score))

    return {
        "slug": slug,
        "score": round(score, 1),
        "patch_scars": scar_count,
        "temporal_layers": temporal_count,
        "bq_ratio": round(bq_ratio, 2),
        "parentheticals": paren_count,
        "supersessions": ss_count,
        "appendix_sections": appendix_count,
        "long_sentences": long_sent_count,
        "passive_ratio": round(passive_ratio, 2),
        "dense_paragraphs": dense_para_count,
        "wall_sections": wall_section_count,
        "penalties": {
            "scars": round(scar_penalty, 1),
            "temporal": round(temporal_penalty, 1),
            "blockquote": round(bq_penalty, 1),
            "parens": round(paren_penalty, 1),
            "supersessions": round(ss_penalty, 1),
            "appendix": round(appendix_penalty, 1),
            "long_sent": round(long_sent_penalty, 1),
            "passive": round(passive_penalty, 1),
            "dense_para": round(dense_para_penalty, 1),
            "wall_section": round(wall_section_penalty, 1),
        },
    }


# ── Tier 2: Cross-page consistency ──────────────────────────────────

SYMBOL_STATUS_RE = re.compile(
    r"`([A-Z][A-Za-z0-9_]{3,})`"
    r"\s+(?:is|was|has been|are|were)\s+"
    r"(active|removed|deprecated|bypassed|disabled|enabled|replaced|renamed|deleted|"
    r"commented out|no longer used|currently active|still active|still used)",
    re.IGNORECASE,
)

CORRECTIVE_RE = re.compile(
    r"(?:Note|Update|Warning|NB|Important):\s*(.{20,120})",
    re.IGNORECASE,
)


def check_cross_page_consistency(pages: list[Path]) -> tuple[list[str], list[str]]:
    symbol_claims: dict[str, list[tuple[str, str, int]]] = {}
    corrective_texts: dict[str, list[str]] = {}

    for page in pages:
        slug = page_slug(page)
        text = page.read_text(encoding="utf-8")
        if text.startswith("\ufeff"):
            text = text[1:]
        lines = text.split("\n")

        in_fm = False
        in_code = False
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()
            if line_num == 1 and stripped == "---":
                in_fm = True
                continue
            if in_fm:
                if stripped == "---":
                    in_fm = False
                continue
            if stripped.startswith("```"):
                in_code = not in_code
                continue
            if in_code:
                continue

            for m in SYMBOL_STATUS_RE.finditer(line):
                sym = m.group(1)
                status = m.group(2).lower().strip()
                if sym not in symbol_claims:
                    symbol_claims[sym] = []
                symbol_claims[sym].append((slug, status, line_num))

            m = CORRECTIVE_RE.search(line)
            if m:
                normalized = re.sub(r"\s+", " ", m.group(1).strip().lower())
                if len(normalized) >= 20:
                    if normalized not in corrective_texts:
                        corrective_texts[normalized] = []
                    if slug not in corrective_texts[normalized]:
                        corrective_texts[normalized].append(slug)

    ACTIVE_STATUSES = {"active", "enabled", "currently active", "still active", "still used"}
    INACTIVE_STATUSES = {"removed", "deprecated", "bypassed", "disabled", "replaced",
                         "renamed", "deleted", "commented out", "no longer used"}

    contradictions: list[str] = []
    for sym, claims in symbol_claims.items():
        has_active = any(s in ACTIVE_STATUSES for _, s, _ in claims)
        has_inactive = any(s in INACTIVE_STATUSES for _, s, _ in claims)
        if has_active and has_inactive:
            active_pages = [f"{sl} (line {ln})" for sl, s, ln in claims if s in ACTIVE_STATUSES]
            inactive_pages = [f"{sl} (line {ln})" for sl, s, ln in claims if s in INACTIVE_STATUSES]
            contradictions.append(
                f"`{sym}`: described as active in {', '.join(active_pages)} "
                f"but inactive in {', '.join(inactive_pages)}"
            )

    echoes: list[str] = []
    for text_frag, slugs in corrective_texts.items():
        if len(slugs) >= 3:
            echoes.append(
                f"Corrective text echoed in {len(slugs)} pages ({', '.join(slugs)}): "
                f"\"{text_frag[:60]}...\""
            )

    return contradictions, echoes


def run_health_scoring(pages: list[Path], threshold: float = 3.0,
                       page_filter: str | None = None) -> list[dict]:
    supersessions = load_supersessions()

    target = resolve_page_filter(page_filter)
    if page_filter and not target:
        return []
    score_pages = [target] if target else pages

    results = []
    for page in score_pages:
        result = score_page_health(page, supersessions)
        results.append(result)

    if not page_filter:
        contradictions, echoes = check_cross_page_consistency(pages)

        contradiction_pages: set[str] = set()
        for c in contradictions:
            for r in results:
                if r["slug"] in c:
                    contradiction_pages.add(r["slug"])

        echo_pages: set[str] = set()
        for e in echoes:
            for r in results:
                if r["slug"] in e:
                    echo_pages.add(r["slug"])

        for r in results:
            t2_penalty = 0.0
            if r["slug"] in contradiction_pages:
                t2_penalty += 1.0
                r["penalties"]["contradiction"] = 1.0
            if r["slug"] in echo_pages:
                t2_penalty += 0.5
                r["penalties"]["echo"] = 0.5
            if t2_penalty > 0:
                r["score"] = max(1.0, round(r["score"] - t2_penalty, 1))

        results.append({"_cross_page": True, "contradictions": contradictions, "echoes": echoes})

    return results


def print_health_report(results: list[dict], threshold: float = 3.0) -> int:
    cross_page = None
    page_results = []
    for r in results:
        if r.get("_cross_page"):
            cross_page = r
        else:
            page_results.append(r)

    page_results.sort(key=lambda r: r["score"])

    print(f"\n{'='*70}")
    print(f"  PAGE HEALTH SCORES")
    print(f"{'='*70}")
    print(f"  {'Page':<40} {'Score':>5}  {'Status':<16} {'Top penalty'}")
    print(f"  {'-'*40} {'-'*5}  {'-'*16} {'-'*25}")

    below_count = 0
    for r in page_results:
        score = r["score"]
        if score >= 4.0:
            status = "OK"
        elif score >= threshold:
            status = "CAUTION"
        else:
            status = "REWRITE NEEDED"
            below_count += 1

        penalties = r.get("penalties", {})
        top_penalty = ""
        if penalties:
            top_key = max(penalties, key=penalties.get)
            top_val = penalties[top_key]
            if top_val > 0:
                top_penalty = f"-{top_val} ({top_key})"

        name = f"{r['slug']}.md"
        print(f"  {name:<40} {score:>5.1f}  {status:<16} {top_penalty}")

    print(f"  {'-'*70}")
    avg = sum(r["score"] for r in page_results) / len(page_results) if page_results else 0
    print(f"  Average: {avg:.1f}   Below threshold ({threshold}): {below_count}")

    if cross_page:
        if cross_page["contradictions"]:
            print(f"\n  CROSS-PAGE CONTRADICTIONS ({len(cross_page['contradictions'])})")
            print(f"  {'-'*60}")
            for c in cross_page["contradictions"]:
                print(f"    {c}")
        if cross_page["echoes"]:
            print(f"\n  ECHO DUPLICATION ({len(cross_page['echoes'])})")
            print(f"  {'-'*60}")
            for e in cross_page["echoes"]:
                print(f"    {e}")

    print(f"{'='*70}")
    return below_count


class LintReport:
    def __init__(self):
        self.errors: list[str] = []
        self.warnings: list[str] = []
        self.fixes: list[str] = []

    def error(self, msg: str):
        self.errors.append(msg)

    def warn(self, msg: str):
        self.warnings.append(msg)

    def fix(self, msg: str):
        self.fixes.append(msg)

    def print_report(self):
        if self.fixes:
            print(f"\n{'='*60}")
            print(f"  FIXES APPLIED ({len(self.fixes)})")
            print(f"{'='*60}")
            for f in self.fixes:
                print(f"  [FIX] {f}")

        if self.errors:
            print(f"\n{'='*60}")
            print(f"  ERRORS ({len(self.errors)})")
            print(f"{'='*60}")
            for e in self.errors:
                print(f"  [ERR] {e}")

        if self.warnings:
            print(f"\n{'='*60}")
            print(f"  WARNINGS ({len(self.warnings)})")
            print(f"{'='*60}")
            for w in self.warnings:
                safe = w.encode("ascii", "replace").decode("ascii")
                print(f"  [WARN] {safe}")

        print(f"\n{'='*60}")
        print(f"  SUMMARY: {len(self.errors)} errors, {len(self.warnings)} warnings, {len(self.fixes)} fixes")
        print(f"{'='*60}")

    @property
    def ok(self) -> bool:
        return len(self.errors) == 0


def resolve_page_filter(filter_slug: str | None) -> Path | None:
    if not filter_slug:
        return None
    filter_slug = filter_slug.replace(".md", "")
    for d in CONTENT_DIRS:
        candidate = WIKI_ROOT / d / f"{filter_slug}.md"
        if candidate.exists():
            return candidate
    return None


def lint(page_filter: str | None = None, readability_check: bool = True) -> LintReport:
    report = LintReport()
    pages = iter_pages()
    all_slugs = collect_all_slugs(pages)

    target_path = resolve_page_filter(page_filter)
    if page_filter and not target_path:
        report.error(f"Page '{page_filter}' not found in wiki")
        return report

    inbound: dict[str, list[str]] = {s: [] for s in all_slugs}
    lint_pages = [target_path] if target_path else pages

    for page in lint_pages:
        slug = page_slug(page)
        relpath = page.relative_to(WIKI_ROOT)

        fm, _, _ = parse_frontmatter(page)
        if fm is None:
            report.error(f"{relpath}: missing or unparseable YAML frontmatter")
            continue

        for field in REQUIRED_FIELDS:
            if field not in fm:
                report.error(f"{relpath}: missing required field '{field}'")

        if "confidence" in fm and fm["confidence"] not in VALID_CONFIDENCE:
            report.error(f"{relpath}: invalid confidence '{fm['confidence']}' (must be verified|likely)")

        if "tier" in fm and fm["tier"] not in VALID_TIERS:
            report.error(f"{relpath}: invalid tier '{fm['tier']}' (must be working|episodic|semantic|procedural)")

        if "tags" in fm and isinstance(fm["tags"], list):
            for tag in fm["tags"]:
                if tag not in VALID_TAGS:
                    report.error(f"{relpath}: invalid tag '{tag}' (must be one of {VALID_TAGS})")

        outlinks = extract_outbound_links(page)
        for link in outlinks:
            if link == "glossary" or "#" in link:
                continue
            if link not in all_slugs:
                report.warn(f"{relpath}: broken link [[{link}]]")
            elif link in inbound:
                inbound[link].append(slug)

    if not target_path:
        for slug, sources in inbound.items():
            if not sources and slug not in ("index",):
                report.warn(f"{slug}: orphan page (no inbound [[wiki-links]])")

    if readability_check:
        all_readability: list[tuple[str, int, str]] = []

        for page in lint_pages:
            relpath = str(page.relative_to(WIKI_ROOT))

            for line_num, msg in check_sentence_length(page):
                all_readability.append((relpath, line_num, f"[sentence-length] {msg}"))

            passive_ratio, passive_examples = check_passive_voice(page)
            if passive_ratio > _PASSIVE_WARN_RATIO:
                pct = int(passive_ratio * 100)
                all_readability.append((relpath, 0, f"[passive-voice] {pct}% passive (threshold {int(_PASSIVE_WARN_RATIO*100)}%)"))
                for ln, ex in passive_examples:
                    all_readability.append((relpath, ln, f"[passive-voice] example: \"{ex}\""))

            for line_num, msg in check_paragraph_length(page):
                all_readability.append((relpath, line_num, f"[paragraph-size] {msg}"))

            for line_num, msg in check_section_length(page):
                all_readability.append((relpath, line_num, f"[section-size] {msg}"))

        if all_readability:
            print(f"\n{'='*60}")
            print(f"  READABILITY ({len(all_readability)} issues)")
            print(f"{'='*60}")
            for relpath, line_num, msg in sorted(all_readability):
                loc = f"{relpath}:{line_num}" if line_num > 0 else relpath
                safe_msg = msg.encode("ascii", "replace").decode("ascii")
                report.warn(f"{loc}: {msg}")
                print(f"  [WARN] {loc}: {safe_msg}")
        else:
            print(f"\n  Readability: all checks passed")

    return report


def main():
    parser = argparse.ArgumentParser(description="Rhapsode Wiki Lint Tool")
    parser.add_argument("page", nargs="?", help="Lint a single page by slug (e.g. scene-loop)")
    parser.add_argument("--no-readability", action="store_true", help="Skip readability checks")
    parser.add_argument("--health", action="store_true", help="Run page health scoring")
    parser.add_argument("--health-threshold", type=float, default=3.0,
                        help="Score below which pages are flagged (default: 3.0)")
    args = parser.parse_args()

    if args.page:
        print(f"Rhapsode Wiki Lint - scanning {args.page}")
    else:
        print(f"Rhapsode Wiki Lint - scanning {WIKI_ROOT}")

    report = lint(page_filter=args.page, readability_check=not args.no_readability)
    report.print_report()

    if args.health:
        pages = iter_pages()
        results = run_health_scoring(pages, threshold=args.health_threshold,
                                     page_filter=args.page)
        below = print_health_report(results, threshold=args.health_threshold)
        if below > 0 and report.ok:
            sys.exit(0)

    sys.exit(0 if report.ok else 1)


if __name__ == "__main__":
    main()
