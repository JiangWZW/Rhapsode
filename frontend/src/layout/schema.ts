export type LayoutRegion = 'top' | 'left' | 'main' | 'right' | 'bottom'

export type PanelId =
  | 'status'
  | 'story'
  | 'conversation'

export interface PanelLayout {
  id: PanelId
  region: LayoutRegion
  order: number
  visible: boolean
  minHeight?: number
  minWidth?: number
}

export const defaultLayout: PanelLayout[] = [
  { id: 'status',       region: 'top',    order: 0, visible: true },
  { id: 'story',        region: 'main',   order: 0, visible: true },
  { id: 'conversation', region: 'bottom', order: 0, visible: true },
]
