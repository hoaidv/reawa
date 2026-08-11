---
feature: vector-document
parent_req: [REQ-02]
version: 0.2.0
lifecycle: active
---

# SRS — Vector document (Logic)

Architect-owned wire + tree rules for [REQ-02](../../prd.md#vector-document).
Structural decision: [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md).
Session sync: [ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md).

## [SRS-IN-04] Tree model and three representations

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

| Representation | Role | Mapping |
|---|---|---|
| **In-memory tree** | Session SoT | Above |
| **Persistence** | Disk SVG profile | Serialize/deserialize tree ([srs-data](./srs-data.md)) |
| **Transmit** | Op-log on wire | Append-only ops that mutate the tree; peers apply in order |

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
| 11 | `op.recognize_enclose` | client-op | Propose SmartGroup from enclosure stroke |
| 12 | `op.set_smart_transform` | client-op | translate/rotate/scale SmartGroup |
| 13 | `op.set_ink_scale_mode` | client-op | `withBounds` \| `fixedInk` |

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

- Ink children live in **group-local** coordinates; world = `transform ∘ local`.
- Each ink child has `role: content | boundary`.
- **`bounds`:** recognized axis-aligned `(x, y, width, height)` from the enclose stroke at
  create (or AABB on explicit create). Used for selection handles, hit-testing, connector
  ports. Updated under group transform — **not** a substitute for the drawn box.
- **Boundary ink:** the enclose stroke is **preserved** as `role: boundary` ink (full tablet
  samples). It **always** follows SmartGroup transforms (translate / rotate / scale). It is
  **not** controlled by `inkScaleMode`.
- `inkScaleMode` applies to **`role: content` ink only**:
  - `withBounds` — content ink scales/rotates with transform.
  - `fixedInk` — geometric bounds + boundary ink still transform; content ink sample size stays
    fixed (text-box padding feel).
- Connector target: resolve anchors against **geometric** `bounds` in world space (after transform),
  not the freehand boundary path.
- **Enclose recognition (pilot):** detect roughly rectangular stroke that contains ≥1 content
  ink; propose SmartGroup; on accept, reparent contained ink as `content`, keep enclose stroke
  as `boundary`, set `bounds` from fitted rect of enclose. Always undoable. Explicit create
  path required as fallback.
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

**Parent:** [REQ-04](../../prd.md#smart-group). **ADR:** [ADR-0011](../../../../adr/ADR-0011-smart-group.md).

| Step | Rule |
|---|---|
| Candidate stroke | Closed or near-closed polyline, roughly 4-sided |
| Contained ink | Ink whose samples lie mostly inside candidate polygon (≥80% of samples) |
| Propose | Transient SmartGroup preview; user accept/dismiss |
| Accept | `create_smart_group`; keep enclose stroke as `role: boundary` ink; set geometric `bounds` from fitted (x,y,w,h); reparent content ink; dirty doc |
| Reject / undo | Restore prior tree; ink untouched |

Quality targets live in [srs-quality](./srs-quality.md).

---

## Superseded

_None._ Prior “flat ordered collection” wording is replaced by this tree (same SRS-IN-04 id;
lifecycle remains active — content thickened 2026-08-11). Smart Group added 2026-08-11.
