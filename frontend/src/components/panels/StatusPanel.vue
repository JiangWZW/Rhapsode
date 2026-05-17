<script setup lang="ts">
defineProps<{
  connected: boolean
  processing: boolean
}>()
</script>

<template>
  <header class="status-header">
    <div class="brand">
      <svg class="book-icon" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/>
        <path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/>
      </svg>
      <div class="title-group">
        <h1>Rhapsode</h1>
        <p>An AI-Powered Adventure</p>
      </div>
    </div>
    <div class="states">
      <span :class="['badge', connected ? 'online' : 'offline']">
        {{ connected ? 'connected' : 'disconnected' }}
      </span>
      <span :class="['badge', processing ? 'busy' : 'idle']">
        {{ processing ? 'processing' : 'idle' }}
      </span>
    </div>
  </header>
</template>

<style scoped>
.status-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 1.5rem 0 0.5rem;
}

.brand {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.book-icon {
  width: 2.5rem;
  height: 2.5rem;
  color: #92400e;
  flex-shrink: 0;
}

.title-group h1 {
  font-size: 2.25rem;
  font-weight: 600;
  color: #78350f;
  line-height: 1.1;
  letter-spacing: -0.01em;
}

.title-group p {
  font-size: 1rem;
  color: #b45309;
  font-style: italic;
  margin-top: 0.1rem;
}

.states {
  display: flex;
  gap: 0.5rem;
}

.badge {
  font-size: 0.72rem;
  border-radius: 999px;
  padding: 0.25rem 0.75rem;
  border: 1px solid transparent;
  font-weight: 500;
  letter-spacing: 0.03em;
}

.badge.online {
  background: #d1fae5;
  color: #065f46;
  border-color: #a7f3d0;
}

.badge.offline {
  background: #fee2e2;
  color: #991b1b;
  border-color: #fecaca;
}

.badge.busy {
  background: #fef3c7;
  color: #92400e;
  border-color: #fde68a;
}

.badge.idle {
  background: #ede9fe;
  color: #5b21b6;
  border-color: #ddd6fe;
}
</style>
