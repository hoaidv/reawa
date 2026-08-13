---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.3.0
lifecycle: active
---

# SRS — Vector document (Logic)

Architect-owned tree rules for [REQ-02](../../prd.md#vector-document).
Structural decision: [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md).
Ownership: [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md).
Session sync: [ADR-0015](../../../../adr/ADR-0015-one-way-sync-contract.md).
**Shared semantics: [domain/vector-document](../../../../domain/vector-document.md)** — node kinds,
roles, transforms, ops, and invariants are defined there for both peers; this file binds Infini's
implementation to them.

**Ownership (2026-08-13, CHL-0008).** Infini holds a **mirror**, not the working document. Its job
is: apply published device changes idempotently, paint them, serialize them. It authors no document
changes. The sections below that describe *authoring* are deprecated per element — see each banner.

**Implementation status (code SoT, 2026-08-11)**

| Layer | Status |
|---|---|
| `infini/src/document/VectorDocument.ts` | Tree + `applyOp` + SVG serialize/parse — **unit-tested** |
| Live canvas paint | `InfiniDocument` WorldLayer primitives — **not** tree-driven yet |
| Live RM ink | Appended as WorldLayer `path`s — does **not** `append_ink` into the tree |
| Doc open/save chrome | Not shipped |
| Inbound `doc_change` applier | **Not shipped** — the main new desktop work ([SRS-IN-07](../tablet-sync/srs-logic.md)) |
| Smart Group authoring UI | **Deprecated** — [SRS-IN-14](./srs-ui.md#srs-in-14-ink-box-ui) |
| Live `doc_op` wire | Superseded by `doc_change` (ADR-0015 §2) |

**Critical prerequisite (revised).** `CanvasStage.rebuildWithRmInk` turns `stroke_*` into flat
WorldLayer `path` primitives, and nothing enters `VectorDocument`. Under the rework the desktop's
paint source must become the **mirror tree** fed by `doc_change`, with `stroke_*` demoted to a
transient preview layer. That inversion — not enclose recognition — is now the first desktop slice.

## [SRS-IN-04] Tree model and three representations

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Semantics lifted to the shared domain doc; this
     section now binds Infini's implementation to it. Same id, content revised. -->

> **Revised 2026-08-13.** Node kinds, roles, invariants, and op meanings now live in
> [domain/vector-document](../../../../domain/vector-document.md) because **both peers implement
> them**. This section keeps Infini's representations (in-memory, SVG profile, transmit) and must
> not restate or drift from the domain doc. Idempotent-apply-by-`opId` is load-bearing for the
> mirror ([SRS-IN-07](../tablet-sync/srs-logic.md)) and stays here.

### Endpoint(s)

N/A — local document + session transport (not HTTP). File open/save via Electron dialogs.
Session ops over the document channel ([ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)).

### In-memory tree (source of truth in session)

Materialised structure (see also [srs-data](./srs-data.md)):

```text
Document
 └─ rootChildren[]: Node   # Frames only legal here among container kinds that are Frame
Node =
  | Ink { id, samples[], style, parent? }
  | Text { id, box, runs[], style }
  | Primitive { id, kind: line|rect|ellipse, geom, style }
  | Group { id, children: Node[] }          # no Frame children
  | SmartGroup {                            # ADR-0011 pilot
      id,
      bounds,                               # recognized (x,y,w,h) — handles / connectors
      transform: { x, y, rotation, scaleX, scaleY },
      inkScaleMode: withBounds | fixedInk,
      children: Ink[]                       # content + boundary ink (role on each)
    }
  | Frame { id, bounds, children: Node[] }  # root only
  | Connector { id, from: Anchor, to: Anchor, path? }

# Boundary attachment — not free world points detached from the node.
Anchor = {
  nodeId: Id,
  # Preferred discrete ports (snap targets):
  #   rect/AABB:  north | east | south | west   (= edge midpoints)
  #   ellipse:    top | right | bottom | left   (= cardinals)
  #   line:       start | end | mid
  port?: PortId,
  # Free boundary parameter when not on a preferred port:
  #   rect/AABB edge: { edge: north|east|south|west, t: 0..1 }  (t along edge)
  #   ellipse:        { angle: radians } or { t: 0..1 } around circumference
  #   line:           { t: 0..1 } along segment
  boundary?: BoundaryParam
}
# Exactly one of port | boundary required. Resolver: Anchor → world Vec2 from live node geom.
```

**Invariants**

1. Unique `id` per node in a document.
2. `Frame` nodes appear only in `Document.rootChildren` (not under Group/Frame).
3. `Group` may nest `Ink|Text|Primitive|Group|Connector|SmartGroup` (no `Frame`).
   `SmartGroup` v0 children are **Ink only**. Connectors may be root or Group children.
4. Connector `from.nodeId` / `to.nodeId` must resolve at apply time or connector is marked
   `invalid` (still in tree until product removes it).
5. Tree order = default z-order (later siblings paint above).
6. Connector anchors attach to the **boundary** of the target node’s geometry (rect edges,
   ellipse circumference, line segment, group/frame/**SmartGroup** AABB). Preferred ports are
   edge midpoints (rect) or cardinals (ellipse); free boundary parameters are also valid.
7. Moving/resizing a node **re-resolves** anchors so connector endpoints stay on that boundary.
8. `SmartGroup` uses **local** child coordinates + group transform
   ([ADR-0011](../../../../adr/ADR-0011-smart-group.md)); ordinary Group/Frame stay world-space
   children in v0.
### Three representations

| Representation | Role | Mapping | Shipped? |
|---|---|---|---|
| **In-memory tree** | Target session SoT | Above | Library yes; live paint no |
| **Persistence** | Disk SVG profile | Serialize/deserialize tree ([srs-data](./srs-data.md)) | Library yes; UI chrome no |
| **Transmit** | Op-log on wire | Append-only ops | Unit/session helpers only |
| **WorldLayer snapshot** | Interim tablet picture | `doc_snapshot` nodes | **Live** Infini→Epaper |

### Response fields that drive UI

| Field | Required | Drives SRS-UI |
|---|---|---|
| `doc.status` | yes: `none\|open\|dirty\|error` | states matrix |
| `doc.title` / `doc.path` | when open | `ind.doc_title` |
| `doc.errorMessage` | when error | DocError copy |
| `doc.rootChildren.length` | yes ≥0 | empty vs populated world |
| `doc.stats.inkCount` etc. | optional | not required in chrome v0 |

### Closed ids (actions)

| Order | id | Route kind | Target |
|---|---|---|---|
| 1 | `cta.doc_new` | client-action | reset tree → `doc.open` clean |
| 2 | `cta.doc_open` | client-action | parse file → open/error |
| 3 | `cta.doc_save` | client-action | serialize → clear dirty |
| 4 | `op.append_ink` | session-op | insert/append Ink under parent |
| 5 | `op.create_group` | client-op | wrap ids in Group |
| 6 | `op.create_frame` | client-op | root Frame |
| 7 | `op.create_connector` | client-op | Connector between anchors |
| 8 | `op.create_text` | client-op | Text leaf |
| 9 | `op.create_primitive` | client-op | Primitive leaf |
| 10 | `op.create_smart_group` | client-op | SmartGroup from ink ids + bounds |
| 11 | `op.recognize_enclose` | **internal** | Guarded evaluation of an `intent: enclose` stroke — not a wire op (ADR-0013 §3) |
| 12 | `op.set_smart_transform` | client-op | translate/scale SmartGroup (rotation out of pilot) |
| 13 | `op.set_ink_scale_mode` | client-op | `withBounds` \| `fixedInk` |
| 14 | `cta.tool_select` | client-action | Arm `Selection` — device-local, never on the wire |
| 15 | `cta.tool_ink_box` | client-action | Arm `Ink-box` — device-local, never on the wire |
| 16 | `cta.undo` | client-action | Restore previous snapshot ([SRS-IN-12](#srs-in-12-undo-history)) |

### Routes / presentations (UI-driving)

| Control / trigger | Nav kind | To state | Precondition | On failure |
|---|---|---|---|---|
| `cta.doc_new` | client | `doc.open` | — | — |
| `cta.doc_open` | client | `doc.open` / `doc.error` | file chosen | stay prior; set error |
| `cta.doc_save` | client | `doc.open` clean | dirty or always-write | `doc.error` message |

### Errors / partial failure (UI-visible)

| Case | UI expectation |
|---|---|
| Parse failure | `doc.error`; tree unchanged |
| Missing connector endpoint on apply | Connector `invalid`; still listed; renderer draws fallback or hides stroke |
| Save I/O failure | Keep dirty; show error |

### Other logic

#### Handwriting (`Ink`)

- Geometry = ordered **samples** in **world** coordinates (dense polyline through `x,y`).
- Each sample is a tablet ink record:
  - **Required:** `x`, `y` (world).
  - **Optional (when the tablet reports them):** `pressure`, `tiltX`, `tiltY`, `distance`,
    `timestamp`, proximity/button flags, and any other ink-belonging channel from the device
    pipeline — stored, not discarded.
  - **Extensible:** `extras: { [channel: string]: number | boolean | string }` for channels
    not yet first-class in the schema.
- Parallel arrays are discouraged; prefer `samples: InkSample[]` so channels stay aligned.
- v0 does **not** fit Bézier; recognition to `Primitive` is a separate op that may add a
  Primitive and optionally hide/remove source Ink (product later).
- Render may use only position (+ pressure for width) today; persistence/transmit must
  preserve the full sample set that was captured.
- Ink `style.strokeWidth` (and pressure→width result) is in **world units**; paint scales by
  viewport / panel fit ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)).
#### Text

- `runs[]`: plain strings + optional marks (bold) — rich text minimal.
- Layout box is axis-aligned in world space; wrapping is renderer concern.

#### Groups & frames

- Group bounds = AABB union of descendants (cached).
- Frame has explicit `bounds`; children coordinates are world (v0 **no** local frame space)
  unless a later ADR introduces local transforms — **keeps sync simple**.
- Reparent op updates parent pointer / children arrays atomically in one op.

#### Smart Group (`SmartGroup`) — [ADR-0011](../../../../adr/ADR-0011-smart-group.md)

- Ink children live in **group-local** coordinates; world = `transform ∘ local` (with mode rules).
- Each ink child has `role: content | boundary`.
- **`bounds`:** recognized axis-aligned `(x, y, width, height)` from the enclose stroke at
  create (or AABB on explicit create). Used for selection handles, hit-testing, connector
  ports. Updated under group transform — **not** a substitute for the drawn box.
- **Boundary ink:** the enclose stroke is **preserved** as `role: boundary` ink (full tablet
  samples). It **always** follows SmartGroup transforms (translate / rotate / scale). It is
  **not** controlled by `inkScaleMode`.
- `inkScaleMode` applies to **`role: content` ink only**:
  - `withBounds` — content ink scales/rotates with transform.
  - `fixedInk` — each content ink keeps sample size fixed and tracks the box via **its own**
    UV / `layoutOffset` (ADR-0011 §3). Newly appended ink sets its own offset and does not
    mutate siblings. Geometric bounds + boundary ink still transform.
- **Free layout:** appending content never reflows or shifts siblings ([SRS-IN-15](#srs-in-15-draw-into-membership)).
- **Paint / z order:** tree sibling order (later siblings paint above). Used as the tie-break for
  membership when nested Smart Groups both qualify. No separate z-index field.
- Connector target: resolve anchors against **geometric** `bounds` in world space (after transform),
  not the freehand boundary path.
- **Enclose recognition (pilot):** tool-armed `intent: enclose` → guards → immediate
  `create_smart_group` ([SRS-IN-10](#srs-in-10)).
- **Selection create (pilot):** requires a surround stroke among the selection
  ([SRS-IN-16](#srs-in-16-selection-create-surround)).
- No OCR — samples unchanged.

#### Connectors

- Store endpoint **boundary anchors** (`port` preferred snap **or** continuous `boundary`
  parameter per [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) §6).
- Resolve `Anchor → world point` from live node geometry:
  - **Rect / Frame / Text AABB / Group AABB / SmartGroup bounds:** `north|east|south|west` =
    edge midpoints; free = point on chosen edge at `t ∈ [0,1]`.
  - **Ellipse / circle:** `top|right|bottom|left` = cardinals; free = point on circumference.
  - **Line:** `start|end|mid` or `t` along segment.
- Optional cached `path` polyline for render; invalidate when either endpoint node moves.
- Simple v0 path: straight segment between resolved world points (routing later).

#### Render projection

- `flattenDrawables(doc) → Drawable[]` for spatial index (Ink — with SmartGroup transform
  applied, Text, Primitive, Connector path). Groups/Frames/SmartGroups contribute optional
  bounds overlays when selected.

#### Persistence / transmit

- Save: tree → SVG profile (see srs-data).
- Load: SVG → tree; unknown elements → skip with warning or fail closed (v0: **fail closed**
  on unknown Infini-required structure; skip foreign SVG fluff with warning log).
- Transmit: ops `{ type, payload, opId }` applied idempotently by `opId`.

---

## [SRS-IN-10] Enclose recognition (Smart Group pilot)

<!-- lifecycle: deprecated -->
<!-- deprecated: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-10] -->

> **DEPRECATED 2026-08-13** — re-homed to the device by
> [ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md). The **rules below are
> correct and are inherited verbatim** by
> [SRS-EP-10](../../../epaper/features/ink-box/srs-logic.md); only the host changed. The one clause
> that dies with this section is the trigger: there is no `intent: enclose` on the wire, because the
> device evaluates its own stroke at pen-up.
>
> Kept active-in-text for the desktop's read path (it must still *render* SmartGroups) and as the
> return path if desktop authoring comes back with multi-directional sync.

**Parent:** [REQ-04](../../prd.md#smart-group).
**ADR:** [ADR-0011](../../../../adr/ADR-0011-smart-group.md) as amended by
[ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md).

Runs **only** on a stroke whose `intent` is `enclose` — that is, one drawn while the `Ink-box`
tool was armed ([SRS-IN-13](../tablet-sync/srs-logic.md#srs-in-13-tool-intent-transport)).
A stroke drawn in any other mode is ordinary ink and is never evaluated.

| Step | Rule |
|---|---|
| Trigger | `stroke_end` for a stroke marked `intent: enclose` (local or from Epaper) |
| Candidate shape | Closed or near-closed polyline that fits an axis-aligned rect; **rectangle only** in pilot |
| Fitted bounds | AABB of the enclose stroke samples → `(x, y, width, height)` |
| Guard — size | Shorter side of the fitted rect ≥ `MIN_ENCLOSE_WORLD` (**48 world units**, ADR-0013 §6) |
| Guard — content | ≥1 ink with ≥80% of its samples inside the fitted rect |
| Guard — already grouped | Ink whose parent is already a `SmartGroup` is **skipped**; remaining ink still captures |
| Commit | `create_smart_group` immediately — no proposal, no accept step: enclose stroke becomes `role: boundary` ink, captured ink becomes `role: content` in group-local coordinates, `bounds` = fitted rect, **each content ink seeded with `layoutOffset` UV** (SRS-IN-09), doc dirty |
| Guard fails | **No-op** — the stroke stays ordinary ink; no error state, no banner |
| Undo | One undo restores the pre-op snapshot exactly ([SRS-IN-12](#srs-in-12-undo-history)) |

Recognition is an **internal** Infini step; `recognize_enclose` is not a wire op.
Quality targets live in [srs-quality](./srs-quality.md).

---

## [SRS-IN-16] Selection create — surround stroke required {#srs-in-16-selection-create-surround}

<!-- lifecycle: deprecated -->
<!-- deprecated: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-10] -->

> **DEPRECATED 2026-08-13** — re-homed to the device
> ([SRS-EP-10](../../../epaper/features/ink-box/srs-logic.md)). Rules inherited verbatim, including
> the even-odd artificial-closed-path test and the refuse-without-surround guard.

**Parent:** [REQ-04](../../prd.md#smart-group). **ADR:** [ADR-0011](../../../../adr/ADR-0011-smart-group.md) §4B.

Explicit Smart Group from a multi-ink selection (Solution 3 / `Selection` tool).

| Step | Rule |
|---|---|
| Input | ≥2 selected Ink nodes (or ≥1 content + 1 candidate surround) |
| Surround candidate | For each selected stroke `S`, build an **artificial closed path** from `S` if open (append edge first→last for the test only — **do not** mutate stored samples). Point-in-polygon uses the **even-odd** fill rule. A candidate qualifies when ≥80% of samples of **every other** selected ink lie inside that region |
| Winner | Prefer the candidate that qualifies; if several qualify, highest paint/z order (later sibling) |
| Commit | Winner → `role: boundary`; others → `role: content` in group-local coords; `bounds` = fitted AABB of the winner stroke; **each content ink seeded with `layoutOffset` UV** (SRS-IN-09) |
| Refuse | **No** qualifying surround → do not create; selection unchanged; UI shows why ([srs-ui](./srs-ui.md) — create refused state) |
| Undo | One undo restores the pre-op snapshot ([SRS-IN-12](#srs-in-12-undo-history)) |

This path always produces boundary ink when it succeeds — there is no AABB-only / hint-only Smart
Group from selection. Self-intersecting surrounds are accepted as-is under even-odd (pilot —
no geometry clean-up).

---

## [SRS-IN-15] Draw-into membership (existing Smart Group) {#srs-in-15-draw-into-membership}

<!-- lifecycle: deprecated -->
<!-- deprecated: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-10] -->

> **DEPRECATED 2026-08-13** — re-homed to the device
> ([SRS-EP-10](../../../epaper/features/ink-box/srs-logic.md)). The ≥80% containment rule, the
> highest-paint-order tiebreak, and the never-reflow guarantee are inherited verbatim. Only the
> "runs on Infini" line dies.

**Parent:** [REQ-04](../../prd.md#smart-group). **ADR:** [ADR-0011](../../../../adr/ADR-0011-smart-group.md) §7.

Runs on Infini at `stroke_end` for ordinary ink — `intent` absent or `ink` (Pen). **Never** runs
on `intent: enclose` (that path is [SRS-IN-10](#srs-in-10)).

| Step | Rule |
|---|---|
| Trigger | `stroke_end` for a new Ink node after samples are committed in world space |
| Candidates | Every SmartGroup whose **world** geometric `bounds` contain ≥80% of the stroke’s samples |
| None | Leave ink under its ordinary parent (document root in v0) — no membership |
| One | Reparent as `role: content` under that SmartGroup (samples → group-local); **seed that ink’s `layoutOffset` UV** from its AABB centroid in the current bounds; dirty doc |
| Several (incl. nested) | Reparent under the candidate with the **highest paint/z order** — tree sibling order, later siblings win; **no dual parent**; no separate z-index field |
| Layout | Do **not** translate, scale, or reflow any **existing** content ink (their `layoutOffset` values stay); new ink stays as drawn |
| Bounds | SmartGroup `bounds` are **not** expanded by membership in the pilot |
| Undo | One undo restores the pre-membership snapshot ([SRS-IN-12](#srs-in-12-undo-history)) |

Paint order = document tree sibling order (SRS-IN-04 invariant 5). Nested Smart Groups that each
contain 100% of a stroke resolve by that order (topmost / later sibling) — **implementable as
written**; no z-index field required.

### `layoutOffset` + `fixedInk` draw rule (locked)

Canonical field: [SRS-IN-09](./srs-data.md) `{ u, v }`.

| Mode | Draw / resize |
|---|---|
| `withBounds` | Scale samples by `scaleX`/`scaleY`, then rotate, then translate (today’s boundary path). `layoutOffset` is still stored but unused for placement |
| `fixedInk` | Do **not** scale content samples. Target local centroid = `(bounds.x + u·width, bounds.y + v·height)`. Translate samples so their AABB centroid matches that target, then apply rotation + group translate only |

`smartLocalToWorld` today is `local + translate` under `fixedInk` with **no** UV — implement stories for SRS-IN-11 must close that gap; do not ship `fixedInk` resize against the current helper as-is.

---

## [SRS-IN-11] Selection, hit-testing, and Smart Group manipulation {#srs-in-11-selection-manipulation}

<!-- lifecycle: deprecated -->
<!-- deprecated: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-11] -->

> **DEPRECATED 2026-08-13** — re-homed to the device
> ([SRS-EP-11](../../../epaper/features/ink-box/srs-logic.md)). The gesture table, the LOD cutoff
> rule, and **one op per completed gesture** are inherited. The desktop tool table dies with
> [infini REQ-04](../../prd.md#smart-group); the `TILE_LOD_SCALE` value **0.35** is a desktop
> constant and the device must derive its own (open question, architect).

**Parent:** [REQ-04](../../prd.md#smart-group). **ADR:** ADR-0013.

### Tool modes (Infini)

| Tool | Pointer press on empty canvas | Pointer press on a pickable |
|---|---|---|
| `Selection` (default) | Pan (today's behaviour); clears selection | Select + begin move |
| `Ink-box` | Draw an enclose stroke (`intent: enclose`) | Same — strokes are not blocked by content |

Tool state is **device-local UI state** — not in the document, not on the wire (ADR-0013 §1).

### Hit-testing

| Rule | Value |
|---|---|
| Pickable set (pilot) | `SmartGroup` nodes only — resolved against world `bounds` after transform |
| Resolution order | Topmost first (later siblings paint above, so they pick first) |
| Hit region | Inside `bounds`, plus a handle tolerance band of 8 CSS px when selected |
| LOD cutoff | Picking is **disabled** below `TILE_LOD_SCALE` (0.35) — `allowIndividualInteraction` |
| Below cutoff | Press falls through to pan; UI must state that manipulation is unavailable (see srs-ui) |

### Interactions

| Gesture | Precondition | Result |
|---|---|---|
| Press inside bounds + drag | scale ≥0.35 | Move — `set_smart_transform` translate; canvas does **not** pan |
| Press (no drag) | scale ≥0.35 | Select — handles appear on geometric `bounds` |
| Drag a handle | node selected | Resize — `bounds` follow the handle; under `withBounds` content scales; under `fixedInk` each content ink’s `layoutOffset` UV is preserved (draw rule above) |
| Toggle `inkScaleMode` | node selected | `set_ink_scale_mode` — `withBounds` \| `fixedInk` |
| Press empty canvas | — | Deselect; press continues as pan |
| Press another pickable | — | Selection moves to that node |

**Out of pilot:** rotation handles and connector attachment to a SmartGroup. `resolveAnchor`
computes SmartGroup world AABB for translate + scale only (`anchors.ts`), so exposing rotation
would resolve connector ports incorrectly. Landing rotation requires the anchor math first.

### Op emission

One op per completed gesture, not per frame: a drag emits a single `set_smart_transform` on
release, with intermediate frames rendered locally. Keeps the undo ring meaningful and the wire
quiet.

---

## [SRS-IN-12] Undo history {#srs-in-12-undo-history}

<!-- lifecycle: deprecated -->
<!-- deprecated: 2026-08-13 — CHL-0008 / ADR-0014 -->
<!-- superseded-by: [SRS-EP-07] -->

> **DEPRECATED 2026-08-13** — undo belongs where editing happens, so the ring moves to the device
> ([SRS-EP-07](../../../epaper/features/device-document/srs-logic.md)). Mechanism, depth 20, and
> "not covered" list are inherited verbatim ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §5).
> The **"Remote ops … one shared timeline"** row dies: there is one writer now, so there is nothing
> remote to interleave. Infini keeps no undo stack — it authors nothing.

**Parent:** [REQ-04](../../prd.md#smart-group). **ADR:** ADR-0013 §5.

| Rule | Value |
|---|---|
| Mechanism | Ring buffer of `VectorDocument.snapshotString()` values |
| Depth | **20** entries (pilot) |
| Push | Before every structural op (`create_*`, `reparent`, `remove_node`, `set_smart_transform`, `set_ink_scale_mode`) |
| Undo | Restore the previous snapshot wholesale; tree equals the pre-op tree exactly |
| Not covered | Viewport pan/zoom, tool switches, selection changes — these are not document state |
| Remote ops | Ops arriving from Epaper push a snapshot on the same ring (one shared timeline) |
| Overflow | Oldest entry drops; undo past the ring is a no-op, not an error |

Redo is **out of pilot**. Memory bound is measured in [srs-quality](./srs-quality.md).

---

## Superseded

_None._ Prior “flat ordered collection” wording is replaced by this tree (same SRS-IN-04 id;
lifecycle remains active — content thickened 2026-08-11). Smart Group added 2026-08-11.

**SRS-IN-10 content revision (2026-08-11, same id, lifecycle `active`):** the propose → accept
flow is replaced by tool-armed immediate creation with guards, per
[ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md). SRS-IN-11 and SRS-IN-12 added the same
day to cover selection/manipulation and undo, which REQ-04 depends on and which had no spec.
**SRS-IN-15** added 2026-08-11 for draw-into membership + free layout; ADR-0011 §3 `fixedInk`
per-ink offset clarified the same day. **SRS-IN-16** added for selection-create surround guard.
