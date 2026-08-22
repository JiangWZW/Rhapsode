import { defineStore } from 'pinia'
import { ref } from 'vue'
import type { ChatMessage, EntitySpan } from '../types/protocol'

export type { ChatMessage, EntitySpan } from '../types/protocol'

const RECONNECT_MS_MIN = 500
const RECONNECT_MS_MAX = 8000

export const useWebSocket = defineStore('websocket', () => {
  const messages = ref<ChatMessage[]>([])
  const connected = ref(false)
  const processing = ref(false)

  let ws: WebSocket | null = null
  let intentionalClose = false
  let reconnectTimer: ReturnType<typeof setTimeout> | null = null
  let reconnectAttempt = 0

  function clearReconnectTimer() {
    if (reconnectTimer !== null) {
      clearTimeout(reconnectTimer)
      reconnectTimer = null
    }
  }

  function scheduleReconnect() {
    clearReconnectTimer()
    const delay = Math.min(
      RECONNECT_MS_MAX,
      RECONNECT_MS_MIN * 2 ** reconnectAttempt,
    )
    reconnectAttempt += 1
    reconnectTimer = setTimeout(() => {
      reconnectTimer = null
      connect()
    }, delay)
  }

  function connect() {
    if (ws && (ws.readyState === WebSocket.CONNECTING || ws.readyState === WebSocket.OPEN)) {
      return
    }
    clearReconnectTimer()
    intentionalClose = false
    const proto = location.protocol === 'https:' ? 'wss' : 'ws'
    ws = new WebSocket(`${proto}://${location.host}/ws`)

    ws.onopen = () => {
      connected.value = true
      reconnectAttempt = 0
      // Server reseeds history on every connect; drop stale client buffer.
      messages.value = []
      processing.value = true
    }

    ws.onerror = () => {
      // onclose always follows; reconnect there.
    }

    ws.onclose = () => {
      connected.value = false
      processing.value = false
      ws = null
      if (!intentionalClose) scheduleReconnect()
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
        // `ready` = player beat is on screen; weave/monologue may still run.
        // `idle` = post-turn finished. Only `processing` locks the input.
        processing.value = data.state === 'processing'
      } else if (data.type === 'undo') {
        // Server reseeds after undo; clear before the new timeline arrives.
        messages.value = []
        processing.value = true
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
    intentionalClose = true
    clearReconnectTimer()
    reconnectAttempt = 0
    ws?.close()
    ws = null
    connected.value = false
  }

  return { messages, connected, processing, connect, send, disconnect }
})
