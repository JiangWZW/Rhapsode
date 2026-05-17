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
      placeholder="Describe your action..."
      autofocus
    />
    <button :disabled="disabled || !text.trim()">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <line x1="22" y1="2" x2="11" y2="13"/>
        <polygon points="22 2 15 22 11 13 2 9 22 2"/>
      </svg>
      <span>Send</span>
    </button>
  </form>
</template>

<style scoped>
.input-bar {
  display: flex;
  gap: 0.75rem;
}

input {
  flex: 1;
  padding: 0.85rem 1.25rem;
  background: #f9fafb;
  border: 2px solid #e5e7eb;
  border-radius: 0.75rem;
  color: #111827;
  font-size: 1rem;
  outline: none;
  transition: border-color 0.15s, box-shadow 0.15s;
}

input::placeholder {
  color: #9ca3af;
}

input:focus {
  border-color: #f59e0b;
  box-shadow: 0 0 0 3px rgba(245, 158, 11, 0.15);
}

button {
  padding: 0.85rem 1.75rem;
  background: #d97706;
  color: #fff;
  border: none;
  border-radius: 0.75rem;
  cursor: pointer;
  font-size: 1rem;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 0.5rem;
  transition: background 0.15s, transform 0.1s;
  box-shadow: 0 4px 12px rgba(217, 119, 6, 0.35);
}

button svg {
  width: 1.1rem;
  height: 1.1rem;
}

button:not(:disabled):hover {
  background: #b45309;
}

button:not(:disabled):active {
  transform: scale(0.97);
}

button:disabled {
  opacity: 0.45;
  cursor: not-allowed;
  box-shadow: none;
}
</style>
