import { defineStore } from 'pinia'
import { ref } from 'vue'

interface Message {
  role: 'user' | 'assistant'
  content: string
}

export const useWebSocket = defineStore('websocket', () => {
  const messages = ref<Message[]>([])
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
      if (data.type === 'assistant_message') {
        messages.value.push({ role: 'assistant', content: data.content })
        processing.value = false
      } else if (data.type === 'error') {
        messages.value.push({ role: 'assistant', content: `[Error] ${data.detail}` })
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
