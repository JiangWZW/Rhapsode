---
sources:
  - frontend/src/App.vue
  - frontend/src/components/ChatView.vue
  - frontend/src/components/panels/StatusPanel.vue
  - frontend/src/components/panels/StoryPanel.vue
  - frontend/src/components/panels/ConversationPanel.vue
  - frontend/src/components/InputBar.vue
  - frontend/src/stores/websocket.ts
  - frontend/src/stores/layout.ts
  - frontend/src/layout/schema.ts
  - frontend/src/utils/sceneTextParser.ts
  - frontend/package.json
last_updated: 2026-05-17
confidence: verified
tier: semantic
related:
  - "[[architecture/stack]]"
  - "[[architecture/python-server]]"
tags:
  - vue-frontend
---

# Vue frontend

The frontend is a Vue 3 single-page application in `frontend/`. It is a thin WebSocket client with a panel-based layout — all game logic lives in the C++ core and Python server.

## Tech stack

| Tool | Purpose |
|------|---------|
| Vue 3 (Composition API) | UI framework |
| TypeScript | Type safety |
| Pinia | State management (WebSocket + layout stores) |
| Vite | Dev server + production bundler |
| markdown-it | Scene text rendering with custom inline rules |
| Scoped CSS | Component styling (no framework) |

## Component tree

```
App.vue
  └── ChatView.vue
        ├── StatusPanel.vue        (region: top)
        ├── StoryPanel.vue         (region: main)
        →     └── uses parseScene() from sceneTextParser.ts
        └── ConversationPanel.vue  (region: bottom)
              └── InputBar.vue
```

### App.vue

Root component. Imports global styles and mounts ChatView.

### ChatView.vue

Layout shell using a panel-based system. Manages the WebSocket lifecycle:

- Connects on mount, disconnects on unmount
- Reads panel configuration from the layout store
- Dynamically renders panels by region (top, main, bottom) using `<component :is>`
- Passes props to each panel: status gets connection/processing state, story gets messages, conversation gets disabled state and send handler

The layout uses a centered shell (max-width 1280px) with the story panel in a card with rounded corners and shadow, and the conversation input in a separate bottom card.

### StatusPanel.vue

Brand header with connection and processing state badges. Shows "Connected" (green) / "Disconnected" (gray) and a "Processing" indicator when the server is generating.

### StoryPanel.vue

Main narrative transcript panel. Renders messages differently by `scene_kind`:

- **Narrator** messages — rendered through `parseScene()` (markdown-it) with styled prose paragraphs
- **Character** messages — displayed as dialogue bubbles with the speaker's name

Auto-scrolls to the bottom when new messages arrive. Shows a processing indicator while waiting for the server.

### ConversationPanel.vue

Player input section with a "What do you do?" prompt. Contains the InputBar component. Emits `send` events upward.

### InputBar.vue

Text input with send functionality. Emits `send(text)` on submit. Disabled when not connected or while processing. Enter key submits; the input clears after sending.

### Unused stub panels

- **AvatarPanel.vue** — character avatar display (not wired into ChatView)
- **SceneImagePanel.vue** — hero image with caption (not wired into ChatView)
- **MessageList.vue** — plain message list, alternate simple UI (not used by ChatView)

## Layout system

### Schema (`layout/schema.ts`)

Defines the panel configuration types:

```typescript
type LayoutRegion = 'top' | 'left' | 'main' | 'right' | 'bottom'
type PanelId = 'status' | 'story' | 'conversation'

interface PanelLayout {
  id: PanelId
  region: LayoutRegion
  order: number
  visible: boolean
  minHeight?: number
  minWidth?: number
}
```

Default layout places StatusPanel in `top`, StoryPanel in `main`, ConversationPanel in `bottom`.

### Layout store (`stores/layout.ts`)

Pinia store managing panel visibility, region placement, and ordering:

- `byRegion(region)` — returns visible panels in a region, sorted by order
- `setPanelVisibility(id, visible)` — toggle panel
- `setPanelRegion(id, region)` — move panel to different region
- `resetLayout()` — restore defaults

## WebSocket store (`stores/websocket.ts`)

Pinia store managing connection state and message history:

```typescript
interface ChatMessage {
  role: 'user' | 'assistant'
  content: string
  scene_kind?: 'narrator' | 'character'
  speaker?: string
}
```

**Connection:** Auto-detects protocol (`ws` or `wss`) and uses the current host. No hardcoded URL — works behind any proxy.

**Message handling:** Processes server message types:

| Server message type | Store action |
|---------------------|-------------|
| `scene_message` | Push to messages with `scene_kind` and optional `speaker`, clear processing |
| `assistant_message` | Push as narrator message (backward compat) |
| `error` | Push as assistant message with `[Error]` prefix |
| `status` | Set `processing = (state !== 'idle')` |

**Sending:** Pushes a user message to the local array immediately (optimistic), then sends `{ type: "player_message", content }` over the socket.

## Scene text parser (`utils/sceneTextParser.ts`)

Custom markdown-it configuration for rendering narrator prose:

- `html: false` — prevents XSS from LLM output
- `typographer: true` — smart quotes, em-dashes, ellipses
- Custom inline rules (run before typographer):
  - `"dialogue"` → `<span class="sq">` with curly quotes
  - `(aside)` → `<span class="paren">` for parenthetical/whisper text
  - `[note]` → `<span class="bk">` for game/system annotations
- All paragraphs get `class="sp"` for CSS targeting
- `*emphasis*` renders as `<em>` (amber-accented in CSS)

## Dev server

```bash
cd frontend
npm install
npm run dev     # Vite on port 5173
```

Vite proxies `/ws` to `http://127.0.0.1:8080` during development (WebSocket upgrade enabled). In production, the same-origin WebSocket connection works without proxy configuration.

## Styling

Light modern aesthetic with gradient background, card-based layout, rounded corners, and shadows. Narrator prose uses serif-adjacent type treatment with amber emphasis and styled dialogue spans. Character messages are visually distinct from narrator blocks. No CSS framework — the UI is handcrafted.

## What is not built

- **Scenario picker** — loads the default scenario on connection
- **Reconnect logic** — disconnection requires a page refresh
- **Input mode switching** — freeform-only; constrained choices require the full DAG triggers, see [[plot-graph]]
- **Mobile optimization** — desktop-focused
- **Avatar and scene image panels** — stub components exist but are not wired into the active layout
