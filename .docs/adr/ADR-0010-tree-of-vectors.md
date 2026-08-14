---
id: ADR-0010
title: Tree-of-vectors document model
status: accepted
date: 2026-08-11
deciders: [architect, pm]
supersedes: null
amended-by: [ADR-0020]
---

# ADR-0010 — Tree-of-vectors document model

## Context

[REQ-02](../modules/infini/prd.md#vector-document) needs more than a flat list of strokes.
Creators work with **mostly handwritten polylines**, but also text boxes, recognized or
inserted primitives, **groups** (Figma-like composites that may nest anywhere), **frames**
(root-level visual containers), and **connectors** (edges between nodes). Epaper sync
([ADR-0009](./ADR-0009-shared-document-viewport.md)) already assumes an append-only op-log
over a shared in-memory model — that model must be a **tree**, not a bag of leaves.

Infinity-canvas v0 ([SRS-IN-01](../modules/infini/features/infinity-canvas/srs-logic.md))
already paints a flat primitive list; the document feature must define how that list is
derived from the tree (flatten / visit) without abandoning spatial cull.

Quality stakes: round-trip fidelity (≤1 px @ 100%), transmit op equality, and future
tablet ops that append ink under a parent (frame/group or document root).

## Decision

1. **Composite document tree**  
   `Document` has an ordered list of **root children**. Allowed root kinds: `Frame`, and
   free-floating leaf/composite nodes that are not required to live in a frame (`Ink`,
   `Text`, `Primitive`, `Group`, `Connector`). **Frames may only appear at the document
   root** (not nested inside groups/frames in v0). **Groups may nest anywhere** (composite
   pattern): a `Group` contains ordered children of the same node union (except `Frame`).

2. **Leaf / edge node kinds (v0)**  

   | Kind | Geometry | Notes |
   |---|---|---|
   | `Ink` | Dense **polyline of tablet samples** in world space | Dominant content; RM handwriting. Each sample carries position plus **any tablet-reported ink channels** present for that point (pressure, tilt, …). **Not** fitted to Bézier in v0. |
   | `Text` | Axis-aligned box + paragraph runs | Desktop textbox typing; world-space position/size. |
   | `Primitive` | line / rect / ellipse (extensible enum) | Inserted or recognized; same as canvas demo kinds. |
   | `Group` | Bounds = union of children (+ optional local transform later) | Visual clustering; children keep relative structure. |
   | `SmartGroup` | Explicit bounds + **local transform** + ink children | Ink-box pilot ([ADR-0011](./ADR-0011-smart-group.md)); connector target; `inkScaleMode`. |
   | `Frame` | Explicit world AABB (+ optional clip) | Root-only artboard-like container. |
   | `Connector` | Curve/segment between **boundary anchors** on two node ids | References endpoints; not a spatial parent. Anchors attach to standardized or free points on the target’s **boundary** (see Decision §6). |

3. **Identity & ops**  
   Every node has a stable `id`. Mutations are **ops** on the tree (append ink, insert
   node, reparent into group, create frame, set connector endpoints, set text, …). Live
   session source of truth remains the op-log ([ADR-0009](./ADR-0009-shared-document-viewport.md));
   SVG persistence is a serialization of the materialised tree.

4. **Render / cull projection**  
   Painters walk the tree (skip collapsed/hidden if introduced later) and emit drawable
   leaves + connector paths into the spatial index. Groups/frames contribute bounds for
   hit-testing and culling but are not themselves stroked unless chrome shows selection.

5. **Persistence mapping (v0 direction)**  
   SVG profile maps: `path`/`polyline` → Ink; `text` → Text; `rect`/`ellipse`/`line` →
   Primitive; nested `g` with Infini `data-kind=group|frame` → Group/Frame; connectors as
   `path` + `data-kind=connector` + endpoint ids in attributes (or sibling metadata block).
   Exact attribute schema lands in `srs-data` / profile appendix — not ad-hoc per story.

6. **Connector anchors (boundary attachment)**  
   An anchor is always on a **node boundary**, not an arbitrary interior point (except when
   the product later adds “center” as an explicit kind — v0 default UX is boundary).

   | Target shape (bounds) | Preferred ports (snap) | Also allowed |
   |---|---|---|
   | Axis-aligned **rect** / square / frame / text box AABB | Midpoints of the **4 edges** (N/E/S/W) | **Any point on the 4 edges** (parameter along edge) |
   | **Ellipse** / circle | Cardinal points **top, right, bottom, left** | **Any point on the circumference** (angle or arc-length parameter) |
   | **SmartGroup** | Prefer edge midpoints of transformed bounds | Any point on the 4 edges of the SmartGroup boundary |
   | **Group** / ink cluster | Prefer AABB edge midpoints of the group’s union bounds | Any point on that AABB boundary |
   | **Line** primitive | Prefer endpoints; optionally midpoint | Any point along the segment |

   Snap UX should bias to preferred ports; free boundary points remain valid persisted
   anchors (`port` enum **or** continuous parameter — see SRS-IN-04 / SRS-IN-09).
   Resolving an anchor to world coordinates must recompute when the target node moves or
   resizes so connectors stay glued to the boundary.

7. **Ink samples (tablet channels)**  
   An `Ink` stroke is an ordered sequence of **samples**, not bare `{x,y}` only. A sample
   always has world **position**; it **may** include any channel the tablet reports that
   belongs to the ink — at minimum when available: `pressure`, `tilt` (`tiltX`/`tiltY` or
   equivalent), and other device fields the session chooses to preserve (e.g. distance,
   proximity, timestamp, button state). Unknown future channels are stored in an extensible
   `extras` map rather than dropped. Renderers may ignore channels they do not use; round-trip
   must not strip channels that were present on save/transmit.

## Consequences

- Implement stories must replace flat `Primitive[]` with a tree + flatten visitor (or dual
  index). Spatial index keys leaves (and optionally group/frame AABBs).
- Epaper v0 may only emit `Ink` ops parented to document root or a designated frame; richer
  tree edits can be Infini-first.
- Connectors create referential integrity rules (delete endpoint → connector error/orphan
  policy in product SRS).
- Rejects “everything is a free Bézier” for handwriting (costly, lossy for RM samples).
- Opens follow-up: local transforms on groups, clip-on-frame, z-order explicit vs tree order.

## Alternatives Considered

| Approach | Fidelity | Sync simplicity | Edit power | Notes |
|---|---|---|---|---|
| **Tree composite + polyline ink** | + | 0 | + | **Chosen** |
| Flat list of strokes only | − structure | + | − | Rejected — cannot express groups/frames/connectors |
| Scene graph with free local transforms everywhere (full Figma) | + | − | ++ | Deferred — overkill for v0 sync + RM ink |
| Bézier-only ink | − RM sample fidelity | 0 | 0 | Rejected for handwriting; OK later for connectors/primitives |
| Separate “layer stack” parallel to objects | 0 | − dual models | 0 | Rejected — one tree; z = tree order unless product adds z op |
