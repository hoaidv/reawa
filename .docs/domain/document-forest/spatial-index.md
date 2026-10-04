---
title: Document forest — spatial index (R-tree)
lifecycle: active
owner: architect
source: ADR-0041
---

# Spatial index: an R-tree per container

<a id="spatial-index"></a>

Part of the [document forest](./index.md). Decision: [ADR-0041](../../adr/ADR-0041-document-forest.md).

## Overview

An **R-tree** is a balanced tree of boxes. Each leaf cell holds up to `M` entries.
An entry is one item and its box. Each internal cell holds up to `M` child cells,
with a box around each one. A cell’s box contains every box beneath it. Boxes may
overlap. A query starts at the root and enters only the cells whose box passes the
query’s test. Everything under a rejected cell is skipped without being read.

```text
               [root: box ⊇ all]
              /        |        \
        [cell A]    [cell B]    [cell C]        internal cells: box, cost, count
        /  |  \       |  \        |  \
      e1  e2  e3     e4  e5      e6  e7         entries: child handle, box, orderKey, cost
```

Two properties matter here. Overlapping boxes are normal: a nested ink box sits
inside its parent’s box, and a connector spans two boxes. Unlike a quadtree, an
R-tree handles overlap without pushing items up toward the root. Also, every cell
summarizes everything below it, which is what lets one descent apply several prunes.

## Where the trees live

<a id="placement"></a>

Every container (`Document`, `Frame`, `Group`, `InkBox`) owns one R-tree, inside its
`Children` ([index.md](./index.md#children)). That tree indexes the container’s
**direct** children, in the **container’s child space** ([index.md](./index.md#child-space)).
For an `InkBox` that space is relative to the box’s top-left corner.

```text
Entry  = { node: Handle, box: Aabb (child space), orderKey: u64, cost: u32, kind bits }
Cell   = { box: Aabb, cost: u32 (sum below), count: u32 (entries below), slots: up to M }
```

- `entry.box` is the child’s **paint extent**, not its `bounds`, mapped through its
  transform and placement: the hull of the four mapped corners. The paint extent
  covers stroke padding and boundary ink that sticks out of `bounds`
  ([index.md](./index.md#bounds)). Paint and hit-test prefiltering stay correct
  because they never trust a box smaller than what is drawn.
- `entry.cost` is the child’s paint cost: its own for a leaf, its R-tree root `cost`
  for a container. See [rendering.md](./rendering.md#cost).
- The kind bits say whether the entry is a container, and anything else a query
  filters on: pickable, legal parent, the boundary or content half of an ink box.

The document’s overall spatial index is therefore a tree of trees. A visit descends
the `Document`’s R-tree. When it reaches an entry that is a container, it continues
in that container’s R-tree. These trees are part of the forest. They are not a second
structure beside it.

**Why per container, in child space.** Moving an ink box with a thousand inks
changes one entry in its parent’s tree, because everything inside the box is
relative to the box. With one global tree of world boxes, the same move rewrites a
thousand entries. A global tree would also be a second structure that has to stay
coherent with the forest under concurrent readers.

**Why not a quadtree.** A quadtree stores each item in the smallest square that holds
it. A box that straddles a split line is pushed up, so overlapping ink boxes
accumulate near the root. Then the size prune cannot stop early, and a point query
scans the root’s list. [ADR-0040](../../adr/ADR-0040-logarithmic-hit-test.md)
(Device logarithmic hit-test spatial index) rejected a device quadtree for the same
reason.

## Parameters

| Parameter | Value | Note |
|---|---|---|
| `M` (max entries per cell) | 16 | One cell fits in a few cache lines |
| `m` (min entries per cell) | 6 | About 40% of `M`. Applies at every level except the root. |
| Root cell | Leaf root: 0 to `M` entries. Inner root: 2 to `M` children. | A container with 3 children has one leaf root of 3. The tree loses a level when the root is left with one child. |
| Choose subtree, split | R*-tree rules | Least overlap enlargement at leaf level; R* split |
| Bulk load (`doc_load`, paste of a large subtree) | STR (Sort-Tile-Recursive) | Packs cells full and close together |
| Small containers (≤ `M` children) | one leaf cell | Effectively a flat list; no tree overhead |

## Maintenance

<a id="maintenance"></a>

All updates are done by the writer, by path copy
([concurrency.md](./concurrency.md#granules), Rule 4).

| Change | R-tree work |
|---|---|
| Insert child | R* insert into the parent’s tree. Copy the root-to-leaf path. Split if full. |
| Remove child | Delete the entry. Copy the path. Reinsert the entries of any cell that drops below `m`. |
| Child’s mapped box changes, and it still fits inside its leaf cell’s box | Replace the entry. Copy the path, and shrink boxes on the way up if possible. |
| Child’s mapped box changes, and it no longer fits | Delete and reinsert |
| Reorder (bring to front) | New `orderKey` on that entry only. The box does not change. |
| Child’s cost changes | Replace the entry. Cell costs on the copied path are re-summed. |

After a change, the container’s paint extent (its root box shifted by its child
origin, clipped where its kind clips), its derived `bounds` and its `cost` may have
changed. If so, the writer updates the container’s entry in **its** parent’s tree,
and so on up the chain. The chain stops at the first container whose paint extent,
`bounds` and cost did not change. A stored `bounds` (`InkBox`, `Frame`) never changes
because a child changed. A typical ink commit at the root
touches one path in the `Document` tree and nothing else.

## Queries

<a id="queries"></a>

Every query is one traversal with a **decision function** applied to each cell and
each entry:

```text
decide(box, ctx) → Skip | Enter | Stop(proxy) | Accept
```

`ctx` carries the query shape and the composed transform from this container to
world and panel. When the traversal reaches an entry that is a container and the
query is meant to descend, it maps the query shape into the child’s child space (the
inverse of placement, transform and child origin, then the hull). While every
transform is identity, that map is a translation. It then continues in the child’s
tree with the composed transform.

```text
visit(container, query, ctx):
  stack = [container.children.index.root]
  while stack:
    cell = pop(stack)
    switch decide(cell.box, ctx):
      Skip:           continue
      Stop(proxy):    emit proxy; continue                    paint only
      Enter/Accept:   push the cell's child cells, or handle its entries:
        for each entry e:
          n = resolve(e.node); if Gone: continue              handles.md §1
          switch decide(e.box, ctx):
            Skip: continue
            Stop(proxy): emit proxy
            Accept:
              if n is a container and the query descends:
                  visit(n, mapQueryInto(n), compose(ctx, n))
              else:
                  emit candidate (n, e.orderKey)
```

The R-tree returns **candidates**: entries whose box passes the test. The exact
geometric test for each kind runs only on those `k` candidates. Cost is about
`O(log n + k)` per container visited.

### What each feature asks

<a id="use-cases"></a>

Yes: hit-testing, culling and the other geometric questions are all queries on these
same trees.

| Use | Query shape | Descends into containers | Exact test on candidates | Ordering |
|---|---|---|---|---|
| Paint a region | world rectangle (tile) | yes | none; draw | sort by `orderKey` per container |
| Tap hit-test | point | yes; deepest wins | kind-exact: on the stroke, inside an ink box’s `bounds`, … | later and deeper first |
| Marquee | rectangle | **no**: top level only | 80% of samples or of area, as today | — |
| Freeform lasso | polygon’s box, then the polygon | no | 80%, as today | — |
| Draw-into membership | new stroke’s box | yes, ink boxes only | 80% of the stroke’s length inside `boundaryPolygon` | highest paint wins |
| Enclose capture | fitted rectangle | no | 80%, as today | — |
| Move-commit reparent | moving node’s box | yes, excluding itself | 80% of its area | highest paint wins |
| Erase candidates | brush, area or lasso box | yes | erase’s own table | — |
| Connector end snap | point, nearest first | yes, ink boxes only | distance to the boundary | best-first by distance |
| Render job cost | tile rectangle | yes | none; sums `cost` | — |

Nearest-first search uses a priority queue of cells ordered by distance from the
query point. It stops when the next cell is farther away than the best answer so far.

### Both prunes in one traversal

<a id="prune-a-b"></a>

Yes. Paint uses one descent whose decision function contains both prunes:

```text
decidePaint(box, ctx):
  if box (in world) misses the region:        Skip                         prune A
  if longerSide(panel image of box) < τ:      Stop(dot at box centre)      prune B
  Accept
```

This works in one pass because both tests are **monotone down the tree**. A child’s
box lies inside its parent’s box. So:

- if a cell misses the region, everything under it misses too, and skipping is exact;
- if a cell is smaller on screen than `τ`, everything under it is smaller too, so one
  dot stands for all of it.

Both prunes therefore apply to internal cells as well as to nodes. That matters for
dense documents. A page of handwriting is thousands of strokes directly under
`Document`, so the forest itself has no hierarchy to stop on. The R-tree supplies
one: at extreme zoom-out, a cell holding two hundred small strokes is three pixels
wide. The descent stops there with one dot and reads none of the two hundred.

Prune B is for paint only. Hit-testing and the exact queries never use it: a speck
is still selectable by the rules of the gesture that asks.

### Paint order

The R-tree returns candidates in spatial order. Paint order is list order, so the
visit sorts each container’s accepted candidates by `orderKey` before drawing them.
It draws a container’s subtree in that container’s position in its parent’s order.
Dots are drawn first, below everything; they are a stand-in, not content.

## Sensitivities

- **Overlap.** Cost grows with how many cell boxes contain the query, not with `n`.
  Many boxes stacked over the same spot raise `k`. That is the realistic worst case.
- **Rotation.** None today, because `transform` is reserved. If it is used later, a
  rotated child’s entry is the hull of its rotated box, which is larger than the
  shape. Culling stays correct, just less tight.
- **Write cost.** One path copy is about `16 × log₁₆(n)` entries: about 64 entries at
  100k children.

## Relationship to ADR-0040

[ADR-0040](../../adr/ADR-0040-logarithmic-hit-test.md), now superseded by ADR-0041,
proposed one device R-tree of world boxes for the archived tree, rebuilt on every
commit. This file is the forest’s version of that idea: one tree per container, in
child space, maintained in place, and shared by paint and hit-testing. The exact 80% rules of ADR-0040 carry
over unchanged as the exact tests on candidates.

## Verification

| Claim | Status |
|---|---|
| Point query visits `O(log n + k)` cells on a 100k-stroke root | **Pending** benchmark |
| Moving an ink box updates exactly one entry above it | **Pending** test |
| Prune B never hides a cell that has a descendant larger than `τ` | Follows from cell containment; **pending** property test |
| Query results equal a brute-force walk | **Pending** property test (random forests, random queries) |
