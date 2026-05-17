<script setup lang="ts">
import InputBar from '../InputBar.vue'

defineProps<{
  disabled: boolean
}>()

const emit = defineEmits<{ send: [text: string] }>()
</script>

<template>
  <div class="action-panel">
    <label class="action-label">What do you do?</label>
    <InputBar :disabled="disabled" @send="emit('send', $event)" />
  </div>
</template>

<style scoped>
.action-panel {
  padding: 1.5rem 2rem 1.75rem;
}

.action-label {
  display: block;
  font-size: 1.0625rem;
  font-weight: 500;
  color: #374151;
  margin-bottom: 0.75rem;
}
</style>
