import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'
import { useWebSocket } from './websocket'

class MockWebSocket {
  static readonly CONNECTING = 0
  static readonly OPEN = 1
  static readonly CLOSED = 3
  static instances: MockWebSocket[] = []

  readyState = MockWebSocket.CONNECTING
  onopen: (() => void) | null = null
  onclose: (() => void) | null = null
  onmessage: ((event: { data: string }) => void) | null = null
  sent: string[] = []
  closed = false
  readonly url: string

  constructor(url: string) {
    this.url = url
    MockWebSocket.instances.push(this)
  }

  send(data: string) { this.sent.push(data) }
  close() {
    this.closed = true
    this.readyState = MockWebSocket.CLOSED
    this.onclose?.()
  }
}

describe('websocket store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    MockWebSocket.instances = []
    vi.stubGlobal('WebSocket', MockWebSocket)
    vi.stubGlobal('location', { protocol: 'http:', host: 'example.test' })
  })

  it('does not create duplicate connecting sockets', () => {
    const store = useWebSocket()
    store.connect()
    store.connect()
    expect(MockWebSocket.instances).toHaveLength(1)
  })

  it('tracks connection, processing, errors, and disconnects', () => {
    const store = useWebSocket()
    store.connect()
    const socket = MockWebSocket.instances[0]
    socket.readyState = MockWebSocket.OPEN
    socket.onopen?.()
    expect(store.connected).toBe(true)

    store.send('hello')
    expect(store.processing).toBe(true)
    expect(socket.sent).toEqual([JSON.stringify({ type: 'player_message', content: 'hello' })])

    socket.onmessage?.({ data: JSON.stringify({ type: 'status', state: 'idle' }) })
    expect(store.processing).toBe(false)
    socket.onmessage?.({ data: JSON.stringify({ type: 'error', detail: 'failed' }) })
    expect(store.messages.at(-1)?.content).toBe('[Error] failed')

    store.disconnect()
    expect(socket.closed).toBe(true)
    expect(store.connected).toBe(false)
  })

  it('handles malformed JSON without throwing', () => {
    const store = useWebSocket()
    store.connect()
    const socket = MockWebSocket.instances[0]

    expect(() => socket.onmessage?.({ data: '{bad json' })).not.toThrow()
    expect(store.messages.at(-1)?.content).toBe('[Error] Invalid server message')
    expect(store.processing).toBe(false)
  })
})
