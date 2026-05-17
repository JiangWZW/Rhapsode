/**
 * Scene text parser for Rhapsode narrative output.
 *
 * Uses markdown-it (typographer: true) as the rendering engine so that:
 *   - *emphasis*   → <em> (amber-accented in CSS)
 *   - **bold**     → <strong>
 *   - "dialogue"   → <span class="sq"> with proper curly quotes
 *   - (aside)      → <span class="paren"> for parenthetical/whisper
 *   - [note]       → <span class="bk"> for game/system annotations
 *   - --           → — (em-dash)
 *   - ...          → … (ellipsis)
 *   - "..."        → "..." (smart/curly quotes on non-dialogue text)
 *
 * All block paragraphs receive class="sp" for CSS targeting.
 * html: false is kept to prevent XSS from LLM output.
 */

import MarkdownIt from 'markdown-it'
import type StateInline from 'markdown-it/lib/rules_inline/state_inline.mjs'

const md = new MarkdownIt({
  typographer: true,
  breaks:      false,
  html:        false,
  linkify:     false,
})

// Add class="sp" to every paragraph so CSS can target narration text.
md.renderer.rules.paragraph_open = (tokens, idx, options, _env, self) => {
  tokens[idx].attrSet('class', 'sp')
  return self.renderToken(tokens, idx, options)
}

// --- Custom inline rules ---
// These run before markdown-it's typographer core pass, so we look for
// plain straight-quote characters (U+0022 / U+0028 / U+005B) and emit
// html_inline tokens that are never touched by the typographer.

function dialogueRule(state: StateInline, silent: boolean): boolean {
  if (state.src.charCodeAt(state.pos) !== 0x22 /* " */) return false
  const start = state.pos + 1
  const close = state.src.indexOf('"', start)
  if (close < start + 1) return false
  if (!silent) {
    const inner = state.src.slice(start, close)
    const token  = state.push('html_inline', '', 0)
    // Manually apply em-dash / ellipsis inside the span since it won't be
    // visited by the typographer core rule (which only touches text tokens).
    const cooked = inner.replace(/--/g, '\u2014').replace(/\.\.\./g, '\u2026')
    token.content = `<span class="sq">\u201C${cooked}\u201D</span>`
  }
  state.pos = close + 1
  return true
}

function parenRule(state: StateInline, silent: boolean): boolean {
  if (state.src.charCodeAt(state.pos) !== 0x28 /* ( */) return false
  const start = state.pos + 1
  const close = state.src.indexOf(')', start)
  if (close < start + 1) return false
  if (!silent) {
    const token = state.push('html_inline', '', 0)
    token.content = `<span class="paren">(${state.src.slice(start, close)})</span>`
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
    token.content = `<span class="bk">[${state.src.slice(start, close)}]</span>`
  }
  state.pos = close + 1
  return true
}

// Push after all built-in rules so Markdown links ([text](url)) are handled first.
md.inline.ruler.push('rhapsode_dialogue', dialogueRule)
md.inline.ruler.push('rhapsode_paren',    parenRule)
md.inline.ruler.push('rhapsode_bracket',  bracketRule)

export function parseScene(raw: string): string {
  if (!raw) return ''
  return md.render(raw)
}
