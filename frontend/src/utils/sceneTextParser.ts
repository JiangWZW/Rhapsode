/**
 * Scene text parser for Rhapsode narrative output.
 *
 * Uses markdown-it (typographer: true) as the rendering engine so that:
 *   - (aside)      → <span class="paren"> for parenthetical/whisper
 *   - [note]       → <span class="bk"> for game/system annotations
 *   - --           → — (em-dash)
 *   - ...          → … (ellipsis)
 *
 * Dialogue is NOT detected by matching quote pairs. Under the current design
 * the narrator prose is narration-only; every spoken line arrives structurally
 * as a separate `character` message (rendered by StoryPanel's .dialogue-block,
 * styled by CSS). Quote-pair matching is therefore gone — it would only mangle
 * stray quotes (e.g. a leaked JSON fragment) into fake dialogue spans.
 *
 * Emphasis (*...*  / **...**) is disabled — entity highlighting is
 * driven by the C++ Annotator + FABLE NER instead.
 *
 * All block paragraphs receive class="sp" for CSS targeting.
 * html: false is kept to prevent XSS from LLM output.
 */

import MarkdownIt from 'markdown-it'
import type StateInline from 'markdown-it/lib/rules_inline/state_inline.mjs'
import type { EntitySpan } from '../types/protocol'

const md = new MarkdownIt({
  typographer: true,
  breaks:      false,
  html:        false,
  linkify:     false,
})

md.disable(['emphasis'])

// Add class="sp" to every paragraph so CSS can target narration text.
md.renderer.rules.paragraph_open = (tokens, idx, options, _env, self) => {
  tokens[idx].attrSet('class', 'sp')
  return self.renderToken(tokens, idx, options)
}

// --- Custom inline rules ---
// These run before markdown-it's typographer core pass, so we look for
// plain bracket characters (U+0028 ( / U+005B [) and emit html_inline tokens
// that are never touched by the typographer.

function parenRule(state: StateInline, silent: boolean): boolean {
  if (state.src.charCodeAt(state.pos) !== 0x28 /* ( */) return false
  const start = state.pos + 1
  const close = state.src.indexOf(')', start)
  if (close < start + 1) return false
  if (!silent) {
    const token = state.push('html_inline', '', 0)
    const content = md.utils.escapeHtml(state.src.slice(start, close))
    token.content = `<span class="paren">(${content})</span>`
  }
  state.pos = close + 1
  return true
}

function bracketRule(state: StateInline, silent: boolean): boolean {
  if (state.src.charCodeAt(state.pos) !== 0x5b /* [ */) return false
  const start = state.pos + 1
  const close = state.src.indexOf(']', start)
  if (close < start + 1) return false
  if (!silent) {
    const token = state.push('html_inline', '', 0)
    const content = md.utils.escapeHtml(state.src.slice(start, close))
    token.content = `<span class="bk">[${content}]</span>`
  }
  state.pos = close + 1
  return true
}

// Run parentheses before the catch-all text rule. Brackets stay after built-in
// link handling so [text](url) retains Markdown semantics.
md.inline.ruler.before('text', 'rhapsode_paren', parenRule)
md.inline.ruler.push('rhapsode_bracket',  bracketRule)

export function parseScene(raw: string): string {
  if (!raw) return ''
  return md.render(raw)
}

function escapeRegex(s: string): string {
  return s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
}

export function applyAnnotations(html: string, entities: EntitySpan[]): string {
  if (!entities || !entities.length) return html

  const sorted = [...entities].sort((a, b) => b.text.length - a.text.length)
  const seen = new Set<string>()

  for (const ent of sorted) {
    const key = `${ent.text.toLowerCase()}::${ent.category}`
    if (seen.has(key)) continue
    seen.add(key)

    const escaped = escapeRegex(ent.text)
    const category = ent.category.toLowerCase().replace(/[^a-z0-9_-]+/g, '-') || 'unknown'
    const re = new RegExp(`\\b${escaped}\\b`, 'gi')
    html = html.replace(/([^<]+)|(<[^>]+>)/g, (_match, text, tag) => {
      if (tag) return tag
      return text.replace(re, (m: string) =>
        `<span class="ent ent-${category}">${m}</span>`)
    })
  }
  return html
}
