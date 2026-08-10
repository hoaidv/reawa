---
id: UI-IN-01
screen: infinity-canvas
module: infini
parent_srs: [SRS-IN-02]
parent_req: [REQ-01]
story: STORY-IN-001
fidelity: hifi
platform: desktop
version: 0.1.0
---

# UI Spec — Infini infinity canvas `[UI-IN-01]`

## Platform profile

| Field | Value |
|---|---|
| Profile | **desktop** (Electron; macOS first) |
| `data-platform` | `desktop` |
| Input | pointer + keyboard; hover required |
| Responsive | Window resize only; **world anchor = center** |
| Preview scale (navigator) | `desktop` @ 100% |

## Layout regions (`data-region`)

| Region | Parent | Notes |
|---|---|---|
| WindowFrame | screen | Client area |
| CanvasStage | WindowFrame | Full-bleed gesture surface |
| WorldLayer | CanvasStage | Grid + figures; CSS transform |
| StatusZoom | WindowFrame | Top-trailing zoom `%` |

## Scenes (⊆ SRS states)

| State id | File | Notes |
|---|---|---|
| `canvas.empty` | `infinity-canvas-empty.html` | Default launch |
| `canvas.populated` | `infinity-canvas-populated.html` | **hifi primary** |
| `canvas.gesturing` | `infinity-canvas-gesturing.html` | Grabbing + live zoom |
| `canvas.resized` | `infinity-canvas-resized.html` | Narrower frame; center preserved |
| states showcase | `infinity-canvas-states.html` | Control × state |

## Components

| Name | Kind | Path |
|---|---|---|
| CanvasStage | screen | `components/canvas-stage.html` |
| ZoomReadout | screen | `components/zoom-readout.html` |
| WorldLayer | screen | `components/world-layer.html` |

## Icons / assets

| Icon | Path | Notes |
|---|---|---|
| — | — | **None** — SRS inventory has no toolbar icons in v0 |

## Trace matrix

| Region / state | SRS | Story AC |
|---|---|---|
| CanvasStage / all states | SRS-IN-02 | STORY-IN-001 |
| StatusZoom | SRS-IN-02 chrome.status | STORY-IN-001 |
| Gestures annotated | SRS-IN-02 gesture map | STORY-IN-001 |
| Center resize | SRS-IN-02 open option | STORY-IN-001 |

## SRS delta

| Topic | SRS | Spec/HTML | Gap |
|---|---|---|---|
| Composition layers | CanvasBackdrop/World/Status | honored | none |
| Controls | canvas + zoom readout | both | none |
| Platform desktop | declared | `data-platform=desktop` | none |
| Resize anchor | center locked | annotated on resized scene | none |
| Nav kinds | N/A single scene | N/A | none |

## Gesture legend (annotate on populated + gesturing)

Trackpad pan · mouse drag pan · wheel pan · ⌘/Ctrl+wheel zoom · pinch zoom
