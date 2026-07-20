import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { ChatMessage, EntitySpan } from '../types/protocol'

export type { ChatMessage, EntitySpan } from '../types/protocol'

export const useWebSocket = defineStore('websocket', () => {
  const messages = ref<ChatMessage[]>([])
  const connected = ref(false)
  const processing = ref(false)

  let ws: WebSocket | null = null

  function connect() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) {
      return
    }
    const proto = location.protocol === 'https:' ? 'wss' : 'ws'
    ws = new WebSocket(`${proto}://${location.host}/ws`)

    ws.onopen = () => { connected.value = true }

    ws.onclose = () => {
      connected.value = false
      ws = null
    }

    ws.onmessage = (event) => {
      let data: Record<string, unknown>
      try {
        data = JSON.parse(event.data) as Record<string, unknown>
      } catch {
        messages.value.push({
          role: 'assistant',
          content: '[Error] Invalid server message',
          scene_kind: 'narrator',
        })
        processing.value = false
        return
      }

      function pushAssistant(
        content: string,
        sceneKind?: 'narrator' | 'character',
        speaker?: string,
        entities?: EntitySpan[],
      ) {
        const row: ChatMessage = {
          role: 'assistant',
          content,
          scene_kind: sceneKind ?? 'narrator',
        }
        if (speaker) row.speaker = speaker
        if (entities) row.entities = entities
        messages.value.push(row)
        processing.value = false
      }

      if (data.type === 'scene_message') {
        const kind =
          data.scene_kind === 'character'
            ? 'character'
            : 'narrator'
        pushAssistant(String(data.content ?? ''), kind,
          typeof data.speaker === 'string' ? data.speaker : undefined,
          Array.isArray(data.entities) ? data.entities as EntitySpan[] : undefined)
      } else if (data.type === 'assistant_message') {
        pushAssistant(String(data.content ?? ''), 'narrator')
      } else if (data.type === 'user_message') {
        messages.value.push({ role: 'user', content: String(data.content ?? '') })
      } else if (data.type === 'error') {
        messages.value.push({
          role: 'assistant',
          content: `[Error] ${String(data.detail ?? 'Unknown server error')}`,
          scene_kind: 'narrator',
        })
        processing.value = false
      } else if (data.type === 'status') {
        processing.value = data.state !== 'idle'
      }
    }
  }

  function send(text: string) {
    if (!ws || ws.readyState !== WebSocket.OPEN) return
    messages.value.push({ role: 'user', content: text })
    processing.value = true
    ws.send(JSON.stringify({ type: 'player_message', content: text }))
  }

  function disconnect() {
    ws?.close()
  }

  return { messages, connected, processing, connect, send, disconnect }
})
