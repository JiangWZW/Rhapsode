"""FABLE fiction-NER model loader and callback factory."""

from __future__ import annotations

import json
import logging

log = logging.getLogger(__name__)

_pipe = None
FABLE_MODEL = "SaladTechnologies/fable-base"

LABEL_MAP = {
    "CHA": "character",
    "LOC": "location",
    "FAC": "location",
    "OBJ": "item",
    "EVT": "event",
    "ORG": "faction",
    "MISC": "entity",
}


def _load_fable_pipeline():
    from transformers import (
        AutoModelForTokenClassification,
        AutoTokenizer,
        pipeline,
    )

    try:
        tokenizer = AutoTokenizer.from_pretrained(
            FABLE_MODEL, local_files_only=True)
        model = AutoModelForTokenClassification.from_pretrained(
            FABLE_MODEL, local_files_only=True)
        pipe = pipeline(
            "token-classification",
            model=model,
            tokenizer=tokenizer,
            aggregation_strategy="simple",
            device=-1,
        )
        log.info("FABLE ready (local cache).")
        return pipe
    except Exception as exc:
        log.warning(
            "Local cache miss for %s (%s); fetching from HuggingFace once.",
            FABLE_MODEL, exc,
        )
        pipe = pipeline(
            "token-classification",
            model=FABLE_MODEL,
            aggregation_strategy="simple",
            device=-1,
        )
        log.info("FABLE ready (downloaded).")
        return pipe


def warmup_fable() -> None:
    global _pipe
    if _pipe is not None:
        return
    log.info("Loading FABLE NER model ...")
    _pipe = _load_fable_pipeline()


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
