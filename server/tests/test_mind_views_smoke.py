import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from rhapsode._core import CharacterMemory
from rhapsode.graph_views import _format_monologue_html, _mind_snapshot

m = CharacterMemory("Aqua")
m.ensure_bootstrap("Goddess continuity.")
m.update_perception(
    1,
    "Goddess",
    "Player insults Aqua.",
    lambda _p: json.dumps({"perception": "He insulted me."}),
)
m.update_monologues(
    1,
    "Goddess",
    lambda _p: json.dumps({"line": "How dare he."}),
)
raw = m.render_mind_query()
payload = json.loads(raw)
assert "core" in payload and "monologue" in payload
assert "perception" in payload
assert "He insulted" in payload["perception"]
assert "streams" not in payload
snap = _mind_snapshot(m)
assert snap["core"]
assert snap["perception"]
assert "He insulted" in snap["perception"]
assert snap["monologue"]
assert "How dare" in (snap["monologue"][0].get("text") or "")
html = _format_monologue_html(snap["monologue"])
assert "How dare" in html
print("ok", len(snap["monologue"]), "lines;", "beliefs_len", len(snap["beliefs"]))
