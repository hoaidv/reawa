---
id: ADR-0041
title: Document forest, concurrent readers, progressive paint
status: accepted
date: 2026-10-04
deciders: [architect, human]
supersedes: [ADR-0040]
source: human 2026-10-04 document-model design (review rounds through acceptance)
---

# ADR-0041 — Document forest, concurrent readers, progressive paint

<a id="adr-0041"></a>

**Accepted 2026-10-04** by the human, as the in-memory document of the rebuilt
Epaper (`epaper/`, [TRACK-008](../../.plan/tracks/TRACK-008-rebuild-epaper.md),
Rebuild Epaper). Implementation plan:
[document-forest-implementation.md](../../.plan/document-forest-implementation.md).

Scope of the acceptance:

| Record | After this ADR |
|---|---|
| [ADR-0040](./ADR-0040-logarithmic-hit-test.md) (Device logarithmic hit-test spatial index) | **Superseded.** Its index was for the archived tree; the per-container R-tree replaces it. |
| [ADR-0010](./ADR-0010-tree-of-vectors.md) (Tree-of-vectors document model) | Still accepted. It governs Infini, the shared wire and ops, and `epaper_old/`. The rebuilt Epaper does not sync yet; the conversion at the sync boundary is deferred ([CHL-0034](../../.plan/iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)). |
| [ADR-0011](./ADR-0011-smart-group.md) (Smart Group (ink-box) pilot), [ADR-0039](./ADR-0039-nested-ink-box-rendering.md) (Nested ink-box RenderingContext and own-transform) | Still accepted for Infini and `epaper_old/`. For the rebuilt Epaper, `InkBox` and its manipulation rules here replace them. |
| [vector-document.md](../domain/vector-document.md) | Still the shared wire, ops and undo vocabulary. The forest hosts it without restating it. |
| Epaper SRS records on ink-box manipulation, scale mode and geometry queries | Settled 2026-10-04 in [CHL-0033](../../.plan/iter-006/challenges/CHL-0033-forest-product-records.md): query records replaced, ink-box records deprecated |

Specification: [document-forest/](../domain/document-forest/index.md).

## Revision history

| Date | Change | Reason |
|---|---|---|
| 2026-10-04 | First draft: linked-list children, world cull boxes, edits during a single-threaded walk, strip refill | Initial design |
| 2026-10-04 | One writer thread and concurrent readers; handle lists with an `orderKey`; local `bounds` on every node; `Document` is a node; `InkBox` with explicit boundary and content lists; connector payload reorganized; per-container R-tree with cost aggregates; tile-based budgeted progressive paint | Human review: the requirement is concurrency, not editing during a walk; requested `bounds`, `children`, document-as-node, `InkBox` fields, connector detail, separate algorithm files, and an evaluation of budgeted rendering |
| 2026-10-04 | `transform` reserved: manipulation never writes it. Per-kind manipulation responses; `InkBox` `manipMode` `All` / `Boundary` replaces `contentScaleMode`. `boxRect` carries position and size; children are relative to the box. Reparenting keeps world placement. Connector departure stored in the side’s face frame. | Human review: departure must stay fixed relative to its node under any node or ancestor change; moving a box into a transformed box scaled it or threw it outside the boundary |
| 2026-10-04 | Header `bounds` is the node’s own box: stored for `InkBox` and `Frame` (replacing `boxRect` and `rect`), derived for the others. The paint extent lives only in the parent’s R-tree entry. | Human review: `boxRect` played the role that `bounds` plays for every other kind |
| 2026-10-04 | Move and resize rules for `Ink`, `Primitive`, `Group`, `Frame` and `Connector`, and for multi-selection. One `resize(m)` contract with an axis-aligned map. Every container except `Document` has a translation-only child origin; `Group` gains a stored `origin`. Primitives are not rotated. | Human decisions on each kind’s behavior |
| 2026-10-04 | **Accepted.** Status `accepted`; supersedes ADR-0040; scope fixed to the rebuilt Epaper. Provisional choices become accepted starting values. R-tree root exempt from the minimum fill. Atomics written for C++17. Implementation plan written. | Human: “I adopt the design. Now we will commit to it.” |
| 2026-10-04 | Consequences updated after plan step 0: product records settled (CHL-0033); Infini sync and the wire mapping deferred (CHL-0034); implementation owned by TRACK-009. No design change. | Human: sync with Infini is out of the implementation scope |
| 2026-10-04 | Connector derived geometry type renamed from `RoutedPath` to `DerivedPath`. The field stays `route`. | Human, during implementation: the type name should say that the value is derived |

## Context

1. **Concurrency.** A reader thread renders or computes while another thread edits
   the same document. Cloning the document so that each reader has a private copy
   is forbidden. The current `DocNode` keeps children by value
   (`std::vector<DocNode> children` in `epaper_old/document/doc_model.hpp`), so any
   concurrent insert can move a node out from under a reader.
2. **Paint at every zoom.** Modest zoom needs viewport culling. Extreme zoom-out
   makes almost everything intersect the viewport, so specks must collapse into gray
   dots without being read. In a dense document at a middle zoom, both prunes still
   leave too much for one frame. Painting must be budgeted and progressive.
3. **One structure.** The spatial index has to be part of the forest, not a second
   tree kept in sync beside it.
4. **Keep connector behavior.** The warp, anchors, markers and labels of
   [ADR-0020](./ADR-0020-connector-ink-geometry.md) (Connector-ink geometry: rest shape,
   cubic and morph warps), [ADR-0038](./ADR-0038-endpoint-ink-face-frame.md)
   (Endpoint-ink on ConnectorAnchor) and [ADR-0027](./ADR-0027-attachment-t-rest-spine.md)
   (Attachment parameter t on connector rest spine) stay. Only their data is
   reorganized.
5. Stroke width stays in world units
   ([ADR-0012](./ADR-0012-world-stroke-viewport-parity.md) — World-space stroke width
   and viewport paint parity).
6. **Manipulation through the transform misbehaves.** Today a box resize scales its
   transform, and `fixedInk` content undoes that scale with a placement. A box moved
   into a box with a non-identity transform picks up that transform: it scales
   unexpectedly, or is offset outside the target’s boundary.
7. **Consistent connector look.** A connector must leave its node at the same angle
   relative to that node, whatever happens to the node or its ancestors.

## Decision

1. **One writer, concurrent readers, granule rules.** Every mutation runs on one
   writer thread. Readers read the same live nodes without locks. Small fields are
   written in place under a per-node sequence lock. Variable-size data is built
   beside the old copy and published with one pointer store. R-tree updates copy one
   root-to-leaf path. Memory is freed by epoch-based reclamation. Every commit posts
   damage after its last publish, which is what makes paint converge.
   [concurrency.md](../domain/document-forest/concurrency.md).
2. **Handles.** Nodes live in a slot table that never moves. References are
   `(slot, generation)` handles. Ids are kept for sync, undo and persistence.
   [handles.md](../domain/document-forest/handles.md).
3. **Node shape.** Every node has `id`, `type`, `parent`, `transform`, `bounds`
   and `children` (an ordered list of handle links, empty for leaves). Everything
   else is per-kind payload. `Document` is a node: the one root. `bounds` is the
   node’s own box in local space: what knobs, move, resize and anchors act on.
   `InkBox` and `Frame` store it; the other kinds derive it from their geometry. The
   **paint extent** (with stroke padding and any overhang) is not a node field. It
   is the node’s entry box in its parent’s R-tree, and culling reads only that.
4. **Kinds.** `Document`, `Frame`, `Group`, `InkBox` (containers); `Ink`,
   `Primitive` (Segment, Triangle, Square, Rectangle, Circle, Ellipse, Polygon),
   `Connector` (leaves). `InkBox` has a stored `bounds` `{x, y, w, h}` relative to
   its parent, a `boundaryPolygon`, a `manipMode`, and its children split into
   `boundaryInks` and `contents`. Its children are stored relative to the box’s
   top-left corner. [index.md](../domain/document-forest/index.md).
5. **Connector payload.** Source and target ends (`NodeRef`, side or centre anchor,
   `departure`, marker, `lastPose`); `style` `Curve` or `Ink`; a fixed `RestPath`;
   `labels`; and a writer-derived `route`. Same algorithms as today. A side end’s
   `departure` is stored in that side’s face frame and mapped through the frame
   rebuilt from the side’s current world image, so the angle to the side never
   changes. [connector.md](../domain/document-forest/connector.md#departure).
6. **Per-container R-tree in child space, with cost aggregates.** It serves paint,
   hit-testing, lasso, membership, erase candidates, snapping and render budgeting.
   [spatial-index.md](../domain/document-forest/spatial-index.md).
7. **One paint descent, two prunes.** Outside the region: skip. Smaller than `τ` on
   screen: one gray dot, nothing below it read. This applies to R-tree cells, not
   only to nodes. Exact queries never use the size prune.
8. **Budgeted progressive paint on tiles.** The padded image is split into tiles.
   A render-thread queue completes them in time-bounded slices, in priority order.
   Jobs are pixel regions stamped with a camera epoch. A best-first coarse pass gives
   a first picture when no pixels exist yet. This adopts the human’s tail-queue
   proposal, with three changes: cost replaces node count, a best-first frontier
   replaces a depth cut, and region jobs replace subtree jobs.
   [rendering.md](../domain/document-forest/rendering.md).
9. **Reserved transform; per-kind manipulation.** No manipulation writes
   `transform`; every node has the identity transform until a later decision uses it.
   Each kind answers `move(Δ)` and `resize(m)`, where `m` is an axis-aligned
   scale-and-shift. A container passes `m` down as a pure scale after moving its
   child origin.
   - `Ink`: samples shift or map, free non-uniform.
   - `Primitive`: points or parameters shift or map. `Square` and `Circle` always
     keep their aspect and never change kind.
   - `Group`: move writes only its stored `origin`; resize scales every child.
   - `Frame`: an artboard. Move shifts `bounds`; resize changes only the rectangle
     and clip.
   - `InkBox`: move changes `bounds.min`. A knob changes `bounds` and rescales
     child-space geometry: in `All` mode the boundary and the contents, each nested
     box applying its own mode; in `Boundary` mode the boundary only, with content
     keeping its coordinates relative to the box.
   - `Connector`: select-only while attached; movable, by its saved end positions,
     only when both ends are detached.

   A multi-selection applies the same `Δ` or `m` to each top-most selected node.
   [index.md](../domain/document-forest/index.md#manipulation).
10. **Reparenting keeps world placement.** The node is moved by the translation
    between the old and new parents’ child spaces. Once transforms are used, the node’s
    transform is recomposed instead, so it neither jumps nor scales.
    [index.md](../domain/document-forest/index.md#reparenting).

## Consequences

- A render reader may mix before- and after-commit states inside one job. It is
  wrong only inside damaged areas, and those are redrawn. Queries that need
  consistency retry against a commit sequence, then fall back to the writer.
  Decisions that lead to edits run on the writer.
- Writes copy only what they change: one buffer, one child list, one R-tree path.
  Updating a 100k-child root costs about 64 entries for the R-tree plus that one
  list. If the list copy becomes measurable, the list is chunked.
- Visits compose translations only. There is no `fixedInk` placement, no `S⁻¹` and
  no per-ink layout UV. Moving a box writes one rectangle and one parent R-tree entry.
- Resize is a geometry edit at commit time. An `All` resize rewrites every sample in
  that subtree and rebuilds its R-trees. That happens once per commit; the live drag
  is a tool-overlay preview.
- Moving a box into another box no longer scales it or throws it outside the target.
- Moving any container writes one origin and one parent R-tree entry, because
  `InkBox`, `Frame` and `Group` all store children relative to a translation-only
  child origin. That origin must never gain scale or rotation, or it becomes the
  reserved `transform` under another name.
- Primitives are not rotated while `transform` is reserved. A rotated shape is
  stored as a `Polygon`.
- Move and resize of `Ink`, `Primitive`, `Group`, `Frame`, and of detached
  connectors, are new product behavior. Today only the ink box has them.
- A side anchor’s `side + t` lies on the box’s current `bounds`, so the attach point
  keeps its fraction along the side through any resize. Erasing boundary ink widens
  or narrows only the paint extent, so it never moves a knob or an anchor.
- `bounds` and the R-tree entry box differ by design. Code that culls or prefilters
  must read the entry box, never `bounds`.
- For every box this model can hold today (axis-aligned in world), the face-frame
  `departure` gives the same facing as the box-axes copy the live warp reads. The
  connector payload therefore changes no warp output. Reorganizing it surfaced code
  drift from ADR-0020 on centre ends: the code has no 60° cone and does not clip
  ([connector.md](../domain/document-forest/connector.md#adr-0020-drift)). By
  human decision, centre ends keep the shipped ray toward the peer. Only side ends
  hold a fixed angle to their node.
- Moving to this model changes resize behavior for current boxes. A `withBounds` box
  maps to `All`. A `fixedInk` box maps to `Boundary`: its content keeps its
  coordinates relative to the box (human decision), so a top or left knob carries
  the content with the corner instead of re-centring each ink at its UV position.
- Tiles, slices, dots and the coarse pass are renderer policy. The document only
  answers region visits.
- Acceptance supersedes ADR-0040 now. The Epaper product records were settled on
  2026-10-04 in [CHL-0033](../../.plan/iter-006/challenges/CHL-0033-forest-product-records.md)
  (Epaper product records under the document forest):
  - SRS-EP-79 and SRS-EP-78 are retired; their successors
    [SRS-EP-80](../modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries)
    and [SRS-EP-81](../modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality)
    specify the geometry queries and their quality bars on the forest;
  - REQ-06 and SRS-EP-10, -11, -12, -14, -21, -75, -76 and -77 are `deprecated`. They
    still describe `epaper_old/`; each gets a successor when its tool is ported;
  - REQ-08 stays parked; SRS-EP-09 is not affected.
- The sync wire is unchanged. Connecting the rebuilt Epaper to it would need a
  translation at the boundary: child-relative coordinates to and from the wire's
  transforms, and `manipMode` to and from `inkScaleMode`. `Boundary` and `fixedInk`
  differ on a top or left knob, so a box resized on the device could look different on
  Infini. The human put Infini sync out of the implementation scope on 2026-10-04; the
  choice is deferred in [CHL-0034](../../.plan/iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md).
- Implementation: [TRACK-009](../../.plan/tracks/TRACK-009-document-forest.md) (Document
  forest), following [document-forest-implementation.md](../../.plan/document-forest-implementation.md).

## Alternatives considered

| Topic | Option | Why not |
|---|---|---|
| Concurrency | One reader–writer lock | Edits would wait for visits. Credible as a first step if render jobs are capped at about 10 ms; kept as the fallback. |
| Concurrency | Persistent tree, path copy to the root | Every edit copies all of its ancestors’ lists. Revisit if multi-node consistent snapshots become common. |
| Concurrency | Separate render replica, or a clone per visit | A second document |
| Children | Linked siblings (first draft) | Several pointer writes per change cannot be published atomically to a concurrent reader; no indexing |
| Children | List of node values | A growing array moves the nodes inside it |
| Bounds | World cull box per node (first draft) | Moving a box rewrites every descendant |
| Index | One global R-tree, rebuilt (ADR-0040 style) | A second structure; a rebuild races readers; no size prune |
| Index | Quadtree | Overlapping ink boxes collect at the root, so the size prune cannot stop early |
| Budget | Cumulative node count, cut at one depth | Does not divide flat or balanced trees; one cut for the whole document; count is not cost |
| Budget | Deferred subtree jobs | Paints out of order; leaves stand-ins behind; holds tree positions across edits |
| Budget | Clear the queue on any new request | Throws away valid work on pan and edit; races. Replaced by epochs. |
| Manipulation | Resize through `transform`, with a scale mode on content (previous version) | Scale leaks into anything moved into the box; content needs an inverse-scale placement |
| Bounds | `bounds` = paint extent, with a separate `InkBox.boxRect` (previous version) | Two names for one concept on different kinds; padding and boundary overhang leak into knobs and anchors |
| Departure | Store in box axes, map through the box’s linear map | Skews under uneven scale with rotation; correct today only because every box is axis-aligned |

**Reversal conditions.** If the granule rules cost too much to build, switch to a
reader–writer lock with slice-sized jobs. If exact queries fall back to the writer
often, revisit persistent snapshots. If dots must be one per node, the zoom-out
claim of decision 7 no longer holds.

## Verification

| Claim | Status |
|---|---|
| Design specified in [document-forest/](../domain/document-forest/index.md) | Written; **accepted** by the human 2026-10-04 |
| Human check of the provisional choices ([index.md](../domain/document-forest/index.md#provisional-choices)) | Done: adopted with the design 2026-10-04. The numeric starting values stay open to tuning by measurement. |
| Epaper SRS supersession | **Done** 2026-10-04 ([CHL-0033](../../.plan/iter-006/challenges/CHL-0033-forest-product-records.md)) |
| Wire mapping of `manipMode` | **Deferred** with Infini sync ([CHL-0034](../../.plan/iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)) |
| Connector payload changes no warp output | Checked by reading the code on both peers; fixture run **pending** |
| Fixture equivalence: connector route, anchor mapping | **Pending** |
| Reparent keeps world placement; departure angle survives resize and reparent | **Pending** tests |
| Per-kind move and resize rules, including aspect lock and the detached connector | **Pending** tests |
| Centre-end facing (ray to the peer); `Boundary`-mode content (relative to the box) | Decided by the human, 2026-10-04 |
| Concurrency stress under ThreadSanitizer and AddressSanitizer; convergence pixel diff | **Pending**; no implementation |
| Slice-time, first-present and refresh-count measurements on the device | **Pending** |
