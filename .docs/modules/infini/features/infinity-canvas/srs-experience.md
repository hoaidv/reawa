---
feature: infinity-canvas
parent_req: [REQ-01]
version: 0.2.0
lifecycle: active
owner: pm
co_author: designer
purpose: PRD → technical bridge — journeys for Infini infinity canvas
---

# SRS — Infinity canvas (Experience)

**Authority:** [REQ-01](../../prd.md#infinity-canvas) → this file → [srs-ui](./srs-ui.md) /
[srs-logic](./srs-logic.md).

## Capability narrative

Infini opens as a quiet desktop window whose **entire client area is the canvas**. The artist
pans and zooms an infinite world with trackpad, mouse, and keyboard-modified wheel — no tool
palette, no cards. Content (or emptiness) lives under a translate + uniform-scale transform;
optional chrome is only a small zoom readout.

## Entry context

| Field | Value |
|---|---|
| Persona / role | Creator on macOS (Electron Infini) |
| Situation / when | Starting or continuing a drawing session; viewing vectors |
| Trigger | Launch Infini / focus main window |
| Preconditions | App shell running ([ADR-0008](../../../../adr/ADR-0008-electron-react-infini.md)) |

## Primary journeys

### Journey: `journey.navigate` — Pan and zoom the world

- **Realizes:** [REQ-01] AC (gestures + transform)
- **Success end-state:** Artist has moved the drawing region; zoom % reflects scale; circle stays circular

| Step | Beat | scene_id or in-scene | Notes |
|---|---|---|---|
| 1 | See canvas (empty or with primitives) | `scene.canvas` | states `canvas.empty` or `canvas.populated` |
| 2 | Pan via trackpad / drag / wheel | in-scene `canvas.gesturing` | translate only |
| 3 | Zoom via pinch or ⌘/Ctrl+wheel | in-scene `canvas.gesturing` | uniform scale; focus point |
| 4 | Release → settle | `scene.canvas` | back to empty/populated |

### Journey: `journey.resize` — Resize the window

- **Realizes:** [REQ-01] / SRS-IN-02 `canvas.resized`
- **Success end-state:** World anchor stable at **window center**; no content jump to a corner

| Step | Beat | scene_id or in-scene | Notes |
|---|---|---|---|
| 1 | Drag window edge | `scene.canvas` → `canvas.resized` | Electron frame |
| 2 | View recomputes; center world point stays centered | `scene.canvas` | **Decision: center anchor** |

## Critical alternate journeys

### Journey: `journey.empty` — Empty world still navigable

| Step | Beat | scene_id or in-scene | Notes |
|---|---|---|---|
| 1 | Open with no vectors | `canvas.empty` | Grid/atmosphere only; gestures work |

## Bridge matrix

| Journey step | scene / state | UI | Product AC | Logic |
|---|---|---|---|---|
| navigate.1 | scene.canvas | srs-ui regions | REQ-01 | SRS-IN-01 |
| navigate.2–3 | canvas.gesturing | gesture map | ≤2 dropped frames/s | SRS-IN-01/03 |
| resize.2 | canvas.resized | center anchor | REQ-01 | SRS-IN-01 inverse |
| empty.1 | canvas.empty | empty atmosphere | REQ-01 | vector count = 0 |

## Anti-invent / out-of-journey

- No on-canvas toolbars, brush docks, layer panels, or FAB.
- No mobile bottom-nav / tab bar.
- Document open/save chrome belongs to **vector-document** (REQ-02), not this feature.
