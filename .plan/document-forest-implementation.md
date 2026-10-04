---
title: Document forest — implementation plan
owner: architect
source: ADR-0041
status: active
date: 2026-10-04
track: TRACK-009
---

# Document forest — implementation plan

Implements [ADR-0041](../.docs/adr/ADR-0041-document-forest.md) (Document forest,
concurrent readers, progressive paint) in the rebuilt `epaper/`. The specification is
[document-forest/](../.docs/domain/document-forest/index.md). This file orders the
work. It does not restate the design: each step says what to build, why it sits
where it does, and links to the spec that defines it.

**Status:** step 0 done 2026-10-04. Owned by [TRACK-009](./tracks/TRACK-009-document-forest.md)
(Document forest) in [iter-006](./iter-006/iter.md), one story per step. The human writes the
code; agents guide and review.

## Scope

**In:**
- the forest in `epaper/`: geometry, slots and handles, nodes and children, R-tree,
  concurrency, manipulation, connector, progressive paint;
- local undo and redo on the forest.

**Out:**
- **Infini, and any connection or sync with it**: the wire mapping, `doc_load` from the
  desktop, and forest ↔ wire conversion. Deferred by the human on 2026-10-04
  ([CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)).
- **Recognizers and tools**: enclose, connector recognition, erase, clipboard,
  selection chrome. They are product features that will run *on* the forest. Their
  rules stay in their SRS ([index.md](../.docs/domain/document-forest/index.md#what-this-model-does-not-restate)).
- **`Text`**, which is not a kind in this model.
- **Using `transform`**, which stays reserved.
- **Deleting `epaper_old/`.**

## Ground rules for every step

- **Docs first.** If code must differ from the spec, change the spec, or open a
  challenge, before merging the code ([docs-first](../.agent/rules/docs-first.md)).
- **No Qt in the document core.** `epaper/src/doc/` is plain C++17 with no Qt
  includes. It then builds and tests on the host, and runs under the thread and
  address sanitizers. Qt appears only in rendering and app glue.
- **C++17 atomics.** Per-node small fields are `std::atomic<…>`, accessed relaxed
  inside the `version` sequence lock
  ([concurrency.md](../.docs/domain/document-forest/concurrency.md#rule-1--small-fixed-size-fields-in-place-under-a-version)).
  `epaper/CMakeLists.txt` is C++17 today.
- **Each step ends with evidence:** a test, a benchmark or a device measurement,
  named in its *Done when* line. A passing build is not evidence.
- **Traceability.** Code tags point at each story's parent SRS (for example
  [SRS-EP-80](../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries)
  for the R-tree), or at the story where no SRS fits. The domain docs carry no SRS IDs
  ([traceability.md](../.agent/rules/traceability.md#domain-entities-vs-behavior-ids)).

## Code layout

```text
epaper/src/doc/            C++17, no Qt; static library epaper_doc
  geometry/                Pt, Aabb, WorldPt/WorldAabb, Transform, ResizeMap, panel ↔ frame ↔ world
  store/                   Handle, SlotTable, NodeStore, epochs (EBR)
  node/                    NodeStorage header, per-kind payloads, Children, child space, bounds
  index/                   per-container R-tree (cells, STR, R*, path copy, queries)
  commit/                  writer: commit protocol, invariants checker, damage queue
  manip/                   move / resize per kind, multi-selection, reparent
  connector/               payload, derive (warp), dependents
  history/                 undo and redo entries (ids, inverse ops)
epaper/src/render/         Qt: region painter, tiles, job queue, present
epaper/tests/              host tests (ctest), sanitizer presets, benchmarks
```

This layout is a proposal. The developer may adjust it in step 1.1; the dependency
direction must stay `render → doc`, never the reverse. A `wire/` folder comes back only
with [CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md).

## Milestones

| Milestone | Steps | What you can show |
|---|---|---|
| **A. Single-threaded forest** | 1, 2, 3, 4.1 | Pen strokes committed into a forest and painted from it, on one thread |
| **B. Concurrent** | 4.2, 4.3, 5 | Render thread reads while the writer edits; sanitizers clean |
| **C. Manipulation** | 6 | Move and resize of every kind, ink box modes, reparent |
| **D. Connector** | 7 | Connector warp from the forest, equal to the old fixtures |
| **E. Progressive paint** | 8 | Tiles, slices, coarse pass and batched e-ink present on the device |
| **F. Undo** | 9 | Undo and redo of every structural edit on the forest |

Order: 0 → 1 → 2 → 3 → 4 → 5, then 6, 7 and 8 in any order, and 9 last. Step 7
needs step 6’s reparent and move. Step 9 needs step 6.

---

## Step 0 — Product records and planning

**Done 2026-10-04.** No code. This step made the plan executable under the repository's rules.

### 0.1 Settle the Epaper product records

**Done.** Human decision, recorded in
[CHL-0033](./iter-006/challenges/CHL-0033-forest-product-records.md) (Epaper product records
under the document forest):
- **Replaced now**, because this plan builds them: SRS-EP-79 by
  [SRS-EP-80](../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-80-forest-geometry-queries)
  (geometry queries on the forest) and SRS-EP-78 by
  [SRS-EP-81](../.docs/modules/epaper/features/device-document/srs-quality.md#srs-ep-81-forest-query-quality)
  (their quality bars).
- **Deprecated** until each tool is ported: REQ-06 and SRS-EP-10, -11, -12, -14, -21, -75, -76
  and -77. Each note names what its successor must change.
- **Not affected:** REQ-08 (still parked), SRS-EP-09 (wire binding, deferred with sync) and
  SRS-EP-18 (connector warp). REQ-04 and SRS-EP-07 stay active with a change note.

### 0.2 Wire mapping

**Deferred.** The human put Infini sync out of the implementation scope
([CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)). The choice between
Infini adopting `manipMode`, a new wire field, and resolved geometry waits for that challenge to
reopen. The old steps 9.1 (`doc_load`) and 9.2 (forest ↔ wire) left the plan with it.

### 0.3 Track and stories

**Done.** [TRACK-009](./tracks/TRACK-009-document-forest.md) (Document forest) holds one story
per step, with step 3 split so Milestone A is its own story. iter-005 closed through the retro
gate and [iter-006](./iter-006/iter.md) commits Milestones A and B. STORY-EP-078…080 were
cancelled. The execution lock in [MASTER.md](./MASTER.md) names TRACK-009.

### 0.4 ADLC configuration

**Done.** `.agent/.adlc-local.json` sets the source roots to `epaper`, `infini` and `macOS` and
marks the configuration ready, so code edits and trace scans are allowed.

---

## Step 1 — Foundations

**Story:** [STORY-EP-082](./iter-006/stories/STORY-EP-082.md)

### 1.1 Host test target and the core library

Create `epaper_doc` as a static library with no Qt, plus a host test executable run
by `ctest`. Add build presets for ThreadSanitizer and AddressSanitizer. The old tree’s
tests were standalone programs with shell runners (`epaper_old/tests/`). A `ctest`
target replaces that pattern; the test framework is the developer’s choice.

Details: [Ground rules](#ground-rules-for-every-step).

**Done when:** an empty test passes on the host in all three presets (plain, thread
sanitizer, address sanitizer), and the reMarkable 2 build still links.

### 1.2 Geometry types

`Pt`, `Aabb` (min/max, empty rule), `WorldPt` / `WorldAabb`, `Transform` (kept, always
identity), `ResizeMap` (`sx, sy, tx, ty`, built from a knob and a new rectangle),
and the panel ↔ frame ↔ world map ported from `epaper_old/drawing/canvas_frame.hpp`.

Details: [index.md — Geometry](../.docs/domain/document-forest/index.md#geometry)
· [Manipulation contract](../.docs/domain/document-forest/index.md#manipulation-contract).

**Done when:** unit tests cover box union and intersection, the empty box, building
`ResizeMap` from each of the 8 knobs (opposite side fixed), and a round trip of the
panel map against the cases in `epaper_old/tests/canvas_frame_test.cpp`.

---

## Step 2 — Slots and handles

**Story:** [STORY-EP-083](./iter-006/stories/STORY-EP-083.md)

Single-threaded first. The atomics are declared now, so step 5 changes no layout.

### 2.1 Slot table

Chunks of 1024 slots, the directory, `chunkCount`, `nextUnused`, the free list and
the `retired` list. `Handle = {slot, gen}`. `resolve`.

Details: [handles.md — the parts](../.docs/domain/document-forest/handles.md#the-parts-and-how-they-connect)
· [Resolving a handle](../.docs/domain/document-forest/handles.md#resolving-a-handle).

**Done when:** growth across a chunk boundary keeps every slot address stable (test
compares addresses before and after).

### 2.2 `NodeStore`, allocate and free

The id → handle map, `allocate` (free list first, then `nextUnused`), and `free`
(`gen + 1`, the wrap rule). Grace periods are immediate until step 5.

Details: [allocate](../.docs/domain/document-forest/handles.md#allocate)
· [free](../.docs/domain/document-forest/handles.md#free)
· [Worked example: slot 7](../.docs/domain/document-forest/handles.md#worked-example-slot-7).

**Done when:** the slot-7 example runs as a test (a stale handle resolves to `Gone`
after reuse), and a slot at `gen = 2³² − 1` is never reused.

---

## Step 3 — Nodes and children

### 3.1 Node header and payloads

**Story:** [STORY-EP-084](./iter-006/stories/STORY-EP-084.md)

`NodeStorage` holds the header (`id`, `type`, `parent`, `transform`, `bounds`,
`version`) and one pointer to a per-kind payload. Kinds: `Document`, `Frame`,
`Group` (with `origin`), `InkBox`, `Ink`, `Primitive` (seven shapes) and `Connector`
(payload only; behavior in step 7). No struct carries every kind’s fields.

Details: [Shared attributes](../.docs/domain/document-forest/index.md#shared-attributes)
· [Payloads](../.docs/domain/document-forest/index.md#payloads).

**Done when:** each kind can be constructed, read and destroyed in a test, and a
payload of one kind cannot be read as another.

### 3.2 `Children`, link, unlink, retire

An immutable `Children` holds the links sorted by `orderKey`, with midpoint keys,
append at `last + 2³²`, and renumbering when a gap runs out. `link`, `unlink` and
`retire` (including the subtree) work as specified, each edit publishing a new
`Children`. The R-tree is a flat list here; step 4 replaces it.

Details: [handles.md — Children](../.docs/domain/document-forest/handles.md#children)
· [Writer operations](../.docs/domain/document-forest/handles.md#writer-operations).

**Done when:** insert, remove, reorder and reparent each pass tests; a reparented
node keeps its handle; a removed container retires its whole subtree.

### 3.3 Child space, bounds and paint extent

Child origins: `InkBox` and `Frame` use `bounds.min`, `Group` its `origin`, and
`Document` none. Derived `bounds` per kind; stored `bounds` for `InkBox` and
`Frame`. The paint extent goes into the parent entry, with upward propagation that
stops when nothing changed.

Details: [Child space](../.docs/domain/document-forest/index.md#child-space)
· [Bounds](../.docs/domain/document-forest/index.md#bounds).

**Done when:** moving an ink box changes exactly one entry above it, and boundary
ink overhang widens only the paint extent.

### 3.4 Invariants checker

A debug-build check of every invariant after each commit.

Details: [Invariants](../.docs/domain/document-forest/index.md#invariants).

**Done when:** a randomized edit sequence (insert, remove, reparent, move) runs
10,000 commits with no violation.

### 3.5 Milestone A: paint pen ink from the forest

**Story:** [STORY-EP-085](./iter-006/stories/STORY-EP-085.md)

At pen-up, the existing pen path commits the stroke as an `Ink` under `Document`.
The canvas paints it by a synchronous, single-threaded visit of the forest. There
are no tiles yet.

Details: [rendering.md — pipeline](../.docs/domain/document-forest/rendering.md).

**Done when:** on the device, strokes persist after pen-up and redraw from the
forest after a full refresh, with no change in pen latency (measured, compared with
the commit before this step).

---

## Step 4 — R-tree per container

**Story:** [STORY-EP-086](./iter-006/stories/STORY-EP-086.md)

### 4.1 Bulk-loaded tree and queries

Cells hold `{box, cost sum, count}`. STR bulk load. The generic visit takes a
`decide` function (skip, enter, stop with a proxy, accept). Queries: paint region,
point, rectangle, polygon candidates.

Details: [spatial-index.md](../.docs/domain/document-forest/spatial-index.md)
· [Queries](../.docs/domain/document-forest/spatial-index.md#queries).

**Done when:** every query equals a brute-force walk on random forests (property
test).

### 4.2 Dynamic updates by path copy

R* insert and split, delete with reinsertion below `m = 6`, the root rules, entry
replacement with shrinking. Cells are immutable once published; each update copies
one path and publishes a new `Children`.

Details: [Parameters](../.docs/domain/document-forest/spatial-index.md#parameters)
· [Maintenance](../.docs/domain/document-forest/spatial-index.md#maintenance).

**Done when:**
- after random edits, every cell holds 6 to 16 entries, except the root;
- the property test from 4.1 still passes;
- an old root a test holds still answers its old query unchanged.

### 4.3 Both prunes in one descent

Outside the region: skip (prune A). Smaller than `τ` on the panel: one gray dot
(prune B), for cells as well as nodes. Paint order comes from `orderKey` per
container.

Details: [Prunes A and B](../.docs/domain/document-forest/spatial-index.md#both-prunes-in-one-traversal)
· [rendering.md](../.docs/domain/document-forest/rendering.md).

**Done when:**
- a property test shows that prune B never hides a cell with a descendant larger
  than `τ`;
- a benchmark shows a point query visiting `O(log n + k)` cells on a root with
  100k strokes.

---

## Step 5 — Concurrency

**Story:** [STORY-EP-087](./iter-006/stories/STORY-EP-087.md)

### 5.1 Granule rules

- Rule 1: a sequence lock on header and scalar fields.
- Rule 2: build beside, then publish with one release store, for payload buffers
  and `Children`.
- Rule 4: already in place from step 4.2.
- Rule 3 (append-only live ink) is optional. It is skipped, because ink commits at
  pen-up.

Details: [How each kind of data is written](../.docs/domain/document-forest/concurrency.md#how-each-kind-of-data-is-written).

**Done when:** a reader loop that copies headers and walks children under a writer
loop shows no torn read in a ThreadSanitizer run.

### 5.2 Commit protocol and damage

`commitSeq` is odd during a commit. Changed nodes are written deepest first, with
the upward update; dependents are re-derived; then the commit closes and damage is
posted through a release/acquire queue.

Details: [Commit protocol](../.docs/domain/document-forest/concurrency.md#commit-protocol).

**Done when:** a test shows that any render job started after a damage message sees
that commit.

### 5.3 Read sections and epoch-based reclamation

`enterRead` and `exitRead` announce the epoch. Retired buffers and slots are freed
two epochs later. Step 2’s immediate grace period is replaced.

Details: [Reclamation](../.docs/domain/document-forest/concurrency.md#reclamation)
· [Slot lifecycle](../.docs/domain/document-forest/handles.md#slot-lifecycle).

**Done when:** a stress run with one writer churning slots and two readers looping
reports nothing under AddressSanitizer or ThreadSanitizer.

### 5.4 Exact queries from a reader

A reader retries up to 3 times against `commitSeq`, then hands the query to the
writer.

Details: [Exact queries](../.docs/domain/document-forest/concurrency.md#exact-queries-from-a-reader-thread).

**Done when:** a membership query terminates under continuous inking (test with a
writer committing every 5 ms).

**Fallback.** If the granule rules cost too much to build, a reader–writer lock with
slice-sized jobs sits behind the same read-section API
([Alternatives](../.docs/domain/document-forest/concurrency.md#alternatives)). That
choice is the architect’s, recorded in ADR-0041’s revision history. Steps 6–9 do not
depend on which one is used.

---

## Step 6 — Manipulation

**Story:** [STORY-EP-088](./iter-006/stories/STORY-EP-088.md)

### 6.1 Contract and gating

`move(Δ)` and `resize(m)` per kind. A container passes a pure scale to its children.
Shared rules: stroke width kept, no flip, 1 world unit minimum, no move or resize
below 96 px on the panel.

Details: [Manipulation contract](../.docs/domain/document-forest/index.md#manipulation-contract).

**Done when:** each kind’s response is reachable through one entry point, and a knob
below the minimum size clamps.

### 6.2 Per-kind responses

- **Ink:** free, non-uniform resize, with flat-axis knobs hidden.
- **Primitive:** per shape, with `Square` and `Circle` always aspect-locked.
- **Group:** `origin`.
- **Frame:** artboard.
- **InkBox:** `All` / `Boundary`.
- **Connector:** detached move only.

Details: [Ink](../.docs/domain/document-forest/index.md#ink-manipulation)
· [Primitive](../.docs/domain/document-forest/index.md#primitive-manipulation)
· [Group](../.docs/domain/document-forest/index.md#group-manipulation)
· [Frame](../.docs/domain/document-forest/index.md#frame-manipulation)
· [InkBox](../.docs/domain/document-forest/index.md#inkbox-manipulation)
· [Connector](../.docs/domain/document-forest/index.md#connector-manipulation).

**Done when:** the verification rows of
[index.md](../.docs/domain/document-forest/index.md#verification) pass as tests:
- nested `All` resize applies each box’s own mode;
- a group resize keeps circles round, and a group move writes only `origin`;
- `Boundary` content follows the top-left corner.

### 6.3 Multi-selection and reparent

Only the top-most selected nodes are manipulated. The world map is converted into
each node’s parent child space. Reparenting keeps world placement.

Details: [Moving or resizing several nodes](../.docs/domain/document-forest/index.md#moving-or-resizing-several-nodes)
· [Reparenting](../.docs/domain/document-forest/index.md#reparenting).

**Done when:** moving a box into a nested box leaves every world sample position
unchanged (test), and the node keeps its handle.

---

## Step 7 — Connector

**Story:** [STORY-EP-089](./iter-006/stories/STORY-EP-089.md)

### 7.1 Payload and derive

Port the warp from `epaper_old/document/connector_warp.hpp` onto the new payload:
rest spine and body, `Curve` / `Ink` style, markers, side anchors on the current
`bounds`, the face-frame departure, and the centre-end ray to the peer.

Details: [connector.md — payload](../.docs/domain/document-forest/connector.md#payload)
· [Departure](../.docs/domain/document-forest/connector.md#departure-stays-fixed-relative-to-the-node)
· [Deriving the route](../.docs/domain/document-forest/connector.md#deriving-the-route).

**Done when:**
- for every fixture of `epaper_old/tests/connector_warp_test.cpp`, the new
  `route.body` matches the old output;
- tests show the departure angle unchanged after a knob resize, an `All`-mode
  ancestor resize and a reparent.

### 7.2 Dependents, missing ends and detached move

The `dependents` index re-derives connectors in the same commit. A `Gone` end uses
its `lastPose`. A connector with both ends detached moves by its saved end positions.

Details: [Missing end](../.docs/domain/document-forest/connector.md#missing-end)
· [Who computes what](../.docs/domain/document-forest/connector.md#who-computes-what-and-when).

**Done when:** deleting a bound box keeps the connector drawn; undo rebinds it
through the id; moving a detached connector shifts its route exactly by `Δ`.

### 7.3 Labels

A node hung on a connector follows it through a `Follow` placement, re-derived with
the route.

Details: [Labels](../.docs/domain/document-forest/connector.md#labels).

**Done when:** a labelled node stays at its `t` and offset after either end box
moves.

---

## Step 8 — Progressive paint

**Story:** [STORY-EP-090](./iter-006/stories/STORY-EP-090.md)

### 8.1 Tiles and the padded buffer

The padded image is split into tiles (`T = 256`, `T_min = 64`). Tile states, a
`damageSeq` per tile, and a buffer shift on pan.

Details: [rendering.md](../.docs/domain/document-forest/rendering.md).

**Done when:** after a pan, only the newly exposed tiles are rendered (counted in a
test).

### 8.2 Job queue, epochs and slices

The queue is owned by the render thread, with priorities. `cameraEpoch` cancels
stale jobs instead of clearing the queue. Slices of about 12 ms, splitting of
expensive tiles, and a cost model with an EWMA time rate.

Details: [rendering.md — queue and slices](../.docs/domain/document-forest/rendering.md).

**Done when:** after edits stop, every tile equals a fresh single-threaded render
(pixel diff test).

### 8.3 Coarse pass and present

A best-first coarse pass for a first picture when no pixels exist. E-ink present
batched at most every 250 ms.

Details: [rendering.md — coarse pass and e-ink](../.docs/domain/document-forest/rendering.md).

**Done when:** on the device, measurements of slice time, first present and refresh
count on a dense document are recorded. The starting values `τ = 2 px`, 12 ms
slices and a pad of 1 tile are then confirmed or tuned, and the result is written
into rendering.md.

---

## Step 9 — Undo on the forest

**Story:** [STORY-EP-091](./iter-006/stories/STORY-EP-091.md)

Inverse-op undo ([ADR-0032](../.docs/adr/ADR-0032-inverse-op-undo.md), Inverse-op undo
per session). Ids, not handles, are kept in undo entries. An undo of a remove
allocates new slots under the same ids. The skip and no-op rules are those of
[SRS-EP-07](../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-07-device-document).

Details: [handles.md — where handles are not used](../.docs/domain/document-forest/handles.md#8-where-handles-are-not-used).

**Done when:** undo and redo of insert, remove, move, resize and reparent restore the
same world geometry and ids.

`doc_load` and forest ↔ wire conversion are out of scope
([CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)). When sync returns,
the design for replacing the whole document is in
[handles.md](../.docs/domain/document-forest/handles.md#9-replacing-the-whole-document).

---

## Risks and open decisions

| Item | Effect if it goes wrong | Where it is handled |
|---|---|---|
| Wire mapping of `manipMode` and child-relative coordinates | Device and Infini disagree after a resize | Deferred with Infini sync ([CHL-0034](./iter-006/challenges/CHL-0034-forest-wire-sync-deferred.md)) |
| Granule rules too costly to build | Milestone B slips | Fallback in step 5: reader–writer lock behind the same API |
| Starting values (`τ`, slice, tile, pad) | Paint too slow or too coarse on the device | Step 8.3 measures and tunes them |
| `Children` copy for very wide containers | Commit cost grows with child count | Measured in step 4 benchmarks; chunk the list if it shows ([ADR-0041 consequences](../.docs/adr/ADR-0041-document-forest.md#consequences)) |
| A tool is ported before its deprecated record has a successor | Code follows a record describing the old tree | Each port writes the successor first ([CHL-0033](./iter-006/challenges/CHL-0033-forest-product-records.md)) |

## Log

| Date | Event |
|---|---|
| 2026-10-04 | Plan written after the human accepted ADR-0041 |
| 2026-10-04 | Step 0 done: records settled (CHL-0033), Infini sync deferred (CHL-0034), TRACK-009 and stories EP-082…091 opened in iter-006. Step 9 reduced to local undo |
