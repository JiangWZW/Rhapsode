"""Shared spaCy lemmatizer model singleton."""

from __future__ import annotations

import logging

log = logging.getLogger(__name__)

_nlp_lemma = None


def _ensure_model() -> None:
    import spacy
    if not spacy.util.is_package("en_core_web_sm"):
        log.info("Downloading spaCy model en_core_web_sm ...")
        from spacy.cli import download
        download("en_core_web_sm")


def get_nlp_lemma():
    """Lemmatizer-only pipeline (parser/NER disabled) for BM25."""
    global _nlp_lemma
    if _nlp_lemma is not None:
        return _nlp_lemma
    try:
        _ensure_model()
        import spacy
        _nlp_lemma = spacy.load("en_core_web_sm", disable=["ner", "parser"])
        log.info("spaCy lemma model loaded")
    except Exception:
        log.warning("Failed to load spaCy lemma model", exc_info=True)
        return None
    return _nlp_lemma
