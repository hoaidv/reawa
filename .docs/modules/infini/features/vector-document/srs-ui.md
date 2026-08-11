---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.3.0
lifecycle: active
needs_design: true
---

# SRS — Vector document (UI)

Durable UI contract for open/save chrome and minimal tree affordances on the infinity
canvas. Companion surface: [infinity-canvas SRS-IN-02](../infinity-canvas/srs-ui.md).

## [SRS-IN-05] Document chrome (open / save / tree hints)

**Logic:** [SRS-IN-04](./srs-logic.md). **Quality:** [SRS-IN-06](./srs-quality.md).
**Experience:** [srs-experience](./srs-experience.md). **Product:** [srs-product](./srs-product.md).

---

### Design authority

1. This `srs-ui.md`
2. Infinity-canvas `srs-ui` (CanvasStage / WorldLayer ownership)
3. Package tokens / `.docs/DESIGN.md` when present
4. Human brief (tree-of-vectors) after PM adopt
5. Reference images — atmosphere only

---

### Purpose

**One job:** let the user know which document is loaded, create/open/save it, see dirty/error,
and lightly perceive that content is a **tree** (not a full layer studio). Editing tools for
group/frame/connector may be minimal v0 chrome or deferred to implement stories — but states
and regions must not assume a flat stroke bag.

---

### Composition layers (binding)

| Layer id | Scroll? | Role | Fill |
|---|---|---|---|
| WindowFrame | Fixed | App chrome + canvas host | Surface |
| DocChrome | Fixed | Title / path / dirty / open-save actions | Elevated / blur OK |
| CanvasStage | Fixed | Gesture + world (from infinity-canvas) | Transparent host |
| WorldLayer | Fixed (paints) | Tree projection (ink, text, primitives, connectors; group/frame bounds optional) | Canvas |
| DocError | Fixed | Inline error when `doc.error` | Non-modal strip/banner |

**Scroll policy:** No document chrome scroll. World pan/zoom is canvas transform, not CSS scroll.

---

### Layout regions (containment tree)

| # | Region id | Parent | Contents | Component |
|---|---|---|---|---|
| 0 | WindowFrame | screen | Full window | — |
| 1 | DocChrome | WindowFrame | Title/path, dirty mark, New/Open/Save | DocChromeBar |
| 2 | CanvasStage | WindowFrame | Full-bleed canvas (existing) | CanvasStage |
| 3 | WorldLayer | CanvasStage | Drawable tree projection | WorldLayer |
| 4 | StatusZoom | CanvasStage or WindowFrame | Zoom % (existing) | ZoomReadout |
| 5 | DocError | WindowFrame | Open/parse error copy + dismiss | DocErrorInline |

**Containment:** `DocChrome` must **not** cover the entire CanvasStage (thin top or corner).
`DocError` overlays chrome or sits under DocChrome — never a blocking modal that steals
gestures unless user confirms abandon-dirty (open option).

---

### Seam / overlap rules

- DocChrome sits above CanvasStage in z-order; pointer events only on chrome controls.
- WorldLayer paints under chrome; grid/ink must remain visible in the remaining viewport.
- No card/dashboard chrome invent for tree (no Figma-full properties panel in v0).

---

### Chrome relationships

| Pair | Relationship |
|---|---|
| DocChrome ↔ CanvasStage | Seamless sibling under WindowFrame |
| DocChrome ↔ StatusZoom | Same family; do not collide (chrome leading, zoom trailing) |
| DocError ↔ DocChrome | Contrastive when error |

---

### Open options (locked)

| Topic | Decision |
|---|---|
| Dirty close | **A** — Electron `beforeunload` / confirm dialog when dirty (v0) |
| Tree outliner | **B** — **Deferred** full outliner; v0 may show selection-only hint in DocChrome |
| Frame/group create UI | **C** — Minimal commands OK in later implement; design package must reserve no huge panel |

---

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `cta.doc_new` | New document | DocChrome |
| `cta.doc_open` | Open… | DocChrome |
| `cta.doc_save` | Save | DocChrome |
| `ind.doc_dirty` | Unsaved indicator | DocChrome |
| `ind.doc_title` | Title or path label | DocChrome |
| `btn.doc_error_dismiss` | Dismiss error | DocError |

---

### Copy table (en)

| id | Copy |
|---|---|
| `cta.doc_new` | New |
| `cta.doc_open` | Open… |
| `cta.doc_save` | Save |
| `ind.doc_dirty` | Unsaved |
| `doc.none` hint | Open or create a document to keep your sketches |
| `doc.error` | Couldn’t open that file. Your canvas was left unchanged. |

---

### Interaction map

| Control | Action | Destination | Chrome side-effect | Feedback |
|---|---|---|---|---|
| `cta.doc_new` | Create empty tree | `doc.open` clean | Title → “Untitled” | Dirty off; canvas empty world |
| `cta.doc_open` | Native file picker | `doc.open` or `doc.error` | Title → path | Success: tree paints; fail: DocError |
| `cta.doc_save` | Write persistence | stay `doc.open` | Dirty clears | Brief enabled→saved; disabled if !dirty optional |
| `btn.doc_error_dismiss` | Clear error | prior doc.* | Hide DocError | Focus returns to canvas |

---

### Control states

| Control | hover | focus | active | disabled | loading | error |
|---|---|---|---|---|---|---|
| `cta.doc_save` | yes | yes | yes | when !dirty (optional) | while writing | save fail → DocError |
| `cta.doc_open` | yes | yes | yes | while loading | while parsing | open fail |
| `cta.doc_new` | yes | yes | yes | while loading | — | — |

---

### Transitions / motion intent

- Dirty indicator: instant or ≤150 ms opacity.
- DocError: appear without covering canvas center; no full-screen dim in v0.

---

### States matrix

| State id | DocChrome | WorldLayer | DocError | Notes |
|---|---|---|---|---|
| `doc.none` | Open/New enabled; title empty/none | Empty canvas OK | hidden | Affordance visible |
| `doc.open` | Title/path; Save per dirty | Tree projection | hidden | |
| `doc.dirty` | Dirty on; Save enabled | Tree | hidden | Substate of open |
| `doc.error` | Prior title if any | Unchanged | visible | |

---

### Visual hierarchy / density

1. Canvas / world content  
2. DocChrome title + dirty  
3. Open/Save actions  
4. Zoom readout (existing)  
5. Error strip when needed  

Desktop targets ≥24 px for chrome buttons.

---

### Platform profile

| Field | Value |
|---|---|
| Profile | **desktop** (Electron; macOS first) |
| `data-platform` | `desktop` |
| File UX | Native open/save dialogs |
| Input | pointer + keyboard shortcuts TBD (⌘O / ⌘S later OK) |

---

### Anti-patterns

- Modal sheet that blocks pan/zoom for routine save.
- Dashboard cards for “document stats”.
- Inventing a full layers/outliner without product BR.

### Out of scope (UI)

- Brush / pen toolbars; pressure UI; cloud share; multiplayer cursors.
- OCR convert-to-text UI for Smart Group (ink stays ink).
- Full layers outliner (deferred); Smart Group may show selection handles only in pilot.

---

## [SRS-IN-14] Ink-box tools and selection overlay (Infini) {#srs-in-14-ink-box-ui}

**Parent:** [REQ-04](../../prd.md#smart-group). **Logic:** [SRS-IN-11](./srs-logic.md#srs-in-11-selection-manipulation).
**Decision:** [ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md).
Device sibling (deliberately **not** the same design): [epaper SRS-EP-05](../../../epaper/features/tool-modes/srs-ui.md).

### Purpose

**One job:** show what a pointer press will do right now, and make a Smart Group's extent,
selection, and scale mode manipulable — without growing a properties panel.

### Composition layers (binding, extends SRS-IN-05)

| Layer id | Role | Notes |
|---|---|---|
| ToolStrip | `Selection` · `Ink-box` arming | Small, docked; must not collide with DocChrome or StatusZoom |
| SelectionOverlay | Bounds outline, resize handles, `inkScaleMode` toggle | Painted in canvas space, above WorldLayer |
| BoundsHint | Optional chrome for handles / hit-test only | Not a substitute for missing boundary ink |

**Containment:** ToolStrip lives beside existing chrome (DocChrome leading, StatusZoom trailing —
do not collide). SelectionOverlay is canvas-space and pans/zooms with content; it is **not** DOM
chrome pinned to the window.

### Closed control inventory

| id | Control | Region |
|---|---|---|
| `tool.selection` | Arm Selection (default) | ToolStrip |
| `tool.ink_box` | Arm Ink-box | ToolStrip |
| `ind.manipulation_unavailable` | Below-LOD notice | ToolStrip or StatusZoom |
| `ovl.selection_bounds` | Selected Smart Group bounds | SelectionOverlay |
| `ovl.resize_handles` | Resize handles (**no rotation handle**) | SelectionOverlay |
| `tgl.ink_scale_mode` | `withBounds` ↔ `fixedInk` | SelectionOverlay |
| `hint.smart_group_bounds` | Optional chrome extent (handles / hit-test) | BoundsHint |
| `cta.create_smart_group` | Promote selection → Smart Group | SelectionOverlay / context |
| `ind.create_refused_no_surround` | Why create failed (no surround stroke) | Inline near CTA |

### Box appearance (binding — BR-09d)

| Origin | Unselected appearance | Rule |
|---|---|---|
| Enclosure or selection-with-surround | The creator's own **boundary** ink | Add **no** synthetic ink rectangle; the drawn surround is the box |
| Create refused (no surround) | N/A — no Smart Group | `cta.create_smart_group` disabled **or** shows `ind.create_refused_no_surround`; selection unchanged |

### Interaction map

| Control | Action | Result | Feedback |
|---|---|---|---|
| `tool.selection` | Click | Arm Selection | Active state; cursor affordance changes |
| `tool.ink_box` | Click | Arm Ink-box | Active state |
| `cta.create_smart_group` | Click (≥2 inks selected, surround qualifies) | `create_smart_group` (SRS-IN-16) | Overlay → Smart Group selected |
| `cta.create_smart_group` | Click (no surround) | Refuse | `ind.create_refused_no_surround`; selection unchanged |
| Canvas press inside bounds | Press + drag | Move node | Node follows pointer; canvas does not pan |
| Canvas press inside bounds | Press, no drag | Select | Bounds + handles appear |
| `ovl.resize_handles` | Drag | Resize bounds | Live bounds; content ink per mode |
| `tgl.ink_scale_mode` | Click | Swap mode | Immediately visible on next resize |
| Canvas press on empty | Press | Deselect | Overlay hides; press continues as pan |

### Control states

| Control | hover | focus | active | disabled |
|---|---|---|---|---|
| `tool.selection` | yes | yes | when armed | never |
| `tool.ink_box` | yes | yes | when armed | never |
| `ovl.resize_handles` | yes | yes | while dragging | **below 0.35 scale** |
| `tgl.ink_scale_mode` | yes | yes | reflects current mode | when nothing selected |

### States matrix

| State id | ToolStrip | WorldLayer | SelectionOverlay |
|---|---|---|---|
| `tool.selection.idle` | Selection armed | Tree | hidden |
| `tool.selection.selected` | Selection armed | Tree | bounds + handles + mode toggle |
| `tool.selection.dragging` | Selection armed | Node follows pointer | bounds only |
| `tool.ink_box.armed` | Ink-box armed | Tree | hidden |
| `manipulation.unavailable` | `ind.manipulation_unavailable` visible | Tile-LOD paint | hidden |

### Anti-patterns

- A properties panel — the pilot has exactly one toggle, and it belongs on the overlay.
- A rotation handle (the geometry does not support it — SRS-IN-11).
- Drawing the bounds hint so it looks like ink; the creator must never mistake chrome for content.
- Creating an AABB-only Smart Group from a selection with no surround stroke.
- Chrome that swallows canvas gestures outside its own controls.

