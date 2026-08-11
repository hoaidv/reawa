---
feature: tool-modes
parent_req: [REQ-03]
version: 0.1.0
lifecycle: active
needs_design: true
---

# SRS — Tool modes Epaper (UI)

Durable UI contract for the on-device toolbar. This is the **first chrome ever placed on the
Epaper panel** — everything before it was full-bleed ink ([region-sync srs-ui](../region-sync/srs-ui.md)).
Logic: [SRS-EP-04](./srs-logic.md). Quality: [SRS-EP-06](./srs-quality.md).
Decision: [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md).

## [SRS-EP-05] On-device tool strip

### Design authority

1. This `srs-ui.md`
2. `epaper` REQ-03 acceptance
3. Physical constraints below (they outrank aesthetics)
4. `.docs/DESIGN.md` tokens — **advisory only**; the desktop design system does not transfer to
   a 1-bit panel

### Purpose

**One job:** let the creator see and change what the pen will do, without ever costing ink
latency or stealing drawing area they need.

### Physical constraints (binding)

| Constraint | Value | Consequence for design |
|---|---|---|
| Panel | 1404 × 1872, **1-bit** e-ink | No color, no greyscale hierarchy, no shadows/blur |
| Full refresh floor | ~250 ms, ghosting allowed | Never depend on a settled frame to convey state |
| Partial refresh | Strip only | Chrome must be a small, fixed, isolatable rect |
| Input | Pen (Wacom EMR) + capacitive touch — **touch unverified from Qt** | Fallback path must exist |
| Touch precision | Finger, no hover, no cursor | ≥120 px targets; no hover state exists |
| Ambient | Reflective display, read in any light | Contrast by shape and fill, never by tint |

### Composition layers (binding)

| Layer id | Role | Fill |
|---|---|---|
| InkSurface | Full-bleed drawing area (existing) | White |
| ToolStrip | The three tools; only persistent chrome | White with 1 px rule separating it from InkSurface |
| SelectionOverlay | Handles + ghost while `selection` is active | Outline only |
| StatusLine | Existing debug/status text | Unchanged |

**Containment:** `ToolStrip` is a fixed edge strip; `InkSurface` shrinks to the remaining area so
a stroke can never begin under the strip. The strip does **not** float over ink.

### Layout regions

| # | Region id | Parent | Contents |
|---|---|---|---|
| 0 | DeviceScreen | panel | Full panel |
| 1 | ToolStrip | DeviceScreen | 3 tool buttons, edge-anchored |
| 2 | InkSurface | DeviceScreen | Drawing region (existing canvas) |
| 3 | SelectionOverlay | InkSurface | Bounds + handles for the selected node |
| 4 | StatusLine | DeviceScreen | Existing status text |

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `tool.selection` | Selection tool | ToolStrip |
| `tool.pen` | Pen tool (**default**) | ToolStrip |
| `tool.ink_box` | Ink-box tool | ToolStrip |
| `ind.tool_active` | Which tool is armed | ToolStrip |
| `ind.tool_unavailable` | Tool cannot act (no session / no pickables) | ToolStrip |
| `ovl.selection_bounds` | Selected node bounds | SelectionOverlay |
| `ovl.resize_handles` | Resize handles on bounds | SelectionOverlay |
| `ovl.drag_ghost` | Advisory position during a drag | SelectionOverlay |

No other controls. No brushes, colors, layers, undo button, or document browser
([epaper Non-Goals](../../prd.md)).

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| `tool.*` | Finger tap | Arm that tool | Active indicator moves within **300 ms** (partial refresh) |
| `ovl.selection_bounds` | Pen press inside | Select + begin move | Bounds outline appears |
| `ovl.resize_handles` | Pen drag on handle | Resize (ghost) | Ghost follows pen; truth on next snapshot |
| InkSurface empty | Pen press in `selection` | Clear selection | Overlay disappears |

### Control states

Note there is **no hover and no focus** on this platform — do not design them.

| Control | default | active (armed) | pressed | unavailable |
|---|---|---|---|---|
| `tool.selection` | outline | filled / inverted | brief invert | hatched + inert when no `pickables` |
| `tool.pen` | outline | filled / inverted | brief invert | never — always available |
| `tool.ink_box` | outline | filled / inverted | brief invert | hatched + inert when session is down |

**Active state must be readable from shape/fill alone**, because a trailing refresh can leave a
ghost of the previous state on screen.

### States matrix

| State id | ToolStrip | InkSurface | SelectionOverlay |
|---|---|---|---|
| `tool.pen` | Pen armed | Ink under pen | hidden |
| `tool.ink_box` | Ink-box armed | Ink under pen (identical) | hidden |
| `tool.selection.idle` | Selection armed | No ink from pen | hidden |
| `tool.selection.selected` | Selection armed | — | bounds + handles |
| `tool.selection.dragging` | Selection armed | — | ghost following pen |
| `tool.selection.empty` | Selection **unavailable** | No ink from pen | hidden; strip states why |
| `session.down` | Ink-box + Selection unavailable | Pen still inks | hidden |
| `touch.unavailable` | Strip inert; pen forced | Pen inks | hidden; status line explains |

### Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2, Qt/QML fullscreen) |
| `data-platform` | `epaper` |
| Input | Pen for content; finger for chrome. No hover, no keyboard, no cursor |
| Color | 1-bit — design in pure black/white with fill and hatch only |
| Motion | **None.** No transitions, no fades — every animation is a refresh cost |

### Anti-patterns

- Any chrome that overlays the ink area or floats above content.
- Conveying active state by tint, greyscale, shadow, or animation.
- A tool that can leave the creator unable to draw (pen must always be reachable).
- Reusing desktop `tokens.css` sizing — desktop's ≥24 px targets are unusable by finger here.
- Refreshing the full panel to show a tool change.

### Out of scope (UI)

- Pan / zoom chrome (PRD Non-Goal), page navigation, document browser.
- Undo affordance on device (undo lives on Infini in the pilot, [SRS-IN-12](../../../infini/features/vector-document/srs-logic.md#srs-in-12-undo-history)).
- `inkScaleMode` toggle on device — desktop-only in the pilot.
