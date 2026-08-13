---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.1.0
lifecycle: active
needs_design: true
---

# SRS — Ink-box on the device (UI)

Selection and manipulation chrome on the panel, for
[REQ-05](../../prd.md#device-ink-box) and [REQ-06](../../prd.md#device-manipulation).
Logic: [SRS-EP-10 / SRS-EP-11](./srs-logic.md). Tool chip:
[SRS-EP-05](../tool-modes/srs-ui.md). Journeys: [srs-experience](./srs-experience.md).

**Not a port.** [SRS-IN-14] (deprecated) solved this for a mouse, a hover state, and a 60 Hz canvas.
This surface has a pen, no hover, 1-bit ink, and a ~250 ms refresh floor. What transfers is the
*intent*: show what a press will do, and make extent and scale mode manipulable without a properties
panel. Everything about how that is drawn is re-decided here.

---

## [SRS-EP-12] Selection overlay and manipulation chrome {#srs-ep-12-selection-chrome}

**Parent:** [REQ-06](../../prd.md#device-manipulation).
**Decision:** [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2 —
what is on the panel is the document.

### Purpose

**One job:** make a box's extent, selection state, and scale mode manipulable with a pen, on a
surface where chrome costs refresh and everything is black or white.

The load-bearing constraint is that **nothing here is a preview**. The overlay annotates ink that is
already moving; it never stands in for ink that will move later
([SRS-EP-11](./srs-logic.md#srs-ep-11-device-manipulation)).

### Composition layers (extends [SRS-EP-05](../tool-modes/srs-ui.md))

| Layer id | Role | Notes |
|---|---|---|
| InkSurface | Full-bleed document paint | Unchanged; the ink itself is the feedback |
| SelectionOverlay | Bounds outline, handles, mode toggle | Panel-space, above InkSurface, below ToolChip |
| ToolChip | Tool arming + publish status | [SRS-EP-05](../tool-modes/srs-ui.md) — do not duplicate here |

SelectionOverlay is **content-space**: it tracks the box through pan/zoom (viewport-driven) and
through a drag. It is not pinned chrome.

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `ovl.selection_bounds` | Selected box bounds outline | SelectionOverlay |
| `ovl.resize_handles` | Resize handles — **no rotation handle** | SelectionOverlay |
| `tgl.ink_scale_mode` | `withBounds` ↔ `fixedInk` | SelectionOverlay |
| `ind.mode_current` | Which mode is active right now | SelectionOverlay |
| `ind.manipulation_unavailable` | Below the LOD cutoff | SelectionOverlay or ToolChip |
| `ind.create_refused_no_surround` | Why a selection-create was refused | Transient, near the selection |

No properties panel, no context menu, no handle labels, no z-order controls, no alignment guides.
`cta.create_smart_group` is **out of scope for v1 chrome** — selection-create exists in logic
([SRS-EP-10](./srs-logic.md#srs-ep-10-device-recognition)) but has no on-panel invocation until the
undo/command affordance question is answered ([Open](#open-needs-design)).

### Box appearance (binding)

| Origin | Unselected appearance | Rule |
|---|---|---|
| Enclosure, or selection-with-surround | The creator's own **boundary** ink | Draw **no** synthetic rectangle — the drawn surround *is* the box |
| Selected | Boundary ink + `ovl.selection_bounds` | Chrome must be visually distinguishable from ink at a glance (see anti-patterns) |
| Create refused | Nothing is created | `ind.create_refused_no_surround`; selection unchanged |
| Enclose guard failed | Ordinary ink, unchanged | **No** banner, no error state, no chrome at all |

The asymmetry is intentional: a *refused explicit command* deserves a reason, a *guard that did not
fire* does not. The creator who drew a small rectangle in `Ink-box` mode drew a rectangle; telling
them it "failed" would be telling them their ink was a mistake.

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| Box bounds | Pen press, no drag | Select | Bounds + handles appear ≤100 ms |
| Box bounds | Pen press + drag | Move | **The ink moves.** Bounds track the ink at ≥5 Hz |
| `ovl.resize_handles` | Pen drag on a handle | Resize | Real ink resizes per mode; bounds follow the handle |
| `tgl.ink_scale_mode` | Pen tap | Swap mode | `ind.mode_current` updates; effect visible on the next resize |
| Empty canvas | Pen press in `selection` | Deselect | Overlay gone; **0** residual pixels on the next settled frame |
| Another box | Pen press | Move selection | Previous overlay fully cleared before the new one draws |
| Any | Pen press below the LOD cutoff | Nothing | `ind.manipulation_unavailable` states why |

Release is the commit. There is no confirm step, no accept affordance, and no state in which the
picture is "pending" — [SRS-EP-11](./srs-logic.md#srs-ep-11-device-manipulation) guarantees the
committed geometry equals the released geometry.

### Control states

No hover, no focus, no cursor on this platform — do not design them.

| Control | default | active | pressed | unavailable |
|---|---|---|---|---|
| `ovl.selection_bounds` | hidden | drawn on the selected box | — | hidden below the LOD cutoff |
| `ovl.resize_handles` | hidden | drawn when selected | the dragged handle is distinct | hidden below the LOD cutoff |
| `tgl.ink_scale_mode` | hidden | drawn when selected; reflects current mode | brief invert | hidden when nothing is selected |
| `ind.manipulation_unavailable` | hidden | visible below the cutoff | — | — |

### States matrix

| State id | InkSurface | SelectionOverlay | Refresh |
|---|---|---|---|
| `sel.none` | Document | hidden | — |
| `sel.selected` | Document | bounds + handles + mode toggle | Partial |
| `sel.moving` | **Ink following the pen** | bounds tracking the ink | Partial only, ≥5 Hz |
| `sel.resizing.with_bounds` | **Content scaling with the box** | bounds + active handle | Partial only, ≥5 Hz |
| `sel.resizing.fixed_ink` | **Content keeping its size, tracking its UV** | bounds + active handle | Partial only, ≥5 Hz |
| `sel.deselected` | Document | hidden | Settled frame shows **0** residual chrome |
| `sel.create_refused` | Selection unchanged | `ind.create_refused_no_surround` | Partial |
| `sel.unavailable` | Document | `ind.manipulation_unavailable` | Partial |
| `sel.reloaded` | Replaced document | hidden — selection cleared | Sharp |

`sel.resizing.with_bounds` and `sel.resizing.fixed_ink` are separate states on purpose: CHL-0005 was
a creator unable to tell which mode they were resizing in until after they released.

### Platform profile

| Field | Value |
|---|---|
| Profile | **epaper-device** (reMarkable 2, Qt/QML fullscreen) |
| `data-platform` | `epaper` |
| Preview | Landscape tablet frame **1872×1404** — not phone chrome |
| Input | Pen for content and for handles; finger for the chip only |
| Color | 1-bit — black/white, fill and hatch only |
| Motion | **None.** Feedback is redraw, not animation |

### Anti-patterns

- Drawing chrome so it reads as ink — the creator must never mistake a handle for a stroke, and
  1-bit gives no tint to lean on.
- A ghost, marquee outline, or any stand-in that moves while the ink stays put. This is the defect
  the whole rework removes ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md)).
- A rotation handle — the geometry does not support it yet
  ([SRS-EP-11](./srs-logic.md#srs-ep-11-device-manipulation)).
- A properties panel, or a mode toggle parked on the ToolChip instead of on the selection.
- Full-panel refresh to show selection, deselection, or drag feedback.
- Handles sized for a mouse. The pen's pointing error, not a desktop's 8 px, sets the tolerance.
- An error banner when an enclose guard declines.
- Chrome that swallows pen input outside its own controls.

### Out of scope (UI)

- Multi-select, marquee, enter/exit group, align/distribute, z-order — [REQ-08](../../prd.md#node-manipulation).
- Rotation affordances of any kind.
- Connector attachment chrome on a box.
- Any confirm/accept step for a completed gesture.

### Open (needs design) {#open-needs-design}

| Question | Why it is open | Owner |
|---|---|---|
| **Handle size and hit tolerance in device units** | [SRS-IN-11]'s 8 CSS px is a mouse constant; the pen and panel DPI give a different answer. Needs a spike on hardware | architect + designer |
| **LOD cutoff value on device** | `TILE_LOD_SCALE = 0.35` is a desktop tile constant; the device has a fixed panel and no tiles | architect |
| **Undo affordance** | Undo is now device-side ([SRS-EP-07](../device-document/srs-logic.md)) with no button and no keyboard | pm + designer |
| **Selection-create invocation** | The logic exists; the gesture or control that triggers it does not, and it competes with undo for the same scarce chrome | pm + designer |
| **Chrome legibility against dense ink** | 1-bit chrome over handwriting, with no tint or shadow available | designer |

---

## Superseded

New section. Inherits the *intent* of infini
[SRS-IN-14](../../../infini/features/vector-document/srs-ui.md#srs-in-14-ink-box-ui) (deprecated
2026-08-13, CHL-0008) but none of its geometry or interaction model. The
[ink-box-ui](../../../../../.plan/iter-003/design/ink-box-ui/) design package deprecates with
SRS-IN-14; this surface needs a new package
([lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md)).
