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

## [SRS-EP-05] On-device tool chip {#srs-ep-05-tool-chip}

<!-- adopted CHL-0003 2026-08-11: floating 32px orientation-top chip; supersedes full-band strip -->

### Design authority

1. This `srs-ui.md`
2. `epaper` REQ-03 acceptance
3. Physical constraints below (they outrank aesthetics)
4. Design package `[UI-EP-01]` (`.plan/iter-003/design/epaper-tool-strip/`) — scenes + Spec
5. `.docs/DESIGN.md` tokens — **advisory only**; the desktop design system does not transfer to
   a 1-bit panel

### Purpose

**One job:** let the creator see and change what the pen will do, without ever costing ink
latency or reserving a full edge band of drawing area.

### Physical constraints (binding)

| Constraint | Value | Consequence for design |
|---|---|---|
| Panel | 1404 × 1872, **1-bit** e-ink | No color, no greyscale hierarchy, no shadows/blur |
| Full refresh floor | ~250 ms, ghosting allowed | Never depend on a settled frame to convey state |
| Partial refresh | Chip bounds only | Chrome must be a small, isolatable rect (not a full edge band) |
| Input | Pen (Wacom EMR) + capacitive touch — **touch unverified from Qt** | Fallback path must exist (`pen-on-chip`) |
| Chip size | Height **32 px**; tools **32×32** icon tiles; hug width | Compact chip — **relaxes** the prior ≥120 px finger-target rule for this control only (CHL-0003) |
| Ambient | Reflective display, read in any light | Contrast by shape and fill, never by tint |

### Composition layers (binding)

| Layer id | Role | Fill |
|---|---|---|
| InkSurface | **Full-bleed** drawing area (existing) | White |
| ToolChip | Compact three-tool cluster (aka ToolStrip) | White; 1 px outline; squared (`border-radius: 0`) |
| SelectionOverlay | Handles + ghost while `selection` is active | Outline only |
| StatusLine | Existing debug/status text | Unchanged |

**Containment:** `ToolChip` is a **floating** compact control cluster (height **32 px**, width hug
content, icon-only) anchored to the **top edge of the current gut orientation** (moves with
device orientation; when gut is on top, oriented “top” places the chip near the opposite short
edge). `InkSurface` remains **full-bleed** — chrome does **not** reserve a full band. Pen/touch
hits on the chip are excluded from ink via hit-test ([SRS-EP-04](./srs-logic.md)); exclusion rect
= **chip bounds**, not a full edge strip.

### Layout regions

| # | Region id | Parent | Contents |
|---|---|---|---|
| 0 | DeviceScreen | panel | Full panel |
| 1 | ToolChip | DeviceScreen | 3 tool buttons, floating orientation-top chip |
| 2 | InkSurface | DeviceScreen | Full-bleed drawing region |
| 3 | SelectionOverlay | InkSurface | Bounds + handles for the selected node |
| 4 | StatusLine | DeviceScreen | Existing status text |

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `tool.selection` | Selection tool | ToolChip |
| `tool.pen` | Pen tool (**default**) | ToolChip |
| `tool.ink_box` | Ink-box tool | ToolChip |
| `ind.tool_active` | Which tool is armed | ToolChip |
| `ind.tool_unavailable` | Tool cannot act (no session / no pickables) | ToolChip |
| `ovl.selection_bounds` | Selected node bounds | SelectionOverlay |
| `ovl.resize_handles` | Resize handles on bounds | SelectionOverlay |
| `ovl.drag_ghost` | Advisory position during a drag | SelectionOverlay |

No other controls. No brushes, colors, layers, undo button, or document browser
([epaper Non-Goals](../../prd.md)).

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| `tool.*` | Finger tap (or pen-on-chip fallback) | Arm that tool | Active indicator moves within **300 ms** (partial refresh of chip) |
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

| State id | ToolChip | InkSurface | SelectionOverlay |
|---|---|---|---|
| `tool.pen` | Pen armed | Ink under pen | hidden |
| `tool.ink_box` | Ink-box armed | Ink under pen (identical) | hidden |
| `tool.selection.idle` | Selection armed | No ink from pen | hidden |
| `tool.selection.selected` | Selection armed | — | bounds + handles |
| `tool.selection.dragging` | Selection armed | — | ghost following pen |
| `tool.selection.empty` | Selection **unavailable** | No ink from pen | hidden; chip states why |
| `session.down` | Ink-box + Selection unavailable | Pen still inks | hidden |
| `touch.unavailable` | Chip inert for finger; pen-on-chip or pen forced | Pen inks | hidden; status line explains |
| `orient.gutOnTop` | Chip on oriented top (near opposite short edge) | Full-bleed | unchanged |

### Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2, Qt/QML fullscreen) |
| `data-platform` | `epaper` |
| Preview | Landscape tablet frame **1872×1404** (native panel rotated) — not phone chrome |
| Input | Pen for content; finger for chrome (pen-on-chip fallback). No hover, no keyboard, no cursor |
| Color | 1-bit — design in pure black/white with fill and hatch only |
| Motion | **None.** No transitions, no fades — every animation is a refresh cost |

### Anti-patterns

- A **full-band** edge strip that shrinks `InkSurface` (retired by CHL-0003).
- Chrome that is taller than **32 px** for this pilot chip, or that uses rounded “pill” chrome.
- Conveying active state by tint, greyscale, shadow, or animation.
- A tool that can leave the creator unable to draw (pen must always be reachable).
- Reusing desktop `tokens.css` sizing as if the chip were a desktop toolbar.
- Refreshing the full panel to show a tool change.
- Phone-sized mobile chrome in design previews for this surface.

### Out of scope (UI)

- Pan / zoom chrome (PRD Non-Goal), page navigation, document browser.
- Undo affordance on device (undo lives on Infini in the pilot, [SRS-IN-12](../../../infini/features/vector-document/srs-logic.md#srs-in-12-undo-history)).
- `inkScaleMode` toggle on device — desktop-only in the pilot.
