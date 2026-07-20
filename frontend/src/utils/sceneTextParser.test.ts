import { describe, expect, it } from 'vitest'
import { applyAnnotations, parseScene } from './sceneTextParser'

describe('parseScene', () => {
  it('escapes model HTML inside custom spans', () => {
    const html = parseScene('(<script>alert(1)</script>) [<img src=x onerror=alert(2)>]')

    expect(html).not.toContain('<script>')
    expect(html).not.toContain('<img')
    expect(html).toContain('&lt;script&gt;')
    expect(html).toContain('&lt;img src=x onerror=alert(2)&gt;')
  })

  it('keeps Unicode spans and leaves malformed brackets as text', () => {
    expect(parseScene('(低声) [警告]')).toContain('<span class="paren">(低声)</span>')
    expect(parseScene('(unfinished [also unfinished')).toContain('(unfinished [also unfinished')
  })
})

describe('applyAnnotations', () => {
  it('annotates text without modifying existing tags', () => {
    const html = applyAnnotations('<p class="sp">Ash met Ash.</p>', [
      { start: 0, end: 3, text: 'Ash', category: 'person' },
    ])
    expect(html).toBe('<p class="sp"><span class="ent ent-person">Ash</span> met <span class="ent ent-person">Ash</span>.</p>')
  })

  it('sanitizes model-provided categories before using them as CSS classes', () => {
    const html = applyAnnotations('<p>Ash</p>', [
      { start: 0, end: 3, text: 'Ash', category: 'person" onclick="alert(1)' },
    ])
    expect(html).not.toContain('onclick=')
    expect(html).toContain('ent-person-onclick-alert-1-')
  })
})
