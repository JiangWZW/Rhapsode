<script setup lang="ts">
import { onMounted, onUnmounted } from 'vue'
import { useWebSocket } from '../stores/websocket'
import MessageList from './MessageList.vue'
import InputBar from './InputBar.vue'

const store = useWebSocket()
onMounted(() => store.connect())
onUnmounted(() => store.disconnect())
</script>

<template>
  <div class="chat-view">
    <header>
      <h1>Rhapsode</h1>
      <span :class="['status', { on: store.connected }]">
        {{ store.connected ? 'connected' : 'disconnected' }}
      </span>
    </header>
    <MessageList :messages="store.messages" :processing="store.processing" />
    <InputBar :disabled="!store.connected || store.processing" @send="store.send" />
  </div>
</template>

<style scoped>
.chat-view {
  display: flex;
  flex-direction: column;
  height: 100vh;
  max-width: 800px;
  margin: 0 auto;
}

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.75rem 1rem;
  border-bottom: 1px solid #2a2a4a;
}

header h1 {
  font-size: 1.1rem;
  font-weight: 600;
  color: #b0b0d0;
}

.status {
  font-size: 0.75rem;
  color: #666;
}
.status.on {
  color: #5c8;
}
</style>
