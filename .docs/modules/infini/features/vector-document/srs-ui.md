---
feature: vector-document
parent_req: [REQ-02]
version: 0.2.0
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
