<script setup lang="ts">
defineProps<{
  imageUrl: string
  title: string
}>()
</script>

<template>
  <div class="scene-panel">
    <div class="image-frame">
      <img :src="imageUrl" :alt="title" />
    </div>
    <div class="caption-bar">
      <p>{{ title }}</p>
    </div>
  </div>
</template>

<style scoped>
.scene-panel {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.image-frame {
  flex: 1;
  border: 4px solid #1e293b;
  margin: 1.25rem 1.25rem 0;
  border-radius: 0.5rem;
  overflow: hidden;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.18);
  min-height: 260px;
}

img {
  width: 100%;
  height: 100%;
  object-fit: cover;
  display: block;
}

.caption-bar {
  background: #1e293b;
  margin: 0 1.25rem 1.25rem;
  border-radius: 0 0 0.375rem 0.375rem;
  padding: 0.5rem 1rem;
}

.caption-bar p {
  color: #f1f5f9;
  font-size: 0.85rem;
  font-style: italic;
  text-align: center;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}
</style>
