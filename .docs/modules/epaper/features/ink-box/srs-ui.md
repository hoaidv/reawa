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
| `ovl.selection_bounds` | Selected box bounds outline (SmartGroup) | SelectionOverlay |
| `ovl.resize_handles` | Resize handles — **no rotation handle** (SmartGroup) | SelectionOverlay |
| `ovl.marquee` | Thin dotted **rectangle** while dragging with `tool.sel_rect` | SelectionOverlay |
| `ovl.lasso` | Thin dotted **polyline** while drawing with `tool.sel_freeform`; gone after pen-up | SelectionOverlay |
| `ovl.nodes_bounds` | Thin dotted selection rect = **tight** union AABB of selected document nodes (**0** extra padding) | SelectionOverlay |
| `ovl.select_anchors` | **6** square anchors on `ovl.nodes_bounds` (visual only; events later) | SelectionOverlay |
| `cta.enclose` | Create Smart Group from selection (Creation B) — **icon only**, primary-button size | SelectionOverlay |
| `tgl.ink_scale_mode` | `withBounds` ↔ `fixedInk` | SelectionOverlay |
| `ind.mode_current` | Which mode is active right now | SelectionOverlay |
| `ind.manipulation_unavailable` | Below the LOD cutoff | SelectionOverlay or ToolChip |
| `ind.create_refused_no_surround` | Why a selection-create was refused | Transient, near the selection |

No fifth ToolChip (Enclose / undo stay off-chip). `cta.enclose` is **selection-contextual** on
SelectionOverlay ([ADR-0016](../../../../adr/ADR-0016-selection-create-enclose-cta.md)).
Primary inventory is four tools ([ADR-0017](../../../../adr/ADR-0017-four-tool-chip.md)).
`cta.create_smart_group` is the logic alias of `cta.enclose`. Enclose-with-Ink-box (Creation A)
remains a separate path.

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
| Box bounds | Pen press, no drag | Select SmartGroup | Bounds + handles appear ≤100 ms |
| Box bounds | Pen press + drag | Move | **The ink moves.** Bounds track the ink at ≥5 Hz |
| Canvas (`tool.sel_rect`) | Pen-down + move | Rect marquee | `ovl.marquee` AABB follows tip; pen-up → nodes with ≥80% inside the rect; then `ovl.nodes_bounds` + 6 anchors + `cta.enclose` |
| Canvas (`tool.sel_freeform`) | Pen-down + move | Lasso | `ovl.lasso` polyline follows tip; pen-up closes path, hit-test ≥80% **inside polyline**; chrome → `ovl.nodes_bounds` (tight AABB) + 6 anchors + `cta.enclose` — **not** a dotted polyline |
| `tool.sel_rect` / `tool.sel_freeform` | Finger tap on ToolChip | Arm that selection tool | Exclusive invert on primary bar ([ADR-0017](../../../../adr/ADR-0017-four-tool-chip.md)) |
| `ovl.resize_handles` | Pen drag on a handle | Resize | Real ink resizes per mode; bounds follow the handle |
| `tgl.ink_scale_mode` | Pen tap | Swap mode | `ind.mode_current` updates; effect visible on the next resize |
| `cta.enclose` | Pen or finger tap | Selection-create | Box created, or `sel.create_refused` |
| Empty canvas | Pen press in either selection tool (no drag) | Deselect | Overlay gone; **0** residual pixels on the next settled frame |
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
| `ovl.marquee` | hidden | drawn during `sel_rect` drag | — | — |
| `ovl.lasso` | hidden | drawn during `sel_freeform` draw; **hidden** after pen-up | — | — |
| `ovl.nodes_bounds` | hidden | drawn when ≥1 document node selected | — | — |
| `ovl.select_anchors` | hidden | 6 squares on `ovl.nodes_bounds` | — | events deferred — not pressable this campaign |
| `cta.enclose` | hidden | visible when selection non-empty under `sel_rect` or `sel_freeform`; **icon-only**, size = ToolChip primary (64 du); **no** context-toolbar chrome | brief invert | hidden when selection empty |
| `tgl.ink_scale_mode` | hidden | drawn when SmartGroup selected; reflects current mode | brief invert | hidden when nothing is selected |
| `ind.manipulation_unavailable` | hidden | visible below the cutoff | — | — |

### States matrix

| State id | InkSurface | SelectionOverlay | Refresh |
|---|---|---|---|
| `sel.none` | Document | overlay hidden; ToolChip shows four tools (`sel_rect` or `sel_freeform` armed) | — |
| `sel.marquee` | Document | `ovl.marquee` AABB follows tip; `tool.sel_rect` armed | Partial |
| `sel.lasso` | Document | `ovl.lasso` polyline follows tip; `tool.sel_freeform` armed | Partial |
| `sel.nodes_selected` | Document | `ovl.nodes_bounds` + 6 anchors + `cta.enclose` (polyline gone) | Partial |
| `sel.selected` | Document | bounds + handles + mode toggle (SmartGroup) | Partial |
| `sel.moving` | **Ink following the pen** | bounds tracking the ink | Partial only, ≥5 Hz |
| `sel.resizing.with_bounds` | **Content scaling with the box** | bounds + active handle | Partial only, ≥5 Hz |
| `sel.resizing.fixed_ink` | **Content keeping its size, tracking its UV** | bounds + active handle | Partial only, ≥5 Hz |
| `sel.deselected` | Document | hidden | Settled frame shows **0** residual chrome |
| `sel.create_refused` | Selection unchanged | `ind.create_refused_no_surround` (+ selection chrome kept) | Partial |
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
- A ghost outline that moves while the ink stays put during **move/resize**. This is the defect
  the whole rework removes ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md)).
  **Exception:** `ovl.marquee` / `ovl.nodes_bounds` are selection chrome, not a stand-in for ink
  motion ([CHL-0013](../../../../../.plan/iter-003/challenges/CHL-0013-selection-create-feedback-enclose-cta.md)).
- A rotation handle — the geometry does not support it yet
  ([SRS-EP-11](./srs-logic.md#srs-ep-11-device-manipulation)).
- A properties panel, or a mode toggle parked on the ToolChip instead of on the selection.
- Full-panel refresh to show selection, deselection, or drag feedback.
- Handles sized for a mouse. The pen's pointing error, not a desktop's 8 px, sets the tolerance.
- An error banner when an enclose guard declines.
- A selection rect with empty padding around the selected nodes — `ovl.nodes_bounds` must be the
  tight union AABB (0 extra padding).

### Out of scope (UI)

- Align/distribute, enter/exit group, z-order chrome — [REQ-08](../../prd.md#node-manipulation).
  **Marquee multi-select for Creation B is in scope** ([CHL-0013](../../../../../.plan/iter-003/challenges/CHL-0013-selection-create-feedback-enclose-cta.md)).
- Rotation affordances of any kind.
- Connector attachment chrome on a box.
- Any confirm/accept step beyond tapping `cta.enclose`.
- Drag events on the 6 selection anchors (visual only this campaign).
- A fifth ToolChip slot (Enclose or undo).

### Open (needs design) {#open-needs-design}

| Question | Why it is open | Owner |
|---|---|---|
| **Handle size and hit tolerance in device units** | **Closed 2026-08-13 (architect).** Visual **28 du**; hit **56 du** (14 du pad beyond visual). 1 du = 1 panel pixel @ 226 dpi. **Not** 8 CSS px. Binding for [STORY-EP-019](../../../../../.plan/iter-003/stories/STORY-EP-019.md). | architect — accepted |
| **LOD cutoff value on device** | **Closed 2026-08-13 (architect).** Manipulation unavailable when the selected box's **smaller on-panel axis < 96 du**. **Not** `TILE_LOD_SCALE = 0.35`. Binding for EP-019. | architect — accepted |
| **Undo affordance** | **Deferred this campaign ([CHL-0010](../../../../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md)).** No on-panel control; no fifth ToolChip; no properties panel. | pm + designer — deferred |
| **Selection-create invocation** | **Closed** CHL-0013 / ADR-0016 / CHL-0014 hit-tests / **CHL-0015 + ADR-0017** four-tool chip. Design: [STORY-EP-022](../../../../../.plan/iter-003/stories/STORY-EP-022.md). | architect — accepted; designer |
| **Chrome legibility against dense ink** | 1-bit chrome over handwriting, with no tint or shadow available | designer |

---

## Superseded

New section. Inherits the *intent* of infini
[SRS-IN-14](../../../infini/features/vector-document/srs-ui.md#srs-in-14-ink-box-ui) (deprecated
2026-08-13, CHL-0008) but none of its geometry or interaction model. The
[ink-box-ui](../../../../../.plan/iter-003/design/ink-box-ui/) design package deprecates with
SRS-IN-14; this surface needs a new package
([lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md)).
