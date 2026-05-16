---
sources:
  - frontend/src/App.vue
  - frontend/src/components/ChatView.vue
  - frontend/src/components/MessageList.vue
  - frontend/src/components/InputBar.vue
  - frontend/src/stores/websocket.ts
  - frontend/package.json
last_updated: 2026-05-12
confidence: verified
tier: semantic
related:
  - "[[architecture/stack]]"
  - "[[architecture/python-server]]"
tags:
  - vue-frontend
---

# Vue frontend

The frontend is a Vue 3 single-page application in `frontend/`. It is a thin WebSocket client — all game logic lives in the C++ core and Python server.

## Tech stack

| Tool | Purpose |
|------|---------|
| Vue 3 (Composition API) | UI framework |
| TypeScript | Type safety |
| Pinia | State management (WebSocket store) |
| Vite | Dev server + production bundler |
| Scoped CSS | Component styling (no framework) |

## Component tree

```
App.vue
  └── ChatView.vue
        ├── MessageList.vue
        └── InputBar.vue
```

### App.vue

Root component. Imports global styles and mounts ChatView.

### ChatView.vue

Layout container. Manages the WebSocket lifecycle:

- Connects on mount, disconnects on unmount
- Reads messages and processing state from the WebSocket store
- Passes messages to MessageList, disabled state to InputBar
- Displays connection status in the header — inline, not a separate component

The chat view is centered at max-width 800px with a flex column layout filling the viewport height.

### MessageList.vue

Receives `messages` array and `processing` boolean as props. Renders a scrollable list of messages styled by role:

- `user` messages — right-aligned, distinct background
- `assistant` messages — left-aligned

Shows a processing indicator when the server is generating a response.

### InputBar.vue

Text input with send functionality. Emits `send(text)` on submit. Disabled when not connected or while processing. Enter key submits; the input clears after sending.

## WebSocket store (`stores/websocket.ts`)

Pinia store managing connection state and message history:

```typescript
interface Message {
  role: 'user' | 'assistant'
  content: string
}

export const useWebSocket = defineStore('websocket', () => {
  const messages = ref<Message[]>([])
  const connected = ref(false)
  const processing = ref(false)
  // connect(), send(), disconnect()
})
```

**Connection:** Auto-detects protocol (`ws` or `wss`) and uses the current host. No hardcoded URL — works behind any proxy.

**Message handling:** Processes three server message types:

| Server message type | Store action |
|---------------------|-------------|
| `assistant_message` | Push to messages array, clear processing |
| `error` | Push as assistant message with `[Error]` prefix |
| `status` | Set `processing = (state !== 'idle')` |

**Sending:** Pushes a user message to the local array immediately (optimistic), then sends `{ type: "player_message", content }` over the socket.

## Dev server

```bash
cd frontend
npm install
npm run dev     # Vite on port 5173
```

Vite proxies `/ws` to the FastAPI backend during development. In production, the same-origin WebSocket connection works without proxy configuration.

## Styling

Scoped CSS per component. Dark background, light text, terminal aesthetic. User messages visually distinct from assistant messages. No CSS framework — the UI is minimal by design.

The connection status is a small colored indicator in the header: green when connected, gray when disconnected.

## What is not built

- **Scenario picker** — loads the default scenario on connection
- **ConnectionStatus as a separate component** — inline in ChatView
- **TypeScript interfaces file** — types are defined inline in the store
- **Reconnect logic** — disconnection requires a page refresh
- **Input mode switching** — freeform-only; constrained choices require the full DAG, see [[plot-graph]]
- **Mobile optimization** — desktop-focused
