import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from rhapsode._core import CharacterMemory
from rhapsode.graph_views import _format_streams_html, _mind_snapshot

m = CharacterMemory("Aqua")
m.ensure_bootstrap("Goddess continuity.")
m.update_monologues(
    1,
    "Goddess",
    "Player insults Aqua.",
    lambda _p: json.dumps({
        "appends": [{"stream_id": "self", "text": "How dare he."}],
        "ops": [],
        "knows": [{
            "fact": "Player insulted me",
            "entities": ["Player"],
            "weight": 6,
        }],
        "core_revision": None,
    }),
)
raw = m.render_mind_query()
payload = json.loads(raw)
assert "core" in payload and "streams" in payload
snap = _mind_snapshot(m)
assert snap["core"]
assert snap["streams"]
assert "How dare" in (snap["streams"][0].get("recent_lines") or [{}])[0].get("text", "")
html = _format_streams_html(snap["streams"])
assert "stream" in html
print("ok", snap["active_stream_count"], "streams;", "beliefs_len", len(snap["beliefs"]))
