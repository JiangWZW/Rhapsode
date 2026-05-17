<script setup lang="ts">
interface AvatarItem {
  name: string
  role: string
}

defineProps<{
  avatars: AvatarItem[]
}>()
</script>

<template>
  <div class="avatar-panel">
    <h3 class="panel-label">Characters</h3>
    <ul class="avatar-list">
      <li v-for="avatar in avatars" :key="avatar.name" class="avatar-item">
        <div class="avatar-circle" aria-hidden="true">{{ avatar.name[0] }}</div>
        <div class="avatar-info">
          <strong>{{ avatar.name }}</strong>
          <span>{{ avatar.role }}</span>
        </div>
      </li>
    </ul>
  </div>
</template>

<style scoped>
.avatar-panel {
  padding: 1rem 1.25rem 1.25rem;
  border-top: 1px solid #e5e7eb;
}

.panel-label {
  font-size: 0.7rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: #9ca3af;
  margin-bottom: 0.75rem;
}

.avatar-list {
  list-style: none;
  padding: 0;
  margin: 0;
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.avatar-item {
  display: flex;
  align-items: center;
  gap: 0.65rem;
}

.avatar-circle {
  width: 36px;
  height: 36px;
  border-radius: 999px;
  background: #fde68a;
  border: 2px solid #d97706;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 0.9rem;
  font-weight: 700;
  color: #78350f;
  flex-shrink: 0;
}

.avatar-info {
  display: flex;
  flex-direction: column;
}

.avatar-info strong {
  font-size: 0.85rem;
  font-weight: 600;
  color: #1e293b;
}

.avatar-info span {
  font-size: 0.72rem;
  color: #6b7280;
}
</style>
