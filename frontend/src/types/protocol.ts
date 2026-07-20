export interface EntitySpan {
  start: number
  end: number
  text: string
  category: string
}

export interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  /** Present for assistant rows from merged flow; omission means narrator prose. */
  scene_kind?: 'narrator' | 'character'
  speaker?: string
  entities?: EntitySpan[]
}
