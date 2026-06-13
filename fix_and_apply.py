#!/usr/bin/env python3
import re
import subprocess
import sys
from pathlib import Path

PATCH = Path("local_changes.patch")
OUT = Path("local_changes_clean.patch")

REPLACEMENTS = [
    ("鈹溾攢鈹€", "├──"),
    ("鈹斺攢鈹€", "└──"),
    ("鈹屸攢鈹€", "┌──"),
    ("鈹尖攢", "├─"),
    ("鈹傗攢", "│ ├"),
    ("鈹傗啋", "│ │"),
    ("鈹傗攤", "│ │"),
    ("鈹傗梽", "│ └"),
    ("鈹粹攢", "└─"),
    ("鈹樷攤", "│ │"),
    ("鈹愨攤", "│ │"),
    ("鈻尖攢", "▼─"),
    ("鈹攢", "──"),
    ("鈹€鈹€", "──"),
    ("鈹€", "─"),
    ("鈫?鈥?", " → … "),
    (" 鈥?", " — "),
    ("鈥?", "—"),
    ("閳?", "—"),
    ("鈫?", "→ "),
    ("鈻?", "▼"),
    ("鈹?", "→ "),
    ("脳", "×"),
    ("搂", "§"),
    ("飪?", "●"),
    ("閳光偓", "──"),
    ("锘?", ""),
    ("鈥檚", "'s"),
    ("鈥搑", "-"),
    ("鈥損", "-"),
    ("鈥揕", "-"),
    ("鈭?", "∝"),
    ("鈮?", "≈"),
    ("Fa莽ade", "Façade"),
    ("rhaps艒idos", "rhapsōidos"),
    ("rh谩ptein", "rhaptein"),
    ("艒id岣?", "ōidē"),
    ("rhaps艒id贸s", "rhapsōidós"),
    ("鈼勨攢", "├─"),
    ("鈼勨攢─", "├──"),
]


def extract(text: str) -> str:
    lines = text.splitlines(keepends=True)
    return "".join(lines[:8471] + lines[43732:])


def fix_encoding(text: str) -> str:
    for old, new in REPLACEMENTS:
        text = text.replace(old, new)
    return text.lstrip("\ufeff")


def parse_hunk_header(header: str) -> tuple[int, int, int, int]:
    old = re.search(r"-(\d+)(?:,(\d+))?", header)
    new = re.search(r"\+(\d+)(?:,(\d+))?", header)
    return (
        int(old.group(1)),
        int(old.group(2) or "1"),
        int(new.group(1)),
        int(new.group(2) or "1"),
    )


def repair_hunk_from_file(file_lines: list[str], body: list[str]) -> list[str]:
    """Rebuild context/removal lines from the working tree file."""
    if not body:
        return body
    old_idx = 0
    repaired: list[str] = []
    for line in body:
        if line.startswith("\\ No newline at end of file"):
            repaired.append(line)
            continue
        if line.startswith("+"):
            repaired.append(line)
            continue
        if old_idx >= len(file_lines):
            repaired.append(line)
            continue
        content = file_lines[old_idx]
        old_idx += 1
        if line.startswith("-"):
            repaired.append("-" + content)
        elif line.startswith(" "):
            repaired.append(" " + content)
        else:
            repaired.append(line)
    return repaired


def repair_file_chunk(chunk: str) -> str:
    path_match = re.match(r"^a/(.+?) b/", chunk)
    if not path_match:
        return chunk
    path = path_match.group(1)
    idx_match = re.search(r"^index ([0-9a-f]+)\.\.", chunk, re.M)
    if not idx_match:
        return chunk
    file_path = Path(path)
    if not file_path.exists():
        return chunk
    blob = subprocess.check_output(["git", "hash-object", str(file_path)], text=True).strip()
    if not blob.startswith(idx_match.group(1)):
        return chunk

    file_lines = file_path.read_text(encoding="utf-8").splitlines()
    lines = chunk.splitlines(keepends=True)
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("@@ "):
            out.append(line)
            i += 1
            continue
        m = re.match(r"^@@ (.+) @@(.*)$", line.rstrip("\n"))
        header, suffix = m.group(1), m.group(2)
        old_start, _, new_start, _ = parse_hunk_header(header)
        body: list[str] = []
        i += 1
        while i < len(lines) and not lines[i].startswith("@@ ") and not lines[i].startswith(
            "diff --git "
        ):
            body.append(lines[i].rstrip("\n"))
            i += 1
        if old_start > 0 and file_path.exists():
            body = repair_hunk_from_file(file_lines[old_start - 1 :], body)
        ctx = sum(1 for b in body if b.startswith(" "))
        add = sum(1 for b in body if b.startswith("+") and not b.startswith("+++"))
        rem = sum(1 for b in body if b.startswith("-") and not b.startswith("---"))
        out.append(f"@@ -{old_start},{ctx + rem} +{new_start},{ctx + add} @@{suffix}\n")
        out.extend(b + "\n" for b in body)
    return "".join(out)


def recalc_hunk_counts(text: str) -> str:
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.startswith("@@ "):
            out.append(line)
            i += 1
            continue
        m = re.match(r"^@@ (.+) @@(.*)$", line.rstrip("\n"))
        header, suffix = m.group(1), m.group(2)
        old_start, _, new_start, _ = parse_hunk_header(header)
        body: list[str] = []
        i += 1
        while i < len(lines) and not lines[i].startswith("@@ ") and not lines[i].startswith(
            "diff --git "
        ):
            body.append(lines[i].rstrip("\n"))
            i += 1
        ctx = sum(1 for b in body if b.startswith(" "))
        add = sum(1 for b in body if b.startswith("+") and not b.startswith("+++"))
        rem = sum(1 for b in body if b.startswith("-") and not b.startswith("---"))
        out.append(f"@@ -{old_start},{ctx + rem} +{new_start},{ctx + add} @@{suffix}\n")
        out.extend(b + "\n" for b in body)
    return "".join(out)


def repair_patch(text: str) -> str:
    parts = text.split("diff --git ")
    repaired = [parts[0]]
    for chunk in parts[1:]:
        repaired.append(repair_file_chunk(chunk))
    return "diff --git ".join(repaired)


def apply_chunk_to_lines(lines: list[str], chunk: str) -> list[str]:
    """Apply unified diff hunks to line list (1-based patch offsets)."""
    result = lines[:]
    offset = 0
    for m in re.finditer(r"^@@ (.+) @@.*$", chunk, re.M):
        header = m.group(1)
        old_start, old_count, _, _ = parse_hunk_header(header)
        start = m.end()
        nxt = chunk.find("\n@@ ", start)
        body = (chunk[start:nxt] if nxt != -1 else chunk[start:]).splitlines()
        idx = old_start - 1 + offset
        new_segment: list[str] = []
        old_used = 0
        for line in body:
            if not line or line.startswith("diff --git "):
                break
            if line.startswith("\\ No newline at end of file"):
                continue
            if line.startswith(" "):
                new_segment.append(line[1:])
                old_used += 1
            elif line.startswith("-"):
                old_used += 1
            elif line.startswith("+"):
                new_segment.append(line[1:])
        result[idx : idx + old_used] = new_segment
        offset += len(new_segment) - old_used
    return result


def apply_file_chunk_direct(path: str, chunk: str) -> None:
    file_path = Path(path)
    lines = file_path.read_text(encoding="utf-8").splitlines()
    updated = apply_chunk_to_lines(lines, chunk)
    file_path.write_text("\n".join(updated) + ("\n" if updated else ""), encoding="utf-8")


def split_patch(text: str) -> list[tuple[str, str]]:
    parts = text.split("diff --git ")
    chunks: list[tuple[str, str]] = []
    for part in parts[1:]:
        path = part.split("\n", 1)[0].split(" b/", 1)[-1]
        chunks.append((path, "diff --git " + part))
    return chunks


def main() -> int:
    raw = PATCH.read_bytes().decode("utf-8", errors="surrogateescape")
    fixed = recalc_hunk_counts(repair_patch(fix_encoding(extract(raw))))
    OUT.write_bytes(fixed.encode("utf-8", errors="surrogateescape"))

    check = subprocess.run(["git", "apply", "--check", str(OUT)], capture_output=True, text=True)
    if check.returncode == 0:
        subprocess.run(["git", "apply", str(OUT)], check=True)
        print("Applied successfully.")
        return 0

    stderr = check.stderr
    print(stderr, file=sys.stderr)
    failed = [p.strip() for p in re.findall(r"error: patch failed: ([^:\n]+):", stderr)]
    failed = list(dict.fromkeys(failed))
    chunks = split_patch(fixed)
    ok_chunks = [c for p, c in chunks if p not in failed]
    fail_map = {p: c for p, c in chunks if p in failed}

    if ok_chunks:
        partial = Path("local_changes_partial.patch")
        partial.write_text("".join(ok_chunks), encoding="utf-8")
        subprocess.run(["git", "apply", str(partial)], check=True)

    for path, chunk in fail_map.items():
        apply_file_chunk_direct(path, chunk)
        print(f"Applied directly: {path}")

    final = subprocess.run(["git", "apply", "--check", str(OUT)], capture_output=True, text=True)
    if final.returncode == 0:
        print("All changes applied.")
        return 0
    print(final.stderr, file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
