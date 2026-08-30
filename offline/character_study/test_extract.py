"""Extractor and nav caps. No API."""

from __future__ import annotations

from cite import cite_text, find_line
from critic import SYSTEM, _study_body
from extract import build_sections, volume_bounds, window_spans
from nav import clip_range, format_brief, format_lines, format_sources

VOL = [{"id": 1, "title": "T", "start": 1, "end": 200}]


def test_window_coalesce():
    hits = [10, 12, 40]
    spans = window_spans(hits, VOL, window=10)
    assert spans == [(1, 0, 22, 2), (1, 30, 50, 1)]


def test_overlapping_windows_merge():
    hits = list(range(0, 90, 15))
    spans = window_spans(hits, VOL, window=20)
    assert spans == [(1, 0, 95, 6)]


def test_window_stays_in_volume():
    vols = [
        {"id": 1, "title": "A", "start": 1, "end": 50},
        {"id": 2, "title": "B", "start": 51, "end": 100},
    ]
    assert volume_bounds(45, vols) == (1, 0, 49)
    spans = window_spans([45, 52], vols, window=20)
    assert spans[0] == (1, 25, 49, 1)
    assert spans[1] == (2, 50, 72, 1)


def test_real_extract_count():
    from corpus import corpus_ready, load_config, load_lines

    cfg = load_config()
    if not corpus_ready(cfg):
        print("skip test_real_extract_count (novel or books/konosuba missing)")
        return
    sections = build_sections(load_lines(cfg), cfg)
    assert 20 <= len(sections) <= 80
    assert sections[0]["volume"] == 1
    assert sections[0]["line_start"] >= 1
    assert "Darkness" in sections[0]["text"] or "Lalatina" in sections[0]["text"]
    assert all(s["line_end"] >= s["line_start"] for s in sections)


def test_nav_caps():
    lines = [f"p{i}" for i in range(1, 21)]
    a, b = clip_range(1, 100, 20, 40)
    assert (a, b) == (1, 20)
    a, b = clip_range(5, 50, 20, 40)
    assert (a, b) == (5, 20)
    text = format_lines(lines, 1, 20, cap=5)
    assert "lines 1-5" in text
    assert "trimmed" in text
    assert "p6" not in text


def test_brief_and_sources():
    from corpus import brief_path

    if brief_path(3).is_file():
        brief = format_brief(3)
        assert "Lalatina" in brief
        assert "wikipedia.org" in brief
    else:
        print("skip brief asserts (books/konosuba missing)")
    vols = [{"id": 1, "title": "T", "start": 1, "end": 10}]
    secs = [{"id": 1, "volume": 1, "line_start": 3, "line_end": 8, "hit_count": 2}]
    listing = format_sources(vols, secs, current_id=1)
    assert "vol 1" in listing
    assert "section 1" in listing


def test_study_body():
    assert _study_body("# Darkness\n\nok") == "# Darkness\n\nok"
    assert _study_body("thinking\n\n# Darkness\n\nok").startswith("# Darkness")


def test_no_register_in_critic_prompt():
    assert "register" not in SYSTEM.lower()


def test_cite_unique_quote():
    lines = ["aaa", "She said hello there world today.", "bbb"]
    vols = [{"id": 1, "start": 1, "end": 10}]
    assert find_line("hello there world today", lines) == 2
    out = cite_text('He said "hello there world today" and left.', lines, vols)
    assert "(v1 L2)" in out
    again = cite_text(out, lines, vols)
    assert again.count("(v1 L2)") == 1


if __name__ == "__main__":
    test_window_coalesce()
    test_overlapping_windows_merge()
    test_window_stays_in_volume()
    test_nav_caps()
    test_brief_and_sources()
    test_real_extract_count()
    test_study_body()
    test_no_register_in_critic_prompt()
    test_cite_unique_quote()
    print("ok")
