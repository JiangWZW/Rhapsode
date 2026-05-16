"""BM25 lemmatization -- stop-word removal + lemma + -ing preservation."""

from __future__ import annotations

from rhapsode.spacy_models import get_nlp_lemma


def lemmatize_for_bm25(text: str) -> str:
    """Lemmatize text for BM25 keyword matching.

    Returns space-joined lemmas. Falls back to lowercased input
    if spaCy is unavailable.
    """
    nlp = get_nlp_lemma()
    if nlp is None:
        return text.lower()

    doc = nlp(text.lower())
    tokens: list[str] = []
    for token in doc:
        if token.is_punct or token.is_stop:
            continue
        lemma = token.lemma_
        if lemma.isalnum():
            tokens.append(lemma)
        if token.text.endswith("ing") and token.text != lemma and token.text.isalnum():
            tokens.append(token.text)
    return " ".join(tokens)
