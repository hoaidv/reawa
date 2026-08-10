---
feature: infinity-canvas
parent_req: [REQ-01]
version: 0.2.0
lifecycle: active
needs_design: true
---

# SRS — Infinity canvas (UI)

Durable UI contract for `/designer` and `/dev`. Thickened 2026-08-10 (PM) for STORY-IN-001.

## [SRS-IN-02] Canvas chrome and gestures

**Logic:** [SRS-IN-01](./srs-logic.md). **Quality:** [SRS-IN-03](./srs-quality.md).
**Experience:** [srs-experience](./srs-experience.md).

---

### Design authority

1. This `srs-ui.md` (closed inventory + composition)
2. `.docs/DESIGN.md` + tokens when present (else package tokens)
3. Human brief / story AC
4. Reference images — atmosphere only; never invent chrome

---

### Purpose

**Navigate an infinite 2D world** inside a desktop window using pan and zoom; show empty or
populated content under translate + uniform scale. One job: navigation + correct transform —
not editing tools.

---

### Composition layers (binding)

| Layer id | Scroll? | Role | Fill |
|---|---|---|---|
| WindowChrome | Fixed | OS/Electron titlebar (system); not designed here | System |
| CanvasBackdrop | Fixed | Infinite-world atmosphere (grid / paper tone) filling client area | Full bleed |
| CanvasWorld | Fixed (transforms) | World content drawn with translate + scale; does not document-scroll | Transparent over backdrop |
| StatusChrome | Fixed | Optional zoom % — corner overlay, non-modal | Minimal label |

**Scroll policy:** There is **no document scroll**. Pan/zoom mutate the canvas transform.
Window resize recomputes the view; **world anchor = window center** (locked).

---

### Layout regions (containment tree)

| # | Region id | Parent | Contents | Component |
|---|---|---|---|---|
| 0 | WindowFrame | screen | Electron client area | — structural |
| 1 | CanvasStage | WindowFrame | Full-bleed interactive canvas | CanvasStage |
| 2 | WorldLayer | CanvasStage | Grid + primitives under transform | WorldLayer |
| 3 | StatusZoom | WindowFrame | Zoom percentage readout | ZoomReadout |

**Containment:** `StatusZoom` is a child of `WindowFrame`, not inside `WorldLayer` (does not pan away).
No cards, side panels, or floating promo chips.

---

### Seam / overlap rules

- `CanvasStage` is edge-to-edge in the client area (flush to window chrome).
- `StatusZoom` overlays top-trailing corner with safe padding (`spacing.md`); never covers >5% of stage.
- No negative-margin tuck; no modal sheets.

---

### Chrome relationships

| Pair | Relationship |
|---|---|
| CanvasStage ↔ WindowFrame | **seamless** — canvas is the composition |
| StatusZoom ↔ CanvasStage | **contrastive** — small readable chrome over busy world |
| WorldLayer ↔ CanvasBackdrop | **same-family** — grid is part of the world |

---

### Open options

| Option | Choice | Notes |
|---|---|---|
| Resize world anchor | **Center** (locked) | Reject top-left for v0 |
| Empty atmosphere | Soft grid on quiet paper tone | Not purple gradient; not cream+terracotta cliché |
| Zoom readout | Always visible when focused | Format `NN%` |

---

### Closed control inventory

| id | Control | Region | Notes |
|---|---|---|---|
| `ctrl.canvas` | CanvasStage (gesture surface) | CanvasStage | Primary; pointer + keyboard focusable |
| `ctrl.zoom_readout` | ZoomReadout | StatusZoom | Readout; not a button in v0 |

---

### Copy table (en)

| id | String |
|---|---|
| `copy.zoom` | `{n}%` |
| `copy.empty_hint` | Pan and zoom — trackpad, drag, or wheel |
| `copy.app_name` | Infini |

---

### Interaction map

| Control | Action | Destination | Chrome side-effect | Feedback |
|---|---|---|---|---|
| `ctrl.canvas` | Trackpad two-finger pan | in-scene translate | none | Cursor `grab` → `grabbing`; world moves continuously |
| `ctrl.canvas` | Mouse drag on background | in-scene translate | none | Same as pan; `:active` press on stage |
| `ctrl.canvas` | Wheel | in-scene pan | none | Immediate translate |
| `ctrl.canvas` | Modifier + wheel | in-scene zoom | StatusZoom updates | Scale about cursor; readout live |
| `ctrl.canvas` | Trackpad pinch | in-scene zoom | StatusZoom updates | Same |
| `ctrl.canvas` | Keyboard focus | — | focus ring on stage | `:focus-visible` outline inset |
| `ctrl.zoom_readout` | (none — display) | — | — | N/A interactive; updates live during gesture |

---

### Control states

| Control | hover | focus-visible | active/press | disabled | loading | selected | error | empty |
|---|---|---|---|---|---|---|---|---|
| `ctrl.canvas` | yes (grab cursor) | yes (inset ring) | yes (grabbing) | N/A | N/A | N/A | N/A | shows empty hint when no vectors |
| `ctrl.zoom_readout` | N/A | N/A | N/A | N/A | N/A | N/A | N/A | N/A |

---

### Transitions / motion intent

- Pan/zoom: **direct**, vsync-aligned; no ease that lags the finger (>1 frame feel).
- StatusZoom value: update every frame during gesture; no fade flicker.
- Resize: instantaneous recompute; no animated zoom pop.

---

### States matrix

| State id | When | UI |
|---|---|---|
| `canvas.empty` | No document vectors | Grid + empty hint; gestures live |
| `canvas.populated` | ≥1 primitive/stroke | Grid + figures under transform |
| `canvas.gesturing` | Pan/zoom in progress | Continuous transform; readout live; no modal |
| `canvas.resized` | Window size changing/changed | Same content; center world point preserved |

Designer ships **one scene HTML per state id** (plus states showcase).

---

### Visual hierarchy / density

1. World content / grid (dominant)
2. StatusZoom (secondary)
3. Optional empty hint (tertiary, center-low, fades when populated)

**Platform targets:** desktop compact; interactive stage is the full window (≥24px targets N/A for stage itself).

---

### Platform profile

| Field | Value |
|---|---|
| Profile | **desktop** (Electron app; macOS first) |
| Input model | pointer + keyboard |
| Nav paradigm | window / no app tab bar |
| Target min | ≥24px for any future chrome controls |
| Density | compact |
| Hover | **required** |
| Responsive | Window resize only — not mobile web breakpoints |
| `data-platform` | `desktop` |

---

### Anti-patterns

- Cards, pill clusters, stat strips, floating badges on the canvas
- Purple-on-white / purple-indigo gradient themes; warm cream + terracotta + serif display cliché
- Inset “hero image” panels — canvas is full-bleed
- Hover-only gesture discovery without keyboard/focus path for focusable stage

### Out of scope

- Document open/save (REQ-02)
- Tablet sync chrome (REQ-03)
- Brush / layer / color tools
- On-device Epaper UI

---

### Multi-scene graph

**N/A** — single Keep scene `scene.canvas` with four **states** (not separate pushed scenes).
No modals/sheets.
