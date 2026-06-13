"""FABLE fiction-NER model loader and callback factory."""

from __future__ import annotations

import json
import logging

log = logging.getLogger(__name__)

_pipe = None

LABEL_MAP = {
    "CHA": "character",
    "LOC": "location",
    "FAC": "location",
    "OBJ": "item",
    "EVT": "event",
    "ORG": "faction",
    "MISC": "entity",
}


def warmup_fable() -> None:
    global _pipe
    if _pipe is not None:
        return
    from transformers import pipeline

    log.info("Loading FABLE NER model ...")
    _pipe = pipeline(
        "token-classification",
        model="SaladTechnologies/fable-base",
        aggregation_strategy="simple",
        device=-1,
    )
    log.info("FABLE ready.")


def make_ner_callback():
    """Return a callback matching C++ NERCallback: text -> JSON array of EntitySpan."""
    warmup_fable()

    def ner(text: str) -> str:
        results = _pipe(text)
        spans = []
        for r in results:
            if r["score"] < 0.5:
                continue
            cat = LABEL_MAP.get(r["entity_group"], "entity")
            spans.append({
                "start": r["start"],
                "end": r["end"],
                "text": r["word"].strip(),
                "category": cat,
            })
        return json.dumps(spans)

    return ner
