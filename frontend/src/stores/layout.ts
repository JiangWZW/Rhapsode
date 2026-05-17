import { computed, ref } from 'vue'
import { defineStore } from 'pinia'
import { defaultLayout, type LayoutRegion, type PanelId, type PanelLayout } from '../layout/schema'

export const useLayoutStore = defineStore('layout', () => {
  const panels = ref<PanelLayout[]>(defaultLayout.map((panel) => ({ ...panel })))

  const orderedVisiblePanels = computed(() => {
    return panels.value
      .filter((panel) => panel.visible)
      .slice()
      .sort((a, b) => a.order - b.order)
  })

  function byRegion(region: LayoutRegion): PanelLayout[] {
    return orderedVisiblePanels.value.filter((panel) => panel.region === region)
  }

  function setPanelVisibility(id: PanelId, visible: boolean) {
    const panel = panels.value.find((item) => item.id === id)
    if (panel) panel.visible = visible
  }

  function setPanelRegion(id: PanelId, region: LayoutRegion) {
    const panel = panels.value.find((item) => item.id === id)
    if (panel) panel.region = region
  }

  function setPanelOrder(id: PanelId, order: number) {
    const panel = panels.value.find((item) => item.id === id)
    if (panel) panel.order = order
  }

  function resetLayout() {
    panels.value = defaultLayout.map((panel) => ({ ...panel }))
  }

  return {
    panels,
    orderedVisiblePanels,
    byRegion,
    setPanelVisibility,
    setPanelRegion,
    setPanelOrder,
    resetLayout,
  }
})
