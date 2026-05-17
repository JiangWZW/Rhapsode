<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import type { ChatMessage } from '../../stores/websocket'
import { parseScene } from '../../utils/sceneTextParser'

const props = defineProps<{
  messages: ChatMessage[]
  processing: boolean
}>()

const el = ref<HTMLElement>()

watch(() => props.messages.length, async () => {
  await nextTick()
  el.value?.scrollTo({ top: el.value.scrollHeight, behavior: 'smooth' })
})
</script>

<template>
  <div class="story-panel">
    <div class="story-header">
      <span class="location-tag">Chapter I · The Unfolding</span>
      <h2 class="scene-title">The Story So Far</h2>
    </div>
    <div ref="el" class="story-scroll">
      <div v-if="messages.length === 0" class="narration-block">
        <p class="sp">The adventure awaits. Type below to begin your journey...</p>
      </div>
      <template v-for="(msg, i) in messages" :key="i">
        <!-- NPC spoken line(s) synthesized locally -->
        <div v-if="msg.role === 'assistant' && msg.scene_kind === 'character'" class="dialogue-block">
          <div class="dialogue-label">{{ msg.speaker || 'Character' }}</div>
          <div class="dialogue-body" v-html="parseScene(msg.content)" />
        </div>
        <!-- Narrator / error / legacy assistant prose -->
        <div
          v-else-if="msg.role === 'assistant'"
          class="narration-block"
          v-html="parseScene(msg.content)"
        />
        <!-- Player action -->
        <div v-else class="action-block">
          <div class="action-label">YOU</div>
          <div class="action-bubble">
            <p>{{ msg.content }}</p>
          </div>
        </div>
      </template>
      <div v-if="processing" class="narration-block thinking">
        <p class="sp">The narrator ponders...</p>
      </div>
    </div>
  </div>
</template>

<style scoped>
.story-panel {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-height: 0;
  overflow: hidden;
}

.story-header {
  padding: 1.25rem 2rem 1rem;
  border-bottom: 1px solid #e7e5e4;
  text-align: center;
  background: transparent;
  font-family: 'Inter', system-ui, sans-serif;
}

.location-tag {
  display: block;
  font-size: 0.68rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: #a8a29e;
  margin-bottom: 0.3rem;
}

.scene-title {
  font-size: 1.25rem;
  font-weight: 600;
  color: #292524;
  line-height: 1.3;
}

.story-scroll {
  flex: 1;
  overflow-y: auto;
  padding: 2rem 2.5rem;
  display: flex;
  flex-direction: column;
  gap: 2rem;
  background: transparent;
}

/* Narration block — clean flowing text, no boxes */
.narration-block {
  /* No border, no background — pure text flow */
}

/* Parser-generated paragraph — Lora serif, upright, muted stone */
.narration-block :deep(.sp) {
  font-family: 'Lora', Georgia, serif;
  font-size: 1.0625rem;
  line-height: 1.9;
  font-style: normal;
  color: #57534e;
  margin-bottom: 0.95em;
  display: block;
}

.narration-block :deep(.sp:last-child) {
  margin-bottom: 0;
}

/* Dialogue quotes — upright, darkest text, clearly pops from muted narration */
.narration-block :deep(.sq) {
  color: #1c1917;
  font-style: normal;
  font-weight: 500;
}

/* *Emphasis* — markdown-it renders *text* as <em>; warm amber accent */
.narration-block :deep(em) {
  color: #b45309;
  font-style: italic;
}

/* **Bold** — markdown-it renders **text** as <strong> */
.narration-block :deep(strong) {
  color: #292524;
  font-weight: 600;
}

/* (Parenthetical) — italic muted aside, now that prose is upright */
.narration-block :deep(.paren) {
  color: #a8a29e;
  font-style: italic;
  font-size: 0.94em;
}

/* [Bracket note] — dark amber, system annotation */
.narration-block :deep(.bk) {
  color: #92400e;
  font-style: normal;
  font-weight: 600;
  font-family: 'Inter', system-ui, sans-serif;
  font-size: 0.93em;
}

.narration-block.thinking :deep(.sp) {
  color: #a8a29e;
  font-style: italic;
  animation: pulse 1.4s ease-in-out infinite;
}

/* Player action block — lightweight, right-aligned */
.action-block {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  gap: 0.3rem;
}

.action-label {
  font-family: 'Inter', system-ui, sans-serif;
  font-size: 0.65rem;
  font-weight: 700;
  letter-spacing: 0.14em;
  color: #a78bfa;
  text-transform: uppercase;
}

.action-bubble {
  background: #fffbeb;
  border: 1px solid #d97706;
  border-radius: 0.5rem;
  padding: 0.6rem 1rem;
  max-width: 85%;
}

.action-bubble p {
  font-family: 'Inter', system-ui, sans-serif;
  color: #292524;
  font-size: 0.9875rem;
  line-height: 1.65;
  font-style: normal;
}

.dialogue-block {
  border-left: 3px solid #a78bfa;
  padding: 0.35rem 0 0.35rem 1rem;
  margin-left: 0;
}

.dialogue-label {
  font-family: 'Inter', system-ui, sans-serif;
  font-size: 0.68rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  color: #7c3aed;
  text-transform: uppercase;
  margin-bottom: 0.35rem;
}

.dialogue-body :deep(.sp) {
  font-family: 'Lora', Georgia, serif;
  font-size: 1rem;
  line-height: 1.75;
  color: #1c1917;
  font-weight: 500;
  margin-bottom: 0.5em;
}

.dialogue-body :deep(.sp:last-child) {
  margin-bottom: 0;
}

@keyframes pulse {
  0%, 100% { opacity: 0.35; }
  50% { opacity: 0.85; }
}
</style>
