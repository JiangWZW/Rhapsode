import { defineStore } from 'pinia'
import { ref } from 'vue'

export interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  /** Present for assistant rows from merged flow; omission means narrator prose. */
  scene_kind?: 'narrator' | 'character'
  speaker?: string
}

export const useWebSocket = defineStore('websocket', () => {
  const messages = ref<ChatMessage[]>([])
  const connected = ref(false)
  const processing = ref(false)

  let ws: WebSocket | null = null

  function connect() {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws'
    ws = new WebSocket(`${proto}://${location.host}/ws`)

    ws.onopen = () => { connected.value = true }

    ws.onclose = () => {
      connected.value = false
      ws = null
    }

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data)

      function pushAssistant(
        content: string,
        sceneKind?: 'narrator' | 'character',
        speaker?: string,
      ) {
        const row: ChatMessage = {
          role: 'assistant',
          content,
          scene_kind: sceneKind ?? 'narrator',
        }
        if (speaker) row.speaker = speaker
        messages.value.push(row)
        processing.value = false
      }

      if (data.type === 'scene_message') {
        const kind =
          data.scene_kind === 'character'
            ? 'character'
            : 'narrator'
        pushAssistant(data.content ?? '', kind, data.speaker)
      } else if (data.type === 'assistant_message') {
        pushAssistant(data.content ?? '', 'narrator')
      } else if (data.type === 'error') {
        messages.value.push({
          role: 'assistant',
          content: `[Error] ${data.detail}`,
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
