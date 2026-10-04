---
title: Document forest — rendering
lifecycle: active
owner: architect
source: ADR-0041
---

# Rendering: prunes, budgeted progressive paint, padded tiles

<a id="rendering"></a>

Part of the [document forest](./index.md). Decision: [ADR-0041](../../adr/ADR-0041-document-forest.md).
Traversal mechanics: [spatial-index.md](./spatial-index.md). Thread rules:
[concurrency.md](./concurrency.md).

## Pipeline

```text
camera (+ pad) ─▶ padded tile set ─▶ job queue ─▶ slices on the render thread ─▶ present to panel
                                       ▲
writer commits ─ damage ───────────────┘
```

The document provides one operation: draw everything that intersects a world region,
clipped to it, in paint order, with the two prunes applied. Everything else here is
renderer policy: which regions to draw, when, and when to show them.

## Prune A and prune B

<a id="prunes"></a>

One descent, defined in [spatial-index.md](./spatial-index.md#prune-a-b):

- **A, outside the region:** a cell or node whose world box misses the region is
  skipped.
- **B, too small:** a cell or node whose image on the panel has a longer side under
  `τ` pixels becomes one gray dot at its centre, and nothing under it is read.

A covers small and modest zoom: the region is small, so most cells miss. B covers
extreme zoom-out: everything intersects the region, but whole R-tree cells are under
`τ`. Dots are drawn first, about one pixel each, in gray. They are paint output
only: never stored, never hit targets.

A and B are still not enough for a **dense document at a middle zoom**. Take a full
page of handwriting where each word is 6–20 pixels on screen. Everything is in the
region, so A does nothing. Most strokes are above `τ`, so B does little. The frame
still has to draw tens of thousands of strokes. That is the case the budget
mechanism below exists for.

## The budget proposal, examined

<a id="proposal-8"></a>

The proposal was: count nodes cumulatively per visit depth, from the root; pick the
deepest depth whose cumulative count fits what one rasterize can handle; render
everything collected down to that depth now; put the rest on a tail queue; work
through the queue afterward; clear the queue when a new whole-document render is
requested.

**What it gets right, and what is kept.** Rendering must be bounded per step, so the
screen never freezes. Show what is ready, then keep going. Deferred work goes on a
queue. A change of camera that makes everything obsolete cancels the deferred work.
All four are kept.

**What breaks, and why.**

1. **A depth cut does not divide the work.** In the forest, a dense page is
   thousands of strokes at depth 1, so no depth splits them. In an R-tree, every
   leaf is at the same depth, because the tree is balanced. A cut above the leaf
   level draws no strokes at all; a cut at the leaf level draws all of them. So
   "stop at depth d" draws either nothing or everything.
2. **One depth for the whole document wastes the budget.** Put a dense page in one
   corner and sparse notes everywhere else. The cut that fits the dense corner also
   holds back the sparse notes, which would have fit easily. The cut should follow
   density, region by region.
3. **Node count is not cost.** One 20 000-sample stroke costs more than a thousand
   five-sample dots. The budget has to count drawing work.
4. **Deferring subtrees paints out of order and leaves stand-ins behind.** A deferred
   subtree drawn later lands above strokes that should cover it. Whatever stood in
   for it in the first pass (a dot, a box) cannot be erased without erasing the
   strokes around it. R-tree cells overlap, so one cell’s area holds other cells’
   strokes. Drawing a subtree later can never make an area correct on its own.
5. **A queued tree position does not survive concurrent edits.** Between slices, the
   writer may have replaced the very cells and child lists a queued item points
   into. Those are reclaimed ([concurrency.md](./concurrency.md#reclamation)), and
   they are not nodes that a handle can validate.
6. **"Clear the queue" is right for zoom and wrong for pan and edit.** After a pan,
   most of the work is still valid. After an edit, only the damaged area is stale.
   An explicit clear sent from another thread also races with a job the render
   thread has already taken.

**The revision.** Keep the idea and change the unit of deferred work from a **piece
of the tree** to a **region of pixels**.

<a id="why-regions"></a>

A region job means: clear this rectangle, then draw everything that intersects it,
in order, clipped to it. That job is complete on its own. Paint order inside it is
correct. It replaces whatever stand-in was there, because it clears first. It holds
no tree position across slices, only a rectangle, which an edit cannot invalidate.
Pans and edits become "which rectangles": pan exposes some, an edit damages some,
and neither needs a clear.

The proposal’s cumulative count becomes a **cumulative cost along a best-first
frontier**. That is used for the first coarse picture, where its intent (show the
document quickly, then refine) is exactly right. The rest of this file specifies the
revised mechanism.

## Cost

<a id="cost"></a>

Every R-tree entry and cell carries `cost`, the total for everything below it
([spatial-index.md](./spatial-index.md#placement)). This is an *aggregate R-tree*.

| Node | Cost units |
|---|---|
| `Ink` | number of segments (samples − 1) |
| `Primitive` | 4 + vertices |
| `Connector` | placed body segments + marker segments |
| Container | sum of its children (its R-tree root cost) |

The writer keeps it current on the same path copy that updates boxes. The cost of a
region is a range query that adds up `cost`. Cells fully inside the region count
whole; only cells straddling its edge are entered.

Units become time through a measured rate, updated after each job:

```text
rate ← 0.8 · rate + 0.2 · (elapsedMs / costDrawn)
costBudget = sliceMs / rate
```

## Tiles

<a id="tiles"></a>

The renderer’s image is the viewport plus a pad on every side. It is divided into
square tiles of `T` panel pixels, on a grid anchored in world space at the current
scale. Panning moves the camera by whole pixels, so tiles always line up with the
grid.

| Tile state | Meaning |
|---|---|
| `Missing` | No usable pixels |
| `Stale` | Shows older pixels: shifted after a pan, scaled from the previous zoom, or from before an edit |
| `Coarse` | Drawn by the coarse pass: some strokes, some stand-in dots |
| `Complete` | Drawn by a full region job since the tile’s last damage |

Each tile has a `damageSeq`, which goes up every time damage touches it.

## Jobs and the queue

<a id="queue"></a>

```text
Job = { tile, cameraEpoch, damageSeqAtQueue }
```

The queue belongs to the render thread alone. Other threads do not touch it; they
post messages (camera change, damage, load) to the render thread’s mailbox. Priority,
highest first:

1. visible tiles touched by damage from the user’s own current gesture or commit;
2. other visible tiles, nearest first to the focus point (pen or gesture position,
   else the viewport centre);
3. pad tiles, nearest the viewport first.

**Cancellation is by epoch, not by clearing.** `cameraEpoch` goes up on zoom,
orientation change and document load. When a job is taken with an old epoch, it is
dropped. Pans do not change the epoch, because the tile grid stays valid at the same
scale; jobs for tiles that left the padded set are dropped when taken. Because
nothing is removed from the queue from outside, there is no race.

## The slice loop

<a id="slice-loop"></a>

```text
loop on the render thread:
  drain the mailbox; apply camera changes and damage to tile states; enqueue
  job = take the highest-priority job          wait on the mailbox if none
  if job.cameraEpoch != cameraEpoch or job.tile is outside the padded set: drop it; continue

  est = cost of job.tile’s world box           aggregate range query
  if est > costBudget and T(job.tile) > T_min:
      split into 4 sub-tile jobs; enqueue them; continue

  seq = job.tile.damageSeq
  enterRead(); clear the tile; draw its world box with prunes A and B; exitRead()
  if job.tile.damageSeq == seq: job.tile.state = Complete
  else: enqueue job.tile again                  damaged while drawing
  update rate; maybe present
```

Presentation also happens on the render thread, between jobs, so a half-drawn tile
is never shown.

A single stroke too costly for one slice, even at `T_min`, is drawn whole and
overruns that one slice. The overrun is logged. Drawing one long stroke in
resumable segment ranges is possible, because a stroke has one color, but it is
deferred until a measurement shows it is needed.

## The coarse pass

<a id="coarse-pass"></a>

Used when the visible tiles have no usable pixels: first open, `doc_load`, a jump
far past the pad. It is the revised form of the cumulative-count proposal:

```text
coarse(viewport, costBudget):
  heap = cells of the Document’s R-tree, keyed by panel size, largest first
  spent = 0
  while heap is not empty and spent < costBudget:
    c = pop the largest
    if c misses the viewport: continue                  prune A
    if panelSize(c) < τ: dot; continue                  prune B
    if c is a drawable entry: draw it; spent += c.cost; continue
    push c’s children (cells, entries; containers open into their own R-tree)
  for each cell left in heap that meets the viewport: dot at its centre
  mark visible tiles Coarse; present once; enqueue full jobs for them
```

Expanding the largest cells first spends the budget where it is most visible, and
it stops at different depths in different regions: sparse areas finish completely,
and dense areas stop high up as dots. The pass runs in a single read section, and
nothing from it is kept: the tile jobs that follow redraw every tile correctly.

## Events

<a id="events"></a>

| Event | Effect on tiles | Epoch |
|---|---|---|
| Pan within the pad | Shift the image by `(dx, dy)`. Tiles keep their state. Tiles newly inside the padded set become `Missing` and are queued. | unchanged |
| Pan beyond the pad | Overlapping tiles shift. The rest are `Missing`. Run the coarse pass if the visible area is mostly `Missing`. | unchanged |
| Zoom | Scale the previous image into place as `Stale` stand-ins. Queue every visible tile, then the pad. | +1 |
| Orientation change | As for zoom | +1 |
| Commit damage | Each tile meeting a damaged world box: `damageSeq += 1`; `Complete → Stale`, keeping its pixels; queued at priority 1 or 2 | unchanged |
| `doc_load` | All tiles `Missing`; coarse pass | +1 |

A pan within the pad is the padded rendering idea: the pad is what makes a small
shift land on pixels that are already drawn. Tiles are that pad split into units the
queue can schedule. The empty strips of the earlier draft are the newly exposed
tiles.

## Presenting on e-ink

<a id="present"></a>

Every panel update is a partial refresh with a visible cost. Showing each tile as it
finishes would cause a storm of refreshes. The rules:

- Present the union of visible tiles that changed, at most once every
  `presentEveryMs`, or as soon as every visible tile is `Complete`.
- Pad tiles never cause a present.
- The coarse pass presents once.
- Damage from the user’s own ink commit only replaces pixels the pen fast path already
  shows. Its present may be skipped if the panel policy says the pixels are the same.

Which waveform each present uses belongs to the panel adapter, not to this file.

## Convergence

If the writer stops editing and the camera stops moving, then within a bounded
number of slices every tile in the padded set is `Complete`, and each one matches a
single-threaded render of the final document.

Why: every edit’s damage reaches the render thread after the edit is published
([concurrency.md](./concurrency.md#commit)). A tile is marked `Complete` only if no
damage arrived while it was being drawn. Any tile damaged after its job started gets
a new job, and that job’s read section starts after the publish.

## Starting values

All are proposals to tune on the device, not measured constants.

| Value | Start |
|---|---|
| `τ` (dot threshold) | 2 px |
| `T` (tile) / `T_min` | 256 px / 64 px |
| Pad | 1 tile on each side |
| `sliceMs` | 12 ms |
| `presentEveryMs` | 250 ms |

## Not in this file

- **Simplifying strokes for the screen** (dropping samples finer than a pixel at
  zoom-out) would be a third saving, alongside A and B. It is deferred until
  measured.
- **Keeping raster tiles from earlier zoom levels** to reuse as stand-ins is deferred.
  The previous zoom’s image already serves as the stand-in.

## Verification

| Claim | What would refute it | Status |
|---|---|---|
| No slice exceeds `sliceMs` except a logged single-stroke overrun | Slice time histogram on a 100k-stroke fixture | **Pending** |
| Dense document at middle zoom shows a full picture within one coarse pass | Time to first present | **Pending** |
| Tiles converge to a fresh single-threaded render | Pixel diff after edits stop | **Pending** |
| Pan within the pad draws no visible tile | Job log during a small pan | **Pending** |
| Progressive presents do not cause a refresh storm | Refresh count per second during refine | **Pending** on device |
