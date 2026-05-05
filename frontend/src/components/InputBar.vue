<script setup lang="ts">
import { ref } from 'vue'

defineProps<{ disabled: boolean }>()
const emit = defineEmits<{ send: [text: string] }>()

const text = ref('')

function submit() {
  const trimmed = text.value.trim()
  if (!trimmed) return
  emit('send', trimmed)
  text.value = ''
}
</script>

<template>
  <form class="input-bar" @submit.prevent="submit">
    <input
      v-model="text"
      :disabled="disabled"
      placeholder="What do you do?"
      autofocus
    />
    <button :disabled="disabled || !text.trim()">Send</button>
  </form>
</template>

<style scoped>
.input-bar {
  display: flex;
  gap: 0.5rem;
  padding: 0.75rem 1rem;
  border-top: 1px solid #2a2a4a;
}

input {
  flex: 1;
  padding: 0.5rem 0.75rem;
  background: #16162b;
  border: 1px solid #3a3a5a;
  border-radius: 0.4rem;
  color: #e0e0e0;
  font-size: 0.95rem;
  outline: none;
}
input:focus {
  border-color: #5a7aba;
}

button {
  padding: 0.5rem 1rem;
  background: #3a5a8a;
  color: #e8eef8;
  border: none;
  border-radius: 0.4rem;
  cursor: pointer;
  font-size: 0.9rem;
}
button:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
button:not(:disabled):hover {
  background: #4a6a9a;
}
</style>
