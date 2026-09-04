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

Product regions stay InkSurface / SelectionOverlay / ToolChip. **Implementation** splits
SelectionOverlay by refresh class ([ADR-0019](../../../../adr/ADR-0019-selection-chrome-layers.md) /
[CHL-0017](../../../../../.plan/iter-003/challenges/CHL-0017-selection-chrome-layers.md)):

| Layer id | Product region | Role | Notes |
|---|---|---|---|
| CanvasLayer | InkSurface | Full-bleed document paint | Persistent document raster + live **pen** ink; waveform **Pen**; **not** the live SmartGroup during move/resize ([CHL-0018](../../../../../.plan/iter-003/challenges/CHL-0018-live-node-tool-canvas.md)) |
| ToolCanvasLayer | SelectionOverlay (stroke + live node) | Marquee, lasso, settled dotted AABB; **during move/resize:** the live node (ink + AABB + handles) | Transparent painted sibling; waveform **Mono**; tight dirty rects; origin box punched off CanvasLayer for the gesture |
| ToolLayer (content) | SelectionOverlay (widgets) | Settled handles, Enclose, ink-scale chip, indicators | QML; content-space. Handles ride ToolCanvasLayer **while** a move/resize is in flight |
| ToolLayer (screen) | ToolChip | Tool arming + publish status | QML; **screen-space** — [SRS-EP-05](../tool-modes/srs-ui.md) |

**Option 2 (live node on CanvasLayer)** is not in this campaign. Reopen only for a later rendering
phase if overlay composition regresses Pen ink.

SelectionOverlay widgets and ToolCanvasLayer are **content-space**: they track the box through
pan/zoom (viewport-driven) and through a drag. ToolChip is not pinned to the box.

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

No fifth **exclusive** ToolChip. Enclose stays off-chip. Undo/Redo are history **actions** after a
gap ([ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)). `cta.enclose` is **selection-contextual** on
SelectionOverlay ([ADR-0016](../../../../adr/ADR-0016-selection-create-enclose-cta.md)).
Primary inventory is three exclusive tools + two recognizer toggles + Undo/Redo
([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md); [ADR-0017](../../../../adr/ADR-0017-four-tool-chip.md) superseded).
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
fire* does not. The creator who drew a small rectangle with `Pen` and `recog.ink_box` armed drew a
rectangle; telling them it "failed" would be telling them their ink was a mistake. There is no
exclusive `ink_box` tool.

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| Box bounds | Pen press, no drag | Select SmartGroup | Bounds + handles appear ≤100 ms |
| Box bounds | Pen press + drag | Move | **The ink moves.** Bounds track the ink at ≥5 Hz |
| Canvas (`tool.sel_rect`) | Pen-down + move | Rect marquee | `ovl.marquee` AABB follows tip; pen-up → nodes with ≥80% inside the rect; then `ovl.nodes_bounds` + 6 anchors + `cta.enclose` |
| Canvas (`tool.sel_freeform`) | Pen-down + move | Lasso | `ovl.lasso` polyline follows tip; pen-up closes path, hit-test ≥80% **inside polyline**; chrome → `ovl.nodes_bounds` (tight AABB) + 6 anchors + `cta.enclose` — **not** a dotted polyline |
| `tool.sel_rect` / `tool.sel_freeform` | Finger tap on ToolChip | Arm that selection tool | Exclusive invert on primary bar ([ADR-0021](../../../../adr/ADR-0021-connector-toolchip.md)); recognizer toggles dim |
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
| `sel.none` | Document | overlay hidden; ToolChip shows 3 tools + 2 dimmed recognizer toggles (`sel_rect` or `sel_freeform` armed) | — |
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
- A fifth **exclusive** ToolChip slot (Enclose as a tool). Undo/Redo are actions ([ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md)).

### Open (needs design) {#open-needs-design}

| Question | Why it is open | Owner |
|---|---|---|
| **Handle size and hit tolerance in device units** | **Closed 2026-08-13 (architect).** Visual **28 du**; hit **56 du** (14 du pad beyond visual). 1 du = 1 panel pixel @ 226 dpi. **Not** 8 CSS px. Binding for [STORY-EP-019](../../../../../.plan/iter-003/stories/STORY-EP-019.md). | architect — accepted |
| **LOD cutoff value on device** | **Closed 2026-08-13 (architect).** Manipulation unavailable when the selected box's **smaller on-panel axis < 96 du**. **Not** `TILE_LOD_SCALE = 0.35`. Binding for EP-019. | architect — accepted |
| **Undo affordance** | **Closed** [CHL-0016](../../../../../.plan/iter-003/challenges/CHL-0016-undo-redo-toolbar.md) / [ADR-0018](../../../../adr/ADR-0018-undo-redo-chip-actions.md): gap then Undo \| Redo. | pm — adopted |
| **Selection-create invocation** | **Closed** CHL-0013 / ADR-0016 / CHL-0014 hit-tests / **CHL-0015 + ADR-0017** four-tool chip. Design: [STORY-EP-022](../../../../../.plan/iter-003/stories/STORY-EP-022.md). | architect — accepted; designer |
| **Chrome legibility against dense ink** | 1-bit chrome over handwriting, with no tint or shadow available | designer |

---

## [SRS-EP-22] Hand-touch chrome and hit rules {#srs-ep-22-hand-touch-ui}

<!-- lifecycle: active -->
<!-- needs_design: yes -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Logic:** [SRS-EP-21](./srs-logic.md#srs-ep-21-one-finger), [SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport). **Quality:** [SRS-EP-25](./srs-quality.md#srs-ep-25-one-finger-quality). **Platform:** **epaper-device** (`data-platform: epaper`) — same profile as [SRS-EP-12](#srs-ep-12-selection-chrome). **Do not parent on SRS-EP-12.** Follow toggle is **[SRS-EP-50](../region-sync/srs-ui.md#srs-ep-50-follow-toggle)**, not this package.

### Purpose

One job: make **finger** pick / move / **resize knobs** vs **one-finger empty pan** vs **palm-rest** vs **two-finger** pan/zoom legible and hittable. Knob **visual** stays a small hollow square; knob **hit** meets the finger-eligible floor.

### Hit rules (binding — Designer must not loosen)

| Target | Min hit | Pointer |
|---|---|---|
| SmartGroup bounds (LOD ok) | AABB as painted | Finger **and** pen |
| ToolChip primary tile | **64×64 du** | Finger-eligible |
| Enclose CTA | 64 du | Finger-eligible |
| Resize / 6 square knobs | visual small hollow square; hit **≥ primary tile** | Finger **and** pen |
| Connector end-kind / other &lt;64 du | as specified | **Pen only** |

### Closed control inventory (additive — do not invent)

| id | Kind | Notes |
|---|---|---|
| *(existing SRS-EP-12 overlay)* | — | Reused; knobs are finger-eligible ([CHL-0024](../../../../../.plan/iter-005/challenges/CHL-0024-finger-resize-knobs.md)) |
| `btn.hand_touch` | 64 du 1-bit toggle | Trailing row left of Debug; inverted when on (default on). Kill-switch for canvas fingers — **not** a pan-mode / hand-tool tile |
| `ind.two_finger_pan` | in-progress | Two-finger pan/pinch active — **no** extra chrome required; Designer may use existing region marker **only if** it does not imply Infini always matches ([ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md)) |

<!-- lifecycle: retired -->
<!-- superseded-by: [CHL-0024] -->
`ind.finger_anchor_noop` — **retired 2026-08-20.** Finger on a knob resizes; no no-op hatch.

No pan-mode tool, no “hand tool” tile. Resize knobs are the existing overlay — not a second handle set.

### States matrix (journeys from PRD — do not add)

| State id | When |
|---|---|
| `hand.finger_hit_box` | Finger-down on box; tool → `sel_freeform` |
| `hand.finger_moving` | Finger drag inside selected box (not on a knob) |
| `hand.finger_resizing` | Finger drag on a resize knob |
| `hand.one_finger_empty_palm` | One finger empty canvas; travel ≤ **20 mm** — tap deselects; 0 pan |
| `hand.one_finger_empty_pan` | One finger empty canvas; travel > **20 mm** — local pan |
| `hand.two_finger_pan` | Two-finger pan in progress (local; Infini matches only if Infini following) |
| `hand.pinch` | Pinch in progress |
| `hand.pan_vs_move` | Two-finger ignored because box-move in flight |
| `hand.link_down_local_view` | Two-finger still pans locally |
| `hand.toggle_off` | `btn.hand_touch` off — canvas fingers ignored; chrome taps still work |
| `hand.palm_contacts` | ≥3 capacitive contacts — 0 pan, 0 pinch |

<!-- lifecycle: retired -->
`hand.finger_anchor_noop` and `hand.pen_resize_after_finger_select` — **retired 2026-08-20** ([CHL-0024](../../../../../.plan/iter-005/challenges/CHL-0024-finger-resize-knobs.md)). Do not demonstrate.

### UI-driving fields

`touch.fingerCount`, `hit.kind`, `touch.travelMm`, `toolMode`, `follow.direction`, `handTouch.toggle` — Designer must not invent extra modes or a follow button in this package.

### Anti-patterns

- Sub-floor knob hit (finger would miss or violate the size rule)
- One-finger empty pan **without** the 20 mm threshold (a rest would pan)
- Canvas hand-touch while the pen is near or in contact (palm would pan the page while inking)
- Follow-toggle buttons in this package ([SRS-EP-50](../region-sync/srs-ui.md#srs-ep-50-follow-toggle))
- Hover/focus/cursor
- Phone chrome; this is RM2 1872×1404 landscape preview
- A required scene for pen-after-finger-select (same knob, either pointer)

---

## [SRS-EP-32] Clipboard affordances {#srs-ep-32-clipboard-ui}

<!-- lifecycle: active -->
<!-- needs_design: no -->

**Parent:** [REQ-12](../../prd.md#clipboard). **Product:** [SRS-EP-73](../clipboard/srs-product.md#srs-ep-73-clipboard-product). **Logic:** [SRS-EP-31](../device-document/srs-logic.md#srs-ep-31-clipboard). **Quality:** [SRS-EP-33](../device-document/srs-quality.md#srs-ep-33-clipboard-quality). **Hold routing:** [SRS-EP-11](./srs-logic.md#srs-ep-11-hold-still) tap vs travel (no 500 ms menu). **Platform:** epaper-device. Chrome **frozen** — no design story.

### Purpose

Copy / cut the **current selection** from the Selected strip. Paste from the **same** toolbar when the slot is non-empty **and** a tap location exists. Not OS paste, not ToolChip, not Infini chrome, not long-press.

### Closed control inventory

| id | Kind | Surface | Visible / enabled when |
|---|---|---|---|
| `cta.copy` | action | Selected context toolbar | SelectionMode + `SelectionPhase::Selected` + non-empty ids |
| `cta.cut` | action | Selected context toolbar | same as copy |
| `cta.paste` | action | Same toolbar (or paste-only strip on empty tap) | Slot non-empty **and** a tap location exists |
| `ind.paste_onto_originals` | refuse | Same toolbar | Paste refused because tap/parent is a live source — enclose-style string; slot kept |

Tiles ≥64 du. **No** ToolChip row. **No** hold / long-press strip.

### Two entry points (SelectionMode only: `sel_rect` and `sel_freeform`)

| Entry | When | Actions | Placement |
|---|---|---|---|
| Selected (tap a node) | `SelectionPhase::Selected` **and** tap location | copy, cut, paste (paste if slot non-empty) | Existing selection AABB strip |
| Empty tap | Selection **already** empty **and** tap location **and** slot non-empty | paste only | Toolbar at the tap panel point, then **clamped**. Clamp does not move paste origin. |
| Empty tap while selected | Selection non-empty | — | Deselect only; **do not** record a tap location; **0** chrome |
| Freeform / marquee | Real gesture (not a tap) | copy, cut if selected; **no paste** | Selection AABB; tap location cleared |

### Dismiss / clear tap location

Freeform / marquee gesture · empty tap while selected · pan · mode switch · camera pan/zoom · successful paste. A new tap replaces the location. No extra timeout.

### States

`clip.idle` · `clip.copied` · `clip.cut` · `clip.tap_toolbar` · `clip.pasted` · `clip.empty_tap_noop` · `clip.refuse_live_original` · `clip.undo_after_cut_paste`

### Anti-patterns

- macOS pasteboard affordance
- Multi-slot clipboard
- Long-press / hold-toolbar paste
- ToolChip / barrel paste
- Paste after freeform / marquee (no tap location)
- Moving paste origin when clamping the toolbar
- Inventing +24 world offset

---

## Superseded

New section. Inherits the *intent* of infini
[SRS-IN-14](../../../infini/features/vector-document/srs-ui.md#srs-in-14-ink-box-ui) (deprecated
2026-08-13, CHL-0008) but none of its geometry or interaction model. The
[ink-box-ui](../../../../../.plan/iter-003/design/ink-box-ui/) design package deprecates with
SRS-IN-14; this surface needs a new package
([lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md)).
