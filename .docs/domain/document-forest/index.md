---
title: Document forest
lifecycle: active
owner: architect
source: ADR-0041
---

# Document forest



The in-memory document of the rebuilt Epaper (`epaper/`), **accepted** 2026-10-04.
Decision, rejected alternatives and verification status:
[ADR-0041](../../adr/ADR-0041-document-forest.md) (Document forest, concurrent readers,
progressive paint). Implementation plan:
[document-forest-implementation.md](../../../.plan/document-forest-implementation.md).
Nothing here is implemented yet.

Scope:
- **Infini, the sync wire, ops and undo** stay on
  [vector-document.md](../vector-document.md). `epaper/` converts between the forest
  and that wire at the sync boundary.
- **The archived `epaper_old/`** keeps the old tree.
- `InkBox` here and `SmartGroup` there belong to different models. They are not two
  names for one live schema.

## Reading path

This file defines the entities and how they relate. Each algorithm has a short
overview here and its own file showing how it applies to this document.


| File                                   | Owns                                                                          |
| -------------------------------------- | ----------------------------------------------------------------------------- |
| this file                              | Geometry types, node header, kinds, payloads, invariants                      |
| [concurrency.md](./concurrency.md)     | One writer thread and concurrent reader threads on one live document          |
| [handles.md](./handles.md)             | `Handle = (slot, generation)`: what it is and every place it is used          |
| [spatial-index.md](./spatial-index.md) | The per-container R-tree, its maintenance, and every query it answers         |
| [rendering.md](./rendering.md)         | Viewport and size pruning, budgeted progressive paint, padded tiles           |
| [connector.md](./connector.md)         | Connector payload, mapped from the current fields, with the current algorithm |




## Pressure

Three requirements shape every choice below.

1. **Paint at every zoom.** A small zoom should stroke only what is on screen. At an
  extreme zoom-out, almost everything is on screen, so painting must also stop at
   clusters that are only a speck and draw a gray dot. In a dense document both
   prunes together can still leave too much work, so painting must also be
   budgeted and progressive.
2. **Concurrent read and write.** A reader thread renders or computes while another
  thread edits the same document (ink commit, manipulation, erase, undo, sync).
   The document is never cloned so that a reader has its own copy.
3. **One structure.** The spatial index is part of the document forest, not a second
  tree kept in sync beside it.



## Geometry




| Type                   | Meaning                                                                                                            |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `Pt`                   | Point in some node’s local space. The space is the field’s owner.                                                  |
| `Aabb`                 | Axis-aligned box `minX, minY, maxX, maxY` in the space named by the field. Empty when `max ≤ min` on either axis.  |
| `WorldPt`, `WorldAabb` | `Pt` and `Aabb` in world space, which is the `Document` node’s local space. X right, Y down.                       |
| `PanelPt`              | Pixel in the panel image, origin top-left, X right, Y down.                                                        |
| `FrameUv`              | Normalized `u, v ∈ [0, 1]` inside the sync frame after device orientation. Same role as today’s panel ↔ frame map. |


`PanelPt ↔ FrameUv ↔ WorldPt` is the existing map through the drawing region and
orientation. Axis swap and flip still map a panel-aligned rectangle to a
`WorldAabb`, so a tile or strip of pixels is always a world box the forest can query.



### Transform

Every node has a `Transform` from its local space into its parent’s child space
([Children](#child-space)): scale `(sx, sy)`, then rotation, then translation,
applied to a column vector. Negative scale mirrors.

`transform` **is reserved.** Manipulation never writes it
([Manipulation](#manipulation)). Every node this model creates has the identity
transform. Visits still compose it, and every rule below is written for a general
transform. A later feature can then use it without changing any visitor; it would
also need its own decision.

World placement is the product of the ancestor chain: each ancestor’s transform and
child origin. A visit composes it while it descends; the node does not store a world
matrix. Samples and shape points are never rewritten because an ancestor moved.



### InkSample

Local `x, y`, plus optional pressure, tilt and time. Missing channels stay absent.

## Entities and relationships



```text
NodeStore                     owns slots, id map, epochs, dependents (not a node)
  └─ root ─▶ Document         the single root node
               └─ children ─▶ Frame | Group | InkBox | Ink | Primitive | Connector
Frame, Group, InkBox          containers: children + R-tree of those children
Ink, Primitive, Connector     leaves: no children
Connector ─ source/target ─▶ NodeRef     reference, not parent/child
Connector ─ labels ─▶ NodeRef            reference, not parent/child
```

A document is a forest of root nodes. That forest is represented as **one node of
kind** `Document` whose children are the roots. Every visitor starts from a node,
including a visit of the whole document, so the document needs no separate visitor
interface.

What the `Document` node does not hold, because no node holds it, belongs to the
`NodeStore`: the slot table, the id → handle map, the reclamation epochs, and the
connector dependents index. There is exactly one `NodeStore` per `Document`.
[handles.md](./handles.md) and [concurrency.md](./concurrency.md) define it.

## Node



### Shared attributes

Every node of every kind has exactly these:


| Attribute   | Meaning                                                                                                                             |
| ----------- | ----------------------------------------------------------------------------------------------------------------------------------- |
| `id`        | Stable document identity. Used by sync, undo, connectors and persistence. Never reused inside a document.                           |
| `type`      | `NodeType`                                                                                                                          |
| `parent`    | Parent handle. Null only for `Document`.                                                                                            |
| `transform` | Local → parent                                                                                                                      |
| `bounds`    | The node’s own box: an `Aabb` in its **local space**. Knobs and move/resize act on it. Not the paint extent. See [Bounds](#bounds). |
| `children`  | The node’s child list. Empty for leaf kinds.                                                                                        |
| `version`   | Change counter used by readers. See [concurrency.md](./concurrency.md#header-fields).                                               |


Everything else is the payload for that kind. The header never contains samples,
shape parameters, connector ends, box policy or style.



### Children

`children` is an ordered list of child links. List order is paint order: a later
child paints above an earlier one. There is no z-index field.

```text
Children
  links: [ChildLink]        ordered; this is the list
  index: RTree               spatial index of the same children (spatial-index.md)

ChildLink
  node: Handle
  orderKey: u64              strictly increasing along links
  placement: Placement       what the container adds between itself and the child
```

The list holds **handles**, not node values. A node does not live inside its
parent’s array. If it did, growing the array would move the node and invalidate any
reader holding it, and a node could not be addressed from outside its parent.
How `Children` relates to slots and handles, and how a child is linked and
unlinked: [handles.md](./handles.md#children).

`orderKey` lets the R-tree return children in any spatial order and still paint
them in list order, by sorting the candidates by key. A new key is the midpoint of
its neighbours. Appending at the top uses `last + 2³²`. If two neighbours have no
gap left, that one container renumbers its keys.



### Child space

Children are stored in their container’s *child space*: the container’s local space
shifted to its **child origin**. Children are relative to that origin, so moving the
container changes only the origin, not the children.

| Container | Child origin |
|---|---|
| `Document` | none: child space is world |
| `InkBox` | `bounds.min`, its top-left corner (stored `bounds`) |
| `Frame` | `bounds.min`, its top-left corner (stored `bounds`) |
| `Group` | `origin`, a stored payload field, because a group’s `bounds` is derived |

A child origin is a translation only. It must never gain scale or rotation; that
would be the reserved `transform` under another name. A container’s R-tree is in its
child space.

`Placement` is a small variant. The container chooses which one:


| Container                                  | Placement           | Effect                                                                                    |
| ------------------------------------------ | ------------------- | ----------------------------------------------------------------------------------------- |
| `Document`, `Group`, `Frame`               | `Plain`             | none                                                                                      |
| `InkBox` boundary ink                      | `Boundary`          | none, not clipped                                                                         |
| `InkBox` content                           | `Content`           | clipped to the box, `[0, w] × [0, h]` in child space                                      |
| `Document`, for a node hung on a connector | `Follow { offset }` | translation onto the connector, set by the writer ([connector.md](./connector.md#labels)) |


A leaf’s `children` is the empty list. No list object is allocated for it. Adding a
child to a leaf is rejected.



### Bounds

A node has two boxes. They answer different questions, and only one is a field.

- `bounds`, in the header: the node’s own box, in its local space. It is what the
user sees as the node’s extent. Selection knobs sit on it, move and resize act on
it, and connector side anchors lie on its sides. It is not padded by stroke width.
- **Paint extent**: everything the node puts on screen, including stroke padding and
anything that sticks out of `bounds`. Culling and the size prune need it. It is
not a node field; it is the box of the node’s entry in its parent’s R-tree.


| Kind        | `bounds`                                                                         | Stored or derived | Paint extent adds                                              |
| ----------- | -------------------------------------------------------------------------------- | ----------------- | -------------------------------------------------------------- |
| `Ink`       | Hull of the samples                                                              | Derived           | Half the stroke width                                          |
| `Primitive` | Hull of the shape                                                                | Derived           | Half the stroke width                                          |
| `Connector` | Hull of `route.body` and `route.markers` ([connector.md](./connector.md#bounds)) | Derived           | Half the stroke width                                          |
| `Group`     | Union of the children’s `bounds`, shifted by `origin`                            | Derived           | The children’s paint extents instead of their `bounds`         |
| `Document`  | As `Group`, in world                                                             | Derived           | As `Group`                                                     |
| `Frame`     | The frame rectangle; it clips its children to it                                 | **Stored**        | Nothing: children are clipped                                  |
| `InkBox`    | The box rectangle `{x, y, w, h}`                                                 | **Stored**        | Boundary ink that sticks out past `bounds`; content is clipped |


Where it is stored, `bounds` is authoritative: the user set it, and only manipulation
changes it. Where it is derived, the writer recomputes it whenever the geometry
changes. Readers never compute it.

The parent’s R-tree entry box for a child is the child’s paint extent, mapped
through the child’s transform and placement (the hull of the four mapped corners).
That box is in the parent’s child space. For a leaf, the paint extent is `bounds`
padded as in the table. For a container, it is computed from its own R-tree: the
union of its entries, shifted by its child origin, with clipped entries cut to
`bounds`. The writer computes it in the same commit that changes the node.

Local boxes keep a move cheap. Moving an ink box changes its `bounds.min`, which
changes one entry in its parent’s R-tree. The box’s own R-tree is in child space, so
nothing inside it changes. If boxes were stored in world space, the same move would
rewrite the box of every descendant.

A world box is never stored. A visit computes it from the composed transform when
it needs one.



### NodeType


| Kind        | Children                                       | Parent may be                                       |
| ----------- | ---------------------------------------------- | --------------------------------------------------- |
| `Document`  | yes                                            | none — it is the root                               |
| `Frame`     | yes                                            | `Document` only                                     |
| `Group`     | yes                                            | `Document`, `Group`, `Frame`                        |
| `InkBox`    | yes: `Ink` boundary, `Ink` or `InkBox` content | `Document`, `Group`, `Frame`, `InkBox` (as content) |
| `Ink`       | no                                             | any container                                       |
| `Primitive` | no                                             | `Document`, `Group`, `Frame`                        |
| `Connector` | no                                             | `Document`                                          |


Frames stay root-only, carried from [ADR-0010](../../adr/ADR-0010-tree-of-vectors.md)
(Tree-of-vectors document model). Connectors are `Document` children because their
rest spine and their offsets are in world units.

## Payloads



A payload belongs to one kind. Its representation is a variant or separate per-kind
storage. One struct carrying every kind’s members is not allowed, even if unused
members are left at defaults.



### Document

No payload beyond `children`. Its transform is identity and cannot be set: its local
space is world space. It cannot be removed, moved or reparented. The viewport is not
the document’s transform.



### Ink


| Field     | Meaning                                 |
| --------- | --------------------------------------- |
| `samples` | `[InkSample]` in this ink’s local space |
| `stroke`  | Color and width, in world units         |


One stroke is one polyline. Edge `i` joins sample `i` to sample `i + 1`. Edges are
that pairing; they are not stored. Committing an ink with zero samples is rejected.
Erase that cuts a stroke produces several `Ink` nodes, as it does today.



### InkBox

The ink box. This model does not call it a SmartGroup.


| Field             | Meaning                                                                                                                                                                                                                                 |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `bounds` (header) | Stored. Where the box is and how big it is, in the box’s local space. With the reserved identity transform, that is the **parent’s child space**. Move and the 8 knobs change it. Below, `x, y` is `bounds.min` and `w, h` is its size. |
| `boundaryPolygon` | The closed polygon of the enclose stroke, in the box’s child space. Invisible, never erased. Membership and object erase use its area and interior.                                                                                     |
| `manipMode`       | `All` or `Boundary`. Default `Boundary`. See [Manipulation](#inkbox-manipulation).                                                                                                                                                     |
| `boundaryInks`    | The links whose placement is `Boundary`: many `Ink`s                                                                                                                                                                                    |
| `contents`        | The links whose placement is `Content`: many `Ink`s and nested `InkBox`es                                                                                                                                                               |


An ink box has no rectangle field of its own; its rectangle is its `bounds`. Boundary
ink that sticks out past it widens only the paint extent, so erasing boundary ink
never moves a knob or an anchor.

`bounds` is relative to the parent. The box’s children (boundary inks, contents,
`boundaryPolygon`) are relative to the box’s top-left corner, so the box itself
covers `[0, w] × [0, h]` in its child space.

`boundaryInks` and `contents` are the two halves of one `children` list. Boundary
links come first and paint first. Content paints above them, clipped to the box.
There is one R-tree over both halves. There are not two copies of either list.

Boundary inks are many because erase can cut the enclose stroke into pieces. A box
whose boundary ink was all erased still has `boundaryPolygon`, so it still has an
interior.

`boundaryInks` may hold only `Ink`. Draw-into, enclose and paste keep their current
rules for which nodes become content. Nested ink boxes are content, as
[ADR-0039](../../adr/ADR-0039-nested-ink-box-rendering.md) (Nested ink-box
RenderingContext and own-transform) already allows.

How the box responds to move and to the 8 knobs is in
[InkBox manipulation](#inkbox-manipulation).



### Primitive

One shape, in the primitive’s local space, plus `stroke` and an optional fill. With
the reserved identity transform, that is the parent’s child space. Shapes are not
rotated: a rotated rectangle, square or ellipse is stored as a `Polygon` until a
later decision uses `transform`.


| Shape     | Stores                |
| --------- | --------------------- |
| Segment   | two points            |
| Triangle  | three points          |
| Square    | center and side       |
| Rectangle | origin, width, height |
| Circle    | center and radius     |
| Ellipse   | center, `rx`, `ry`    |
| Polygon   | closed list of points |




### Group

| Field | Meaning |
|---|---|
| `origin` | The group’s child origin, in its local space (the parent’s child space). Set to the group’s `bounds.min` at creation. After that, only move and resize change it. |

Its children are stored relative to `origin`. Adding or removing a child changes the
derived `bounds` but not `origin`, so no other child is rewritten. A group does not
clip and has no stroke of its own. It cannot hold a `Frame`. Committing a group with
no children is rejected.



### Frame

No payload beyond `children`. Its stored `bounds` is the frame rectangle, in its local
space. It clips its children to `bounds`. Children are stored relative to its
top-left corner, so it behaves like an artboard: its content moves with it.



### Connector

A leaf whose two ends reference ink boxes. It is not their parent. Its payload is
`source` and `target` ends, `style` (`Curve` or `Ink`), a fixed rest path, `stroke`,
end markers and labels. Its world stroke is derived from those fields with the
current warp algorithm. Full payload and the mapping from today’s fields:
[connector.md](./connector.md).

## Manipulation



Each kind decides how it responds to manipulation. Today only the ink box can be
moved or resized (`descriptorFor` in `epaper_old/document/capability.hpp` gives every
other kind select-only), so every rule here except the ink box’s is new behavior.

### Manipulation contract

Every kind answers two calls:

```text
move(Δ)          Δ: offset in the parent’s child space
resize(m)        m: ResizeMap = { sx, sy, tx, ty },  p ↦ (p.x · sx + tx, p.y · sy + ty),  sx, sy > 0
                 in the node’s local space (the parent’s child space)
```

- **A knob drag** builds `m` from the old `bounds` to the new one. The new rectangle
  keeps the corner or side opposite the knob fixed: `sx = w'/w`, `tx = x' − x · sx`,
  and the same on `y`.
- **A container passes the map down** as a pure scale. If the container’s child
  origin is `o`, a child point `q` sits at `o + q`, and
  `m(o + q) = m(o) + (q.x · sx, q.y · sy)`. So the container sets its origin to `m(o)`
  and calls `child.resize({sx, sy, 0, 0})` on each child that its kind scales.
- **No response writes `transform`.** A response edits geometry, the stored
  `bounds`, or the child origin.
- **The live gesture is a tool-overlay preview.** The response runs once, on the
  writer, at commit.

Rules shared by every kind:

- Stroke width, pressure, tilt and time never change.
- Knobs cannot flip a node. Each kind has a minimum size. For `InkBox` it is the
  minimum enclose size. For the others it is the provisional 1 world unit that
  `resizeWorldAabbFromHandle` uses today.
- Move and knobs are unavailable while the node’s smaller on-panel axis is under
  `kLodMinAxisDu = 96`, as today
  ([SRS-EP-11](../../modules/epaper/features/ink-box/srs-logic.md#srs-ep-11-device-manipulation),
  Selection, hit-testing, and manipulation).
- Reparenting is a move ([Reparenting](#reparenting)).

| Kind | move | resize | Details |
|---|---|---|---|
| `Ink` | shift samples | map samples, free non-uniform | [Ink](#ink-manipulation) |
| `Primitive` | shift the shape | per shape; `Square` and `Circle` always keep their aspect | [Primitive](#primitive-manipulation) |
| `Group` | `origin += Δ` | `origin ← m(origin)`; every child scales | [Group](#group-manipulation) |
| `Frame` | `bounds.min += Δ` | `bounds ← m(bounds)`; children not edited | [Frame](#frame-manipulation) |
| `InkBox` | `bounds.min += Δ` | `bounds ← m(bounds)`; children per `manipMode` | [InkBox](#inkbox-manipulation) |
| `Connector` | only when both ends are detached | never | [Connector](#connector-manipulation) |

### Ink manipulation

- **move:** add `Δ` to every sample.
- **resize:** apply `m` to every sample. Scaling is free and non-uniform, the same as
  an ink inside an `All`-mode box.
- The derived `bounds` follows.

A nearly straight stroke has almost no extent on one axis. For a **direct** knob drag
on such an ink, that axis’s knobs are hidden; otherwise `w'/w` with `w ≈ 0` would blow
the stroke up. A map that comes from a parent is applied as is, because its scale is
finite. The threshold is provisional: the 1 world unit minimum size.



### Primitive manipulation

- **move:** shift the stored points, origin or centre by `Δ`.
- **resize** depends on the shape:

| Shape | resize by `m` |
|---|---|
| Segment, Triangle, Polygon | apply `m` to each point |
| Rectangle | `origin ← m(origin)`; `width · sx`, `height · sy` |
| Ellipse | `center ← m(center)`; `rx · sx`, `ry · sy` |
| Square, Circle | always keep the aspect: one uniform scale `s` (below) |

`Square` and `Circle` never stretch into another shape, and never change kind:

- **Direct knob drag.** A corner knob uses `s = max(sx, sy)`, anchored at the opposite
  corner. A side knob uses that axis’s scale, anchored at the midpoint of the opposite
  side, so the shape grows evenly on the other axis. The preview shows the
  constrained rectangle, not the raw knob rectangle.
- **Map from a parent** (a group resize or a multi-selection). `center ← m(center)`,
  and `s = min(sx, sy)`, so the shape stays inside its stretched box. In a group made
  wider, a circle stays round and moves with the group. *Provisional:* `min` rather
  than `max` or the geometric mean.

### Group manipulation

- **move:** `origin += Δ`. Children are relative to `origin`, so nothing else is
  written. The group’s entry in its parent’s R-tree is the only index change.
- **resize:** `origin ← m(origin)`, then `child.resize({sx, sy, 0, 0})` on every child.
  A nested `InkBox` then applies its own `manipMode`, and a `Square` or `Circle` keeps
  its aspect. A group always scales its children; it has no `manipMode`.
- A knob drag builds `m` from the group’s derived `bounds`. If an aspect-locked child
  scales by less than the group, the recomputed `bounds` can come out a little
  smaller than the knob rectangle. That is expected.

### Frame manipulation

A frame is an artboard.

- **move:** `bounds.min += Δ`. Children ride along because they are relative to the
  top-left corner.
- **resize:** `bounds ← m(bounds)`. Only the rectangle and its clip change. Children
  are not edited: a top or left knob carries them with the corner, and content outside
  a shrunk frame is clipped, not moved. This is the same as `Boundary` mode on an ink
  box. A frame has no `manipMode`.

### InkBox manipulation

The ink box applies `m` to its `bounds`, so the corner or side opposite a knob stays
where it was. Write the old rectangle as `(x, y, w, h)` and the new one as
`(x', y', w', h')`. The knobs keep `w', h'` at or above the minimum enclose size. In
the box’s child space, the map is the pure scale `p ↦ (p.x · sx, p.y · sy)`.


| `manipMode` | move              | resize                                                                                                                                                                                                                                                                     |
| ----------- | ----------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `All`       | `bounds.min += Δ` | `bounds ← (x', y', w', h')`. Apply the resize map to `boundaryPolygon` and to every boundary ink’s samples. Each content child **resizes** by the same map: an `Ink` maps its samples; a nested `InkBox` maps its `bounds` and then responds with its **own** `manipMode`. |
| `Boundary`  | `bounds.min += Δ` | `bounds ← (x', y', w', h')`. Apply the resize map to `boundaryPolygon` and boundary inks. Contents are not edited. They keep their coordinates relative to the box, so a top or left knob carries them with the top-left corner.                                           |


Move edits nothing but `bounds`: children are relative to the box, so they follow.
A resize edits only coordinates in the box’s child space. Stroke widths do not change.

In `Boundary` mode, content that falls outside a shrunk box is clipped, not moved.

**Why this replaces the scale mode.** There is no scale in any transform, so the
`fixedInk` placement, `S⁻¹` and the per-ink layout UV are all gone. A visitor
composes plain translations, with no special case. Resize becomes a geometry edit,
like an erase, which the writer already does. The cost is that an `All` resize
rewrites the samples of its subtree. That happens once per commit, not per frame.



### Connector manipulation

A connector’s shape is derived from its two ends, so it is mostly select-only, as
today.

| Ends | move | resize |
|---|---|---|
| Both live | Not available. It follows its end boxes: moving or resizing either box re-derives it ([connector.md](./connector.md#who-computes-what-and-when)). | Never |
| Both detached, each with a `lastPose` | Add `Δ` to both `lastPose` points; the writer re-derives `route`. Both ends shift equally, so the route shifts exactly by `Δ`. | Never |
| One live, one detached | Not available | Never |

Dragging an end onto another box is **re-anchoring**: an edit of the connector’s
ends, not manipulation. Nodes hung on the connector follow it through their
`Follow` placement, which the writer re-derives with the route.

### Moving or resizing several nodes

A multi-selection behaves like a temporary group:

- The knobs sit on the union of the selected nodes’ world `bounds`.
- If a node and one of its ancestors are both selected, only the ancestor is
  manipulated.
- **move:** each selected node gets `move(Δ)`, with `Δ` mapped into its parent’s child
  space. Today that is the same `Δ`.
- **resize:** the knob gives one world map `m`. Each selected node gets `m` mapped
  into its parent’s child space: `inverse(W) ∘ m ∘ W`, where `W` maps that child space
  to world. Today `W` is a translation, so the mapped `m` keeps `sx, sy` and only
  `tx, ty` change.
- A connector whose ends are live is skipped. If its end boxes are in the selection,
  they move it. Aspect-locked shapes use the parent-map rule `s = min(sx, sy)`.



### Reparenting

Moving a node into a different container (move-commit into a box, draw-into, paste,
enclose capture) must not change where or how large it looks:

```text
reparent(node, newParent):
  M = inverse(world(newParent child space)) ∘ world(oldParent child space)
  if M is a pure translation Δ:   node.move(Δ)               today’s only case
  else:                           node.transform = M ∘ node.transform    once transforms are in use
  unlink from oldParent; insert into newParent; update both R-trees
```

`world(C child space)` is the composition, root to `C`, of each container’s
transform and child origin ([Child space](#child-space)). Today every transform is
identity. `M` is then the difference between the two containers’ accumulated child
origins, and reparenting is a plain move. A node moved into a box never scales and never lands somewhere else.
The `else` branch is the rule for later, when a parent may have a non-identity
transform. It is specified now so that using `transform` later cannot bring back the
current flying and scaling.

## Invariants



Checked by the writer on every commit:

- Exactly one `Document`. It is the store’s root.
- Each non-`Document` node has exactly one parent, and the parent’s list holds it once.
- A leaf has no children. Parent kinds follow [NodeType](#nodetype).
- In every list, `orderKey` strictly increases. In an `InkBox`, all `Boundary` links
come before all `Content` links.
- Each container’s R-tree holds exactly its `children`, and each child’s entry box is
that child’s mapped paint extent.
- A derived `bounds` equals its formula in [Bounds](#bounds). A stored `bounds`
changes only through manipulation.
- Ids are unique. An id is never reused.
- Every `transform` is identity until a feature that uses it is decided.
- An `InkBox` has `w, h` at or above the minimum enclose size.
- A `Square` is still square and a `Circle` still round after any resize.



## Algorithms — overview



**Concurrency.** One writer thread performs every mutation. Any number of reader
threads read the same live nodes at the same time. A reader never sees half of a
write, and memory is never freed while a reader may still hold it. Small fields are
written in place under a per-node version counter. Variable-size data is built
beside the old copy and swapped in with one pointer store. After every commit the
writer posts damage, which is how a reader’s view converges.
[concurrency.md](./concurrency.md).

**Handles.** Nodes live in a slot table that never moves. A handle is a slot plus
that slot’s generation. A handle held across frames detects that its node was
removed, even if the slot now holds a different node.
[handles.md](./handles.md).

**R-tree.** Each container indexes its direct children in its child space. Each
cell carries the box and the total paint cost of everything under it. Culling,
hit-testing, lasso, membership, erase candidates, snapping and budgeting are all
queries over these trees. [spatial-index.md](./spatial-index.md).

**Rendering.** One descent applies both prunes. A cell outside the query region is
skipped (prune A). A cell too small on screen becomes one gray dot (prune B). Both
are safe because a cell’s box contains everything under it. On top of that, the
padded panel image is split into tiles. The renderer completes tiles from a
prioritized queue in time-bounded slices, so a dense document never freezes the
screen. [rendering.md](./rendering.md).

## What this model does not restate

The forest must be able to host these. Their rules stay in the current docs until a
supersession moves them on purpose.


| Topic                               | Where it lives now                                                                                                            |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| Ops, `lastOpId`, undo               | [vector-document.md](../vector-document.md) · [ADR-0032](../../adr/ADR-0032-inverse-op-undo.md) (Inverse-op undo per session) |
| Enclose, draw-into, 80% tests       | [vector-document.md](../vector-document.md)                                                                                   |
| Connector recognition and warp math | [ADR-0020](../../adr/ADR-0020-connector-ink-geometry.md) (Connector-ink geometry: rest shape, cubic and morph warps)          |
| Erase, clipboard, selection, follow | module SRS; not document nodes                                                                                                |
| `Text`                              | a current kind; **not a kind here**. A payload can be added later without changing the header.                                |




## Provisional choices

Adopted with the design on 2026-10-04. They are held as starting values: a
measurement or a later decision may revise them without reopening the model.



| Choice                                           | Held as                                                                                                |
| ------------------------------------------------ | ------------------------------------------------------------------------------------------------------ |
| Frames root-only; connectors `Document` children | Carried from current behavior                                                                          |
| No `Text` kind                                   | Not in the requested kind list                                                                         |
| No rotation; resize does not scale stroke width  | Rotation is out of scope today too ([ink-box SRS](../../modules/epaper/features/ink-box/srs-logic.md)) |
| Aspect-locked shape under a parent map           | Scales by `min(sx, sy)`, so it stays inside its stretched box ([Primitive](#primitive-manipulation))   |
| Minimum size, and the flat-ink knob threshold    | 1 world unit, as `resizeWorldAabbFromHandle` uses today                                                |
| Connector with one end detached                  | Not movable: no clear rule for where the live end goes                                                 |
| Boundary inks paint below content                | Visible only where they overlap                                                                        |
| Dot threshold and tile/slice budgets             | Untuned starting values in [rendering.md](./rendering.md)                                              |




## Decided in review




| Decision                                      | Choice                                                                                                                                                                                                          |
| --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Boundary` mode content on a top or left knob | Content keeps its coordinates relative to the box and moves with the top-left corner (human, 2026-10-04). It is not held still on the page, and it is not re-centred at a UV position as `fixedInk` does today. |
| Connector centre-end facing                   | The ray toward the other end, as shipped ([connector.md](./connector.md#adr-0020-drift)) (human, 2026-10-04)                                                                                                    |
| `Ink` resize | Free, non-uniform (human, 2026-10-04) |
| `Square`, `Circle` resize | Always keep the aspect, even under a group or multi-selection resize; never change kind (human, 2026-10-04) |
| `Group` move | A stored `origin`, so a move writes one field (human, 2026-10-04) |
| `Frame` | Artboard: children relative to its corner; resize changes only the rectangle and clip (human, 2026-10-04) |
| `Connector` | Select-only while attached; movable only when both ends are detached (human, 2026-10-04) |




## Verification


| Claim                                                         | Status                                                                                               |
| ------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| Model is specified across these six files                     | Written; **accepted** by the human 2026-10-04                                                        |
| Human check of the provisional choices                        | Done: adopted with the design 2026-10-04                                                             |
| Reparenting into a box keeps world position and size          | **Pending** test: move a box into a nested box, then compare world sample positions before and after |
| `All` resize of nested boxes gives each nested box’s own mode | **Pending** test                                                                                     |
| Group resize: children scale, circles stay round; group move writes only `origin` | **Pending** test |
| Moving a detached connector shifts its route exactly by `Δ` | **Pending** test |
| Connector mapping reproduces today’s warp output on fixtures  | **Pending** ([connector.md](./connector.md#verification))                                            |
| Implementation in `epaper/`                                   | **Pending**; not started. Plan: [document-forest-implementation.md](../../../.plan/document-forest-implementation.md) |


