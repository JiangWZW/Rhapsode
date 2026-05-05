<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'

const props = defineProps<{
  messages: { role: string; content: string }[]
  processing: boolean
}>()

const el = ref<HTMLElement>()

watch(() => props.messages.length, async () => {
  await nextTick()
  el.value?.scrollTo({ top: el.value.scrollHeight, behavior: 'smooth' })
})
</script>

<template>
  <div ref="el" class="message-list">
    <div
      v-for="(msg, i) in messages"
      :key="i"
      :class="['msg', msg.role]"
    >
      {{ msg.content }}
    </div>
    <div v-if="processing" class="msg assistant thinking">...</div>
  </div>
</template>

<style scoped>
.message-list {
  flex: 1;
  overflow-y: auto;
  padding: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.msg {
  max-width: 80%;
  padding: 0.6rem 0.9rem;
  border-radius: 0.6rem;
  line-height: 1.5;
  white-space: pre-wrap;
}

.msg.assistant {
  align-self: flex-start;
  background: #2a2a4a;
  color: #d0d0e8;
}

.msg.user {
  align-self: flex-end;
  background: #3a5a8a;
  color: #e8eef8;
}

.msg.thinking {
  opacity: 0.5;
  animation: pulse 1s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 0.7; }
}
</style>
