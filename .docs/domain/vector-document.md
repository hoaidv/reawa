---
title: Vector document — shared domain model
lifecycle: active
owner: architect
source: [ADR-0010, ADR-0011, ADR-0014, ADR-0032]
implemented_by:
  - epaper — device-side working document (C++/Qt)
  - infini — mirror + persistence (TypeScript)
---

# Domain — Vector document

The document is a **tree of vectors**. Since
[ADR-0014](../adr/ADR-0014-document-ownership-inversion.md) both peers hold one: the device holds
the **working document** it edits, the desktop holds a **mirror** it persists. This file is the
single place their meaning is defined; each module's SRS binds its implementation to it.

Authority: [ADR-0010](../adr/ADR-0010-tree-of-vectors.md) (structure) ·
[ADR-0011](../adr/ADR-0011-smart-group.md) (SmartGroup) ·
[ADR-0014](../adr/ADR-0014-document-ownership-inversion.md) (who writes) ·
[ADR-0015](../adr/ADR-0015-one-way-sync-contract.md) (how changes travel) ·
[ADR-0032](../adr/ADR-0032-inverse-op-undo.md) (inverse-op undo, node `lastOpId`).

## Node kinds

| Kind | Holds | Notes |
|---|---|---|
| `Ink` | Polyline of samples — position plus any tablet-reported channels (pressure, tilt) | Handwriting is first-class; **never** Bézier-fitted in v0, **never** OCR'd |
| `Text` | Paragraph text in world space | Desktop-authored |
| `Primitive` | `line` \| `rect` \| `ellipse` | Parameterized geometry |
| `Group` | Children, nestable anywhere | World-space children in v0 (no local transform) |
| `Frame` | Children, **root only** | Artboard / work-area metaphor |
| `Connector` | References two node ids; rest spine; warp style; per-end terminal; optional attachments | Endpoints attach to a node's **boundary**. Terminal + attachments: below |
| `SmartGroup` | Ink children tagged `role: content \| boundary` | The ink-box — see below |

### Invariants

- Paint order **is** tree sibling order. Later siblings paint above. There is no z-index field.
- `Frame` exists only at the document root; frames do not nest.
- A `Group` with zero children is invalid on commit.
- A `SmartGroup` whose last child is removed is removed itself; its ink returns to the parent.
- Node ids are stable across serialization, sync, and undo. Do **not** reuse a node id inside a document epoch.
- Each node carries `lastOpId` — see [Node revision](#node-revision).
- Stroke width is in **world units** everywhere ([ADR-0012](../adr/ADR-0012-world-stroke-viewport-parity.md)).

## Node revision

`lastOpId` is the `opId` of the last **forward** document-semantic mutation still in effect on that node. New nodes start at the creating op’s `opId`. Undo entries store each target’s `prevLastOpId` (the forward id before this gesture). Undo restores it; redo restamps the original `forwardOpId`. A mismatch vs the entry’s forward `opId` at undo time is **skip**, not clobber ([ADR-0032](../adr/ADR-0032-inverse-op-undo.md)). History publish ids (`undo:N` / `redo:N`) are not written onto nodes.

**Counts as a change** (must update `lastOpId`): parent id or sibling index; geometry (transform, bounds, ink samples including erase, `inkScaleMode`, `layoutOffset`); connector stored fields (`from` / `to`, rest shape `S`, `warpStyle`, `terminal`, attachments); text runs; primitive geometry; kind-specific author fields.

**Does not count** (must **not** update `lastOpId`): derived paint, hit caches, warped polyline `V`, attachment world pose; connector last-live-pose cache when an endpoint is missing; selection, tool mode, viewport, follow, publish-queue metadata; clipboard slot; envelope `seq`.

`lastOpId` is working-document anatomy. It is not required on SVG persist (session stacks empty on `doc_load`). Infini applies it for mirror fidelity when present; Infini does not author an undo stack.

## SmartGroup (ink-box)

The only kind with a **local transform** in v0 — ordinary `Group` and `Frame` keep world-space
children until a later ADR.

| Aspect | Rule |
|---|---|
| Children | `Ink` only in v0, each tagged `role: content` or `role: boundary` |
| `bounds` | Axis-aligned `(x, y, width, height)` in local space; recognized once at creation, updated on transform. Drives handles, hit-testing, connector ports |
| Boundary ink | The creator's own enclose / surround stroke, preserved. **Always** transforms with the frame, never gated by `inkScaleMode`, never replaced by a synthetic rectangle. Brush/area may **clip** this ink ([ADR-0034](../adr/ADR-0034-erase-clip-remnants.md)); a broken surround is allowed |
| Boundary polyline | Invisible closed polyline seeded at create from the enclose stroke. Transforms/resizes **with** boundary ink. **Persisted.** Never clipped by erase. Object-erase 80% uses this polygon’s **area** |
| Transform | `{ translate, rotation, scaleX, scaleY }`; children authored in group-local coordinates. World draw = `groupTransform ∘ localSample` |
| `inkScaleMode` | `withBounds` (default) — content scales with the group · `fixedInk` — content keeps sample size and tracks the box by its own UV |
| `layoutOffset: {u, v}` | Per **content** ink. `u = (cx − bounds.x)/width`, `v = (cy − bounds.y)/height` of that ink's AABB centroid in group-local space, seeded at create / membership / selection-create |
| Connector target | Anchors use the geometric `bounds`, not the wiggly ink path |

**`fixedInk` placement rule.** On a bounds or scale change: do **not** scale content samples; place
each content ink so its centroid sits at `(bounds.x + u·width, bounds.y + v·height)`, then apply
rotation and group translate. Newly appended content gets its own `{u, v}` and never adjusts
siblings. No reflow, no alignment, no OCR.

### Creation

| Path | Rule |
|---|---|
| **Enclose** (tool-armed) | A roughly rectangular stroke, evaluated at pen-up. Guards: fitted rect ≥ **48 world units** on the shorter side **and** contains ≥1 ink with ≥80% of its samples inside. Enclose stroke → `boundary`; contained ink → `content`; `bounds` = fitted rect |
| **Selection** (explicit) | One selected stroke must contain ≥80% of each other selected stroke's samples. It may be **open** — build an artificial closed path for the containment test only, never mutating stored samples. That stroke → `boundary`; the rest → `content`. **No qualifying surround stroke → refuse**; there is no AABB-only SmartGroup |
| **Never unprompted** | A rectangle drawn with the plain pen tool is ordinary ink, always |

### Draw-into membership

At `stroke_end` for ordinary ink: candidate SmartGroups are those whose **world** `bounds` contain
≥80% of the new stroke's samples. None → the ink stays where it is. One or more → reparent as
`content` under the candidate with the highest paint order (later sibling wins; nested boxes resolve
the same way). Membership never shifts, realigns, or reflows existing content.

## Operations

Ops are the unit of change. Both implementations apply them **idempotently by `opId`**; a duplicate
apply is a no-op and an unknown op type must not crash.

| Op | Meaning |
|---|---|
| `append_ink` | Add an `Ink` node |
| `create_smart_group` | Create a SmartGroup from a boundary stroke + captured content |
| `set_smart_group_transform` | Replace the group's transform / bounds |
| `set_ink_scale_mode` | Switch `withBounds` ↔ `fixedInk` |
| `reparent` | Move a node between parents (draw-into membership) |
| `remove` | Delete a node |
| `set_ink_samples` | Replace an Ink node’s samples (erase clip remnant keep-id, and its inverse) |
| `compound` | Apply several tree ops as one atomic gesture (multi-inverse undo/redo) |
| `restore_snapshot` | Last-resort wholesale replace — **not** the undo path ([ADR-0032](../adr/ADR-0032-inverse-op-undo.md)). Tests / emergency only |
| `set_connector_end_style` | Set `terminal[end].style` on one connector end ([ADR-0026](../adr/ADR-0026-endpoint-ink-membership.md) Path A) |
| `bind_endpoint_ink` | Bind a stroke as `terminal[end].ink` (Path B); does not rebake rest spine |
| `bind_attachment` | Bind `nodeId` to connector rest-spine `t` ([ADR-0027](../adr/ADR-0027-attachment-t-rest-spine.md)) |
| `duplicate_subtree` | Paste: insert already-minted node bodies ([ADR-0024](../adr/ADR-0024-in-document-clipboard.md)) |
| `create_frame` | Insert a root `Frame` at placed bounds ([REQ-17](../modules/epaper/prd.md#manual-create) — epaper) |
| `create_primitive` | Insert `line` \| `rect` \| `ellipse` geometry (not a polyline stand-in) |

Wire framing for ops is **not** here — see [ADR-0015](../adr/ADR-0015-one-way-sync-contract.md) §2
and the module SRS.

## Connector terminals and attachments

Shared anatomy consumed by epaper (author) and infini (mirror + SVG). Behaviour IDs stay in module SRS.

### Terminal (per end)

| Field | Shape | Notes |
|---|---|---|
| `style` | closed id | v1: `none` \| `arrow` \| `arrow_empty` \| `star` \| `one` \| `many` ([REQ-13](../modules/epaper/prd.md#connector-ends)) |
| `ink` | optional polyline in end frame | Endpoint decoration; `(s, d)` against **rest** spine at bind ([ADR-0026](../adr/ADR-0026-endpoint-ink-membership.md)) |

Styles ride warp with the end. They are not a second connector and not free Ink.

### Attachments

| Field | Shape | Notes |
|---|---|---|
| `nodeId` | existing node | Text, Primitive, Ink, or SmartGroup in v1 — not Frame |
| `t` | `[0, 1]` | Normalized arc-length on **rest spine S**, never on warped `V` ([ADR-0027](../adr/ADR-0027-attachment-t-rest-spine.md)) |
| `offset.d` | world units | Signed perpendicular offset at bind; may be 0 |

The connector remains **not** a spatial parent. Attachment world pose is derived: point on warped `V` at `t`, plus `d` along the local normal. Moving a bound box does not emit attachment ops.

## Device clipboard (not a document node)

The in-document clipboard slot is a **process-global singleton on Epaper**, not a tree kind, not SVG, and not a field of `DeviceDocument` ([ADR-0037](../adr/ADR-0037-device-clipboard-singleton.md)). Infini does not author it. Cut/paste **do not publish** this track; Infini apply of the resulting tree edits is a later track. The **session undo stack** is also device-local (one document-epoch LIFO pair, depth 20); Infini has none ([SRS-IN-12](../modules/infini/features/vector-document/srs-logic.md#srs-in-12-undo-history) stays deprecated).

## Ownership and authority

| Role | Peer | Since |
|---|---|---|
| Sole writer during a session | **Epaper** (device) | [ADR-0014](../adr/ADR-0014-document-ownership-inversion.md) |
| Mirror — applies published changes, never authors | **Infini** (desktop) | ADR-0014 |
| Persistence — SVG profile serialize / parse | **Infini** | [ADR-0010](../adr/ADR-0010-tree-of-vectors.md) |
| Initial full-document load | Infini → Epaper, handshake-gated | [ADR-0015](../adr/ADR-0015-one-way-sync-contract.md) §4 |

Persistence is a **serialization** of the model, not a second source of truth during a live session.

## Two implementations, one meaning

| Concern | Epaper (C++/Qt) | Infini (TypeScript) |
|---|---|---|
| Working document | **Yes** — the authority in-session | Mirror only |
| Recognition + transforms | **Yes** | No (deprecated — [infini REQ-04](../modules/infini/prd.md#smart-group)) |
| Undo | **Yes** — inverse-op ring, depth 20, `lastOpId` skip ([ADR-0032](../adr/ADR-0032-inverse-op-undo.md)). `restore_snapshot` is last-resort **non-undo** | No |
| SVG serialize / parse | No | **Yes** |
| Change publishing | **Yes** — emits | Applies |

They must agree on shared fixtures
([`fixtures/ops/`](../modules/infini/features/vector-document/fixtures/ops/)). Any divergence in
geometry, guard thresholds, or membership resolution is a `CHL-*`, not a per-module tweak.

## Bound by

| Module SRS | Binding |
|---|---|
| [infini SRS-IN-04](../modules/infini/features/vector-document/srs-logic.md) | Tree implementation, ops, invariants |
| [infini SRS-IN-09](../modules/infini/features/vector-document/srs-data.md) | SVG profile + transmit schemas |
| epaper `[SRS-EP-07]`+ | Device-side tree, ingestion, recognition, manipulation, undo |

## Deferred

- Local transforms for ordinary `Group` / `Frame`.
- Non-rectangular enclosure (ellipse, lasso).
- In-box alignment / reflow.
- Bézier fitting of handwriting.
- Multi-directional sync and conflict resolution — a later ADR replaces
  [ADR-0015](../adr/ADR-0015-one-way-sync-contract.md) §2–§4.
