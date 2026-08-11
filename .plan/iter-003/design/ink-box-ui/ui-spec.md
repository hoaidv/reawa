---
id: UI-IN-02
screen: ink-box-ui
module: infini
parent_srs: [SRS-IN-14]
parent_req: [REQ-04]
story: STORY-IN-013
fidelity: hifi
platform: desktop
version: 0.1.0
---

# UI Spec — Infini ink-box tools + selection overlay `[UI-IN-02]`

## Platform profile

| Field | Value |
|---|---|
| Profile | **desktop** (Electron; macOS first) |
| `data-platform` | `desktop` |
| Input | pointer + keyboard; hover + focus-visible required |
| Responsive | Window resize; world anchor = center (inherits infinity-canvas) |
| Preview scale (navigator) | `desktop` @ 100% |

## Layout regions (`data-region`)

| Region | Parent | Notes |
|---|---|---|
| WindowFrame | screen | Client area |
| ToolStrip | WindowFrame | Leading dock — Selection · Ink-box only (Infini; no Pen) |
| CanvasStage | WindowFrame | Full-bleed gesture surface |
| WorldLayer | CanvasStage | Tree paint + boundary/content ink |
| SelectionOverlay | CanvasStage | Canvas-space bounds, handles, mode toggle, create CTA |
| BoundsHint | SelectionOverlay | Optional extent chrome — never reads as ink |
| StatusZoom | WindowFrame | Top-trailing zoom `%` |

## Scenes (⊆ SRS states + journey refuse)

| State id | File | Notes |
|---|---|---|
| `tool.selection.idle` | `ink-box-ui-selection-idle.html` | **hifi primary** — Selection armed, overlay hidden |
| `tool.selection.selected` | `ink-box-ui-selection-selected.html` | Bounds + handles + mode toggle |
| `tool.selection.dragging` | `ink-box-ui-selection-dragging.html` | Bounds only while moving |
| `tool.ink_box.armed` | `ink-box-ui-ink-box-armed.html` | Ink-box tool active |
| `manipulation.unavailable` | `ink-box-ui-manipulation-unavailable.html` | Below LOD |
| `tool.selection.create_refused` | `ink-box-ui-create-refused.html` | Journey `select_create_refuse` (SRS-IN-16) |
| states showcase | `ink-box-ui-states.html` | Control × state |

## Components

| Name | Kind | Path |
|---|---|---|
| ToolStrip | screen | `components/tool-strip.html` |
| SelectionOverlay | screen | `components/selection-overlay.html` |
| InkScaleToggle | screen | `components/ink-scale-toggle.html` |
| CreateSmartGroupCta | screen | `components/create-smart-group-cta.html` |
| BoundsHint | screen | `components/bounds-hint.html` |

## Icons / assets

| Icon | Path | Notes |
|---|---|---|
| Selection tool | `../system/assets/icon-tool-selection.svg` | pointer/arrow |
| Ink-box tool | `../system/assets/icon-tool-ink-box.svg` | rect enclose |
| Fixed-ink mode | `../system/assets/icon-mode-fixed-ink.svg` | pad lock feel |
| With-bounds mode | `../system/assets/icon-mode-with-bounds.svg` | scale feel |

## Trace matrix

| Region / state | SRS | Story AC |
|---|---|---|
| ToolStrip tools | SRS-IN-14 inventory | STORY-IN-013 |
| SelectionOverlay + handles | SRS-IN-14 / SRS-IN-11 | STORY-IN-013 |
| inkScaleMode toggle | SRS-IN-14 `tgl.ink_scale_mode` | STORY-IN-013 |
| create refused | SRS-IN-16 · journey.select_create_refuse | STORY-IN-013 |
| manipulation.unavailable | SRS-IN-14 states | STORY-IN-013 |
| No rotation handle | SRS-IN-14 anti-pattern | STORY-IN-013 |

## SRS delta

| Topic | SRS | Spec/HTML | Gap |
|---|---|---|---|
| Composition layers | ToolStrip / Overlay / BoundsHint | all present | none |
| Closed controls | 9 ids | all mapped | none |
| Platform desktop | declared | `data-platform=desktop` | none |
| Create refused | interaction + journey | dedicated scene | none (not in SRS states matrix — journey-backed) |
| No Pen on Infini | experience anti-invent | ToolStrip has 2 tools | none |
| No rotation | anti-pattern | 4 corner handles only | none |
| Nav kinds | single-flow scenes | N/A hops | none |

## Interaction notes (binding)

- Overlay is **canvas-space** (transforms with WorldLayer) — not a DOM panel.
- Boundary ink is the box; BoundsHint must stay visually subordinate (dashed / low contrast).
- Properties panel banned — only `tgl.ink_scale_mode` on the overlay.
