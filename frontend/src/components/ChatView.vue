<script setup lang="ts">
import { onMounted, onUnmounted, type Component } from 'vue'
import { useWebSocket } from '../stores/websocket'
import { useLayoutStore } from '../stores/layout'
import type { PanelId } from '../layout/schema'
import ConversationPanel from './panels/ConversationPanel.vue'
import StatusPanel from './panels/StatusPanel.vue'
import StoryPanel from './panels/StoryPanel.vue'

const socket = useWebSocket()
const layout = useLayoutStore()

onMounted(() => socket.connect())
onUnmounted(() => socket.disconnect())

const panelComponents: Record<PanelId, Component> = {
  status:       StatusPanel,
  story:        StoryPanel,
  conversation: ConversationPanel,
}

function panelProps(id: PanelId) {
  if (id === 'status') {
    return { connected: socket.connected, processing: socket.processing }
  }
  if (id === 'story') {
    return { messages: socket.messages, processing: socket.processing }
  }
  if (id === 'conversation') {
    return {
      disabled: !socket.connected || socket.processing,
      onSend: socket.send,
    }
  }
  return {}
}
</script>

<template>
  <div class="page">
    <div class="shell">
      <!-- Top header -->
      <component
        v-for="panel in layout.byRegion('top')"
        :is="panelComponents[panel.id]"
        :key="panel.id"
        v-bind="panelProps(panel.id)"
      />

      <!-- Main card: story only -->
      <div class="main-card">
        <component
          v-for="panel in layout.byRegion('main')"
          :is="panelComponents[panel.id]"
          :key="panel.id"
          v-bind="panelProps(panel.id)"
        />
      </div>

      <!-- Bottom: action input -->
      <div class="bottom-card">
        <component
          v-for="panel in layout.byRegion('bottom')"
          :is="panelComponents[panel.id]"
          :key="panel.id"
          v-bind="panelProps(panel.id)"
        />
      </div>
    </div>
  </div>
</template>

<style scoped>
.page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f8f6f0 0%, #eef1f7 100%);
  padding: 2rem 1rem 3rem;
  overflow-y: auto;
}

.shell {
  max-width: 1280px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
}

.main-card {
  background: #ffffff;
  border-radius: 1.25rem;
  box-shadow: 0 8px 40px rgba(0, 0, 0, 0.1), 0 2px 8px rgba(0, 0, 0, 0.06);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  min-height: 600px;
}

.bottom-card {
  background: #ffffff;
  border-radius: 1.25rem;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
  overflow: hidden;
}

</style>
