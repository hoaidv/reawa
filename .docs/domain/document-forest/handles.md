---

## title: Document forest — handles
lifecycle: active
owner: architect
source: ADR-0041

# Handles: `(slot, generation)`



Part of the [document forest](./index.md). Decision: [ADR-0041](../../adr/ADR-0041-document-forest.md).

## The problem a handle solves

Many things in the program refer to a node: its parent’s child list, R-tree entries,
the selection, a move gesture in progress, a hit-test result passed between threads,
a connector’s ends, a render cache. They need a reference that is:

1. **cheap to follow**, because a visit follows thousands of them per frame;
2. **safe after the node is removed**, because a reader on another thread may still
  be holding it; and
3. **able to tell that the node is gone even if the memory now holds a different
  node.** Reusing the same memory location for a new object, so that an old
   reference now silently points at the new one, is the *ABA problem*.

A raw pointer fails 2 and 3. A node id fails 1, because every step would be a hash
lookup. A plain array index fails 3. A handle meets all three.

## The parts and how they connect

```text
NodeStore                                  one per Document
├─ slots:      SlotTable                   where every node lives
├─ ids:        Map<NodeId, Handle>         writer only
├─ epochs                                  reclamation (concurrency.md)
└─ dependents: Map<NodeId, [NodeId]>       writer only (connector.md)

SlotTable
├─ directory:  atomic ptr → [ptr → Chunk]  the chunk pointers, in order
├─ chunkCount: atomic u32                  how many chunks are published
├─ nextUnused: u32                         slots below this have been used at least once; writer only
├─ freeList:   [u32]                       freed slot numbers ready for reuse, last in first out; writer only
└─ retired:    [(u32, epoch)]              slots waiting for their grace period; writer only

Chunk = Slot[1024]                         allocated once; never moves; freed only with the store

Slot                                       fixed size; fixed address for the life of the store
├─ gen:   atomic u32                       how many times this slot has been freed
├─ state: atomic SlotState                 Free | Live | Retired
└─ node:  NodeStorage                      the node itself, constructed in place

NodeStorage                                fixed size, so every Slot is the same size
├─ header:   id, type, parent: Handle, transform, bounds, version      (Rule 1)
├─ children: atomic ptr → Children         containers only; null for leaves (Rule 2); see Children below
└─ payload:  atomic ptr → <Kind>Payload    samples, shape, box fields, connector … (Rule 2)

Handle = { slot: u32, gen: u32 }           8 bytes; a value, held outside the table
```

The rules named in parentheses are the write rules in
[concurrency.md](./concurrency.md#granules).

`NodeStore` owns everything a node does not: the slot table, the id map, the
reclamation epochs and the connector dependents. It is not a node. There is one per
`Document`.

`SlotTable` is an array of slots, split into chunks of 1024 so it can grow
without moving any slot.

- To grow, the writer allocates a new chunk, writes its pointer into the directory,
then publishes `chunkCount` with a release store.
- When the directory itself is full, the writer builds a larger copy of it beside
the old one, swaps the `directory` pointer, and retires the old copy by epoch.
- The chunk pointers in both copies are the same, so no slot moves either way.
- No chunk is freed while the store exists, so any slot a handle names is always
readable memory, even when the handle is stale. Following a stale handle never
crashes.
- The chunk size 1024 is provisional.

`Slot` is one fixed place in that array. It holds the node’s storage, plus two
small atomics, `gen` and `state`, that say who currently occupies it. A slot is
reused for many nodes over the store’s life; `gen` tells those occupancies apart.

`NodeStorage` is the node: header fields in place, with its children and payload
behind one pointer each. It has a fixed size, so slots do too. Variable-size data
(samples, child lists, rest paths) lives in separate buffers that those pointers
reach.

`Handle` names one occupancy of one slot: the slot’s number, and the slot’s
`gen` at the moment the node was allocated there.

### Where is the handle in the slot table?

**Nowhere.** The table never stores handles to its own slots. A handle is held by
whoever refers to the node. It is an *address into* the table plus a *copy* of a
stamp:

- `slot` finds the slot: chunk `slot / 1024`, position `slot % 1024`.
- `gen` is a copy of that slot’s `gen` at allocation time. The slot keeps the current
value; the handle keeps the value it was issued with. Resolving compares the two.

Think of a locker. `slot` is the locker number. The slot’s `gen` counts how many
times the locker has been reassigned. A handle is your ticket: the locker number and
the assignment count at the time you got it. If the locker is emptied and given to
someone else, the count on the locker moves on, and your ticket no longer matches.

A node does not store its own handle either. Anyone who reached it did so through a
handle, so they already have it. The writer learns it at allocation, when it picks
the slot and reads `gen`.

Where handles are stored:


| Holder              | Field                                            | Lifetime                                   | Details                                      |
| ------------------- | ------------------------------------------------ | ------------------------------------------ | -------------------------------------------- |
| Parent’s `Children` | `ChildLink.node`, and each R-tree entry’s `node` | while that `Children` version is published | [1](#1-child-links-and-r-tree-entries)       |
| The node itself     | `header.parent`                                  | while linked                               | —                                            |
| `NodeStore.ids`     | map value                                        | while the node is `Live`                   | writer only                                  |
| Connector payload   | `NodeRef.cached` for each end and label          | until the writer updates it                | [6](#6-connector-ends-and-labels-noderef)    |
| Selection           | set of handles                                   | many frames                                | [3](#3-selection)                            |
| Tool gesture        | target handles                                   | pen down to pen up                         | [4](#4-gesture-targets)                      |
| Query result        | candidate handles                                | until used                                 | [5](#5-query-results-passed-between-threads) |
| Render caches       | key `(handle, version)`                          | until evicted                              | [7](#7-derived-caches)                       |




### How to get from one to another


| From          | To               | How                                                            |
| ------------- | ---------------- | -------------------------------------------------------------- |
| `Handle`      | `Slot`           | `directory[slot / 1024][slot % 1024]`: two loads, no lookup    |
| `Slot`        | `NodeStorage`    | the slot contains it: same memory                              |
| `Handle`      | node, or `Gone`  | `[resolve](#resolving-a-handle)`: compare `gen`, check `state` |
| `NodeStorage` | `NodeId`         | `header.id`                                                    |
| `NodeId`      | `Handle`         | `NodeStore.ids`: a hash lookup, writer only                    |
| `NodeStorage` | parent           | `header.parent`, a handle                                      |
| `NodeStorage` | children         | `children` pointer → `Children.links[i].node`, handles         |
| `NodeStorage` | its own `Handle` | not stored; the caller already holds it                        |




### `gen` and `state` together

`gen` and `state` are two separate atomics. Together they give the meaning of a
handle `(s, g)` issued while slot `s` had generation `g`:


| Slot state                   | Slot `gen` | `node` memory                               | `resolve((s, g))`            | Who can still reach the node                                                       |
| ---------------------------- | ---------- | ------------------------------------------- | ---------------------------- | ---------------------------------------------------------------------------------- |
| `Live`                       | `g`        | constructed; reachable once [linked](#link) | the node                     | anyone holding `(s, g)`, and readers once it is linked                             |
| `Retired`                    | `g`        | intact, but unlinked                        | `Gone`                       | only readers that loaded the parent’s old child list in their current read section |
| `Free`                       | `g + 1`    | destroyed: payload buffers released         | `Gone` (generation mismatch) | nobody                                                                             |
| `Live` again, for a new node | `g + 1`    | the new node                                | `Gone` (generation mismatch) | holders of `(s, g + 1)`                                                            |


Only three writer steps change them: [allocate](#allocate) stores `state = Live`;
[retire](#retire) stores `state = Retired`; [free](#free) stores `gen = g + 1`, then
`state = Free`. Linking and unlinking change the parent’s `Children`, never the
child’s slot.

A reader loads `gen`, then `state`, both with acquire, inside a read section. A slot
cannot pass from `Retired` to `Free` while that reader is inside its section, because
the grace period waits for it. That is why `resolve` is only valid inside a read
section.

### Worked example: slot 7


| Step | Event                                        | Slot 7 `gen` | Slot 7 `state` | Handles held                                                                                            |
| ---- | -------------------------------------------- | ------------ | -------------- | ------------------------------------------------------------------------------------------------------- |
| 1    | Stroke A is committed into slot 7            | 3            | `Live`         | parent link `(7, 3)`; `ids[A] = (7, 3)`                                                                 |
| 2    | The user selects A                           | 3            | `Live`         | + selection `(7, 3)`                                                                                    |
| 3    | Undo removes A                               | 3            | `Retired`      | link gone from the new child list; `ids[A]` removed; reader R still in the old list; selection `(7, 3)` |
| 4    | R leaves its read section; grace period over | 4            | `Free`         | selection `(7, 3)`                                                                                      |
| 5    | Stroke B is committed and reuses slot 7      | 4            | `Live`         | parent link `(7, 4)`; `ids[B] = (7, 4)`; selection `(7, 3)`                                             |
| 6    | The selection resolves `(7, 3)`              | 4            | `Live`         | `3 ≠ 4`, so `Gone`: A drops out, and B is never selected by accident                                    |
| 7    | Redo restores A, same id, into free slot 12  | —            | —              | `ids[A] = (12, g)`; connectors bound to A get `cached = (12, g)` through `dependents`                   |


A node id and a handle are different things:


|                           | Node id                                                     | Handle                                              |
| ------------------------- | ----------------------------------------------------------- | --------------------------------------------------- |
| Identifies                | the document object, across saves, sync, undo and processes | one occupancy of one slot in this process           |
| Survives undo of a delete | yes, the same id comes back                                 | no, the restored node gets a new slot or generation |
| Cost to follow            | hash lookup in the id map (writer only)                     | array index plus one compare                        |
| Crosses the wire          | yes                                                         | never                                               |




## Children

A container reaches its children through **handles**, never by containing them. The
child nodes stay in their own slots. The container holds a list of small links that
name those slots.

```text
container NodeStorage
└─ children: atomic ptr ──▶ Children                      one immutable version
                            ├─ links: [ChildLink]         contiguous; sorted by orderKey = paint order
                            │    ChildLink = { node: Handle, orderKey: u64, placement: Placement }
                            └─ index: ptr ──▶ R-tree cells  (spatial-index.md)
                                 leaf entry = { node: Handle, box, orderKey, cost, kind bits }

child NodeStorage
└─ header.parent: Handle ──▶ back to the container
```

An ink box in slot 2 with three children:

```text
slot 2  InkBox P   gen 0   children ─▶ Children v5
                                       links: [ (7, 3)  key 1·2³²  Boundary
                                                (9, 1)  key 2·2³²  Content
                                                (4, 6)  key 3·2³²  Content ]
                                       index: R-tree whose leaf entries name (7, 3), (9, 1), (4, 6)
slot 7  Ink        gen 3   parent = (2, 0)   children = null
slot 9  Ink        gen 1   parent = (2, 0)   children = null
slot 4  InkBox     gen 6   parent = (2, 0)   children ─▶ its own Children
```

**Handles only.** A `ChildLink` is a handle plus two small fields, about 24 bytes. A
container with 100,000 children holds 100,000 links, not 100,000 nodes. Growing the
list never moves a node, so a reader holding a child keeps a valid pointer.

**Two views of the same set.**

- `links` answers *in what order*: paint order, and iteration.
- `index` answers *which ones are here*: viewport, point, rectangle.

Both name exactly the same handles. Each leaf entry also carries `orderKey`, so a
spatial query can sort its hits into paint order without going back to `links`.

**Two directions, both handles.** Down is `Children.links[i].node`; up is the child’s
`header.parent`. The writer keeps them in agreement: handle `h` is in `P`’s links
exactly once if and only if the node at `h` has `header.parent = P`. Readers walk
down. Only the writer and upward bounds updates walk up.

**A published** `Children` **is never changed.** Each change builds a new version beside
it:

- the links copied with the change;
- the R-tree copied along one path (Rule 4).

Then one pointer store publishes it (Rule 2), and the old version is retired by
epoch. A reader that loaded version 5 keeps a list and an index that agree with each
other, even while the writer publishes version 6.

**Leaves.** `children` is null. Adding a child to a leaf is rejected. A container
with no children may also have a null `children`.

**Finding a child’s position** (writer only). `links` is sorted by `orderKey`, so a
binary search finds a link once its key is known. The child does not store its own
key, because the key belongs to the parent’s list, not to the node. The writer gets
it from the child’s R-tree entry: query the parent’s tree with the child’s current
entry box and match the handle. Both steps are logarithmic.

## Slot lifecycle

```text
          allocate              retire (after unlink)           free (grace period over)
Free(g) ─────────────▶ Live(g) ──────────────────────▶ Retired(g) ─────────────────────────▶ Free(g+1)
                         │  ▲
                         └──┘  link, unlink, reparent: the slot stays Live(g)
```

Only the writer moves a slot along this cycle, with the steps in
[Writer operations](#writer-operations). The handle handed out at allocation is
`(slot, g)`. The grace period lasts until every reader that might have read the old
child list has left its read section
([concurrency.md](./concurrency.md#reclamation)).

The generation goes up only when a slot is freed. A slot whose generation would wrap
past `2³² − 1` is never put back on the free list. That costs one slot per four
billion reuses.

## Writer operations

Five steps. Only the writer runs them, inside a commit
([concurrency.md](./concurrency.md#commit)). Every edit is a combination of them.


| Step                  | What it changes                                      | Slot state                    |
| --------------------- | ---------------------------------------------------- | ----------------------------- |
| [allocate](#allocate) | one slot; the id map                                 | `Free` → `Live`               |
| [link](#link)         | the parent’s `Children`; the child’s `header.parent` | stays `Live`                  |
| [unlink](#unlink)     | the parent’s `Children`                              | stays `Live`                  |
| [retire](#retire)     | slot `state`; the id map; the `retired` list         | `Live` → `Retired`            |
| [free](#free)         | slot `gen` and `state`; the free list                | `Retired` → `Free`, `gen + 1` |



| Edit                                                    | Steps                                                                                            | The node’s handle afterwards  |
| ------------------------------------------------------- | ------------------------------------------------------------------------------------------------ | ----------------------------- |
| Insert a new node (ink commit, paste, enclose)          | allocate, then link                                                                              | new                           |
| Remove (erase whole stroke, delete, undo of an insert)  | unlink, then retire the node and its subtree; free later                                         | stale: resolves to `Gone`     |
| Reparent (move into a box, enclose capture)             | link into the new parent, then unlink from the old one                                           | **unchanged**                 |
| Reorder (bring to front)                                | publish one new `Children` with a new `orderKey`                                                 | unchanged                     |
| Undo of a remove                                        | allocate with the **same id** into a new slot, then link; update `NodeRef`s through `dependents` | new                           |
| Edit a payload (resize, recolor, stroke split by erase) | Rule 2 or Rule 1 on that node; a split also inserts the new pieces and removes the original      | unchanged for the edited node |




### allocate

`allocate` finds a slot and builds the node in it.

```text
allocate(id, kind, fields) → Handle
  if freeList is not empty:
      s = freeList.pop()                     the most recently freed slot, still warm in cache
  else:
      s = nextUnused;  nextUnused += 1        a slot never used before; its gen is 0
      if s == chunkCount · 1024:
          growTable()                        add a chunk; enlarge the directory if it is full
  slot = table[s]                            state is Free; gen is g
  construct slot.node:
      header   = { id, kind, parent = none, transform = identity, bounds, version = 0 }
      payload  → a new payload buffer for this kind
      children → null
  slot.state.store(Live, release)
  ids[id] = (s, g)
  return (s, g)
```

**Finding the next free slot needs no search.** There are two sources, both
constant time:

- the free list, which holds slots that were used and then freed;
- `nextUnused`, the boundary of slots never used yet.

Scanning slots for a `Free` state would be linear in the table size.

**A slot a reader may still see is never handed out.**

- A retired slot waits in `retired`, not on the free list.
- It joins the free list only in [free](#free), after its grace period.
- So a slot on the free list cannot still be reached through any `Children` a
reader holds.

**Reuse order.** Taking the last freed slot first keeps memory warm. It does not
weaken anything: `gen` detects reuse whatever order slots come back in.

**A new node is not visible yet.** No published `Children` names it, so readers
cannot reach it from the tree. Only the writer holds its handle. It becomes visible
when it is linked.

### link

`link` adds an edge from a parent to a child: the child’s `ChildLink` and its R-tree
entry join the parent’s `Children`.

```text
link(P, h, orderKey, placement)
  node(h).header.parent = handle(P)          Rule 1, done before any reader can reach h
  newLinks = P.children.links with { h, orderKey, placement } inserted at its key
  newIndex = P.children.index with an entry for h (path copy, Rule 4)
  P.children.store(Children { newLinks, newIndex }, release)      Rule 2: readers can now reach h
  retire the old Children buffer at the current epoch
  update P’s paint extent, and its entry upward (commit protocol)
```

The release store is what makes the child safe to reach. A reader that loads the new
`Children` with acquire also sees the fully built child and its `parent` field.

### unlink

**Unlink means removing the edge from a parent to a child.** The child’s `ChildLink`
and its R-tree entry leave the parent’s `Children`. That is all it does:

- it does not destroy the node, free its slot or change its handle;
- it does not touch its subtree;
- it does not change its `state`.

```text
unlink(P, h)
  k = orderKey of h                          from P’s R-tree entry for h
  newLinks = P.children.links without the link at key k           binary search
  newIndex = P.children.index without h’s entry (path copy, Rule 4)
  P.children.store(Children { newLinks, newIndex }, release)
  retire the old Children buffer at the current epoch
  update P’s paint extent, and its entry upward
```

After the store:

- **New readers** cannot reach the child from the tree.
- **Readers already inside a read section** may still hold the old `Children`, and
through it the child. Its memory is intact for them, which is why unlinking is
safe without waiting.

Unlink is always half of an edit:

- **In a remove**, it is followed by [retire](#retire). Readers that reach the node
through an old list then find it `Retired` and skip it.
- **In a reparent**, it is preceded by `link` into the new parent. The child stays
`Live` with the **same handle**, so a selection or a gesture that holds it stays
valid across the move. The new parent is published first, so a reader may briefly
see the node in both places, which only causes extra paint, but never in neither
([concurrency.md](./concurrency.md#topology)).



### retire

`retire` marks an unlinked node as removed and starts its grace period.

```text
retire(h)                                    only after unlink: no published Children names h
  for each child c in node(h).children.links:   a removed container takes its subtree with it
      retire(c.node)
  slot(h).state.store(Retired, release)
  ids.remove(node(h).header.id)
  for each connector bound to this id (dependents): it now draws from lastPose (connector.md)
  retired.push((h.slot, currentEpoch))
```

The descendants are not unlinked from the removed container. Its `Children` still
names them, but no reader can reach that list from the root any more. A reader
already inside the subtree finds `Retired` nodes and skips them.

Undo of a remove does not revive the retired slots. It allocates new slots under the
same ids. A held handle to the removed node therefore stays `Gone`, and only the id
leads to the restored node.

### free

`free` runs during reclamation after each commit
([concurrency.md](./concurrency.md#reclamation)):

```text
for each (s, e) in retired with e ≤ globalEpoch − 2:   no reader can still see slot s
  destroy slot.node: release its payload buffer and its Children buffer
  if slot.gen == 2³² − 1:
      slot.state.store(Free, release)          never reused: it stays out of the free list
  else:
      slot.gen.store(slot.gen + 1, release)
      slot.state.store(Free, release)
      freeList.push(s)
```

The buffers are released at once, not retired again. They were reachable only
through this slot, and the slot has already passed its grace period.

## Resolving a handle

```text
resolve(h):                              valid only inside a read section, or on the writer
  s = slotTable[h.slot]
  if s.gen != h.gen:   return Gone       the slot was freed, and may now hold another node
  if s.state != Live:  return Gone       the node was unlinked and is waiting for its grace period
  return &s.node
```

The returned pointer is valid until the caller leaves its read section. The writer
may keep it for as long as it likes, because only the writer frees slots.

There are two lifetimes, and handles span both:


| Within one read section                                                                                 | Across read sections                                                         |
| ------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| Memory cannot be freed, so a pointer is safe. A `Retired` node is still readable; most readers skip it. | Memory may have been freed and reused. Only the generation compare can tell. |




## Every use

Each row says who holds the handle, for how long, and what happens when it is stale.



### 1. Child links and R-tree entries

**Holder:** the parent’s `Children` object. **Lifetime:** as long as that `Children`
version is published.

A reader descending through a `Children` it just read may find a child whose state
is `Retired`. That happens when the writer unlinked the child after the reader read
the parent’s list. The memory is still intact, because the reader has not left its
section. Paint and queries skip a `Retired` child. The writer has already posted
damage for its removal, so the next paint of that area is correct.

A structural link never has a stale generation. A slot cannot be freed while a
published `Children` names it, and a reader in its section cannot see a freed slot.



### 2. R-tree cells are not handles

R-tree internal cells are plain allocations, reclaimed the same way as other
replaced buffers. A reader may hold a cell pointer only inside its read section. The
progressive renderer therefore never keeps cells or a traversal frontier across
slices. Its queued work is pixel regions, not tree positions
([rendering.md](./rendering.md#why-regions)). Nothing outside a read section needs a
handle to a cell.



### 3. Selection

**Holder:** UI and selection state. **Lifetime:** many frames.

The selection is a set of handles. Each time it is used (chrome paint, a tool acting
on it), each handle is resolved. A `Gone` handle is dropped from the selection.

This is the ABA case. Suppose stroke A is in slot 7, generation 3, and it is
selected. Undo on the writer removes A. After the grace period, slot 7 becomes
generation 4. The next ink commit is stroke B, and it is allocated into slot 7 at
generation 4. The selection still holds `(7, 3)`. Resolving it fails, so A drops out
of the selection. With a bare slot index, the selection would now silently contain B,
and the next move would move a stroke the user never selected.



### 4. Gesture targets

**Holder:** a tool operation (move, resize, erase, connector drag). **Lifetime:** pen
down to pen up.

The operation records its targets as handles at pen-down. It previews them without
writing to the document. At pen-up it sends the writer one commit naming those
handles. The writer resolves them first. A `Gone` target (removed by sync, undo or
another commit during the gesture) is left out of the commit with a reason. The
writer never applies a gesture to whatever node now occupies the slot.



### 5. Query results passed between threads

**Holder:** whoever receives a hit-test or lasso result. **Lifetime:** until it is
used.

A reader thread computes candidates and returns handles. Those are plain values, so
they can be sent anywhere. When the result becomes an edit, the writer resolves
every handle again and re-runs the exact test it depends on (for example the 80%
inside test), because the document may have changed since the reader’s query. The
reader’s result was a proposal, not the decision.



### 6. Connector ends and labels: `NodeRef`

A connector refers to its bound boxes, and to labelled nodes, by `NodeRef`:

```text
NodeRef = { id: NodeId, cached: Handle }
```

The id is the persistent truth: it is what sync, save and undo understand. The
cached handle is the fast path that readers use.

- **Reader:** resolve `cached`. If the result is `Gone`, the bound box was deleted;
use that end’s `lastPose` ([connector.md](./connector.md#missing-end)). Readers never
consult the id map.
- **Writer:** when a node with that id comes back (undo of a delete restores the same
id into a new slot), the writer finds every connector that refers to that id via
the `dependents` index, updates `cached`, and re-derives them in the same commit.



### 7. Derived caches

**Holder:** render caches, such as a connector’s placed polyline or a per-node
raster. **Key:** `(handle, version)`.

The handle makes the key specific to one node, even across slot reuse. The version
makes it specific to that node’s current content. A stale key is a cache miss. It is
never the wrong pixels.



### 8. Where handles are not used


| Use                    | Uses                      | Why                                                      |
| ---------------------- | ------------------------- | -------------------------------------------------------- |
| Undo and redo entries  | node ids                  | Undo recreates nodes, which may land in a different slot |
| Sync, wire, save files | node ids                  | Handles mean nothing outside this process                |
| `dependents` index     | node ids                  | It must outlive one occupancy of a slot                  |
| Damage and render jobs | world or panel rectangles | Regions do not go stale when nodes change                |




### 9. Replacing the whole document

An accepted `doc_load` builds a new `NodeStore`. The old one is retired as a whole:
it stays readable until every reader has left, and then it is freed. A handle is
meaningful only against the store that issued it. On load, everything that holds
handles across sections is cleared: the selection, gesture targets, derived caches,
and the render job queue (by bumping its epoch).

## Rejected alternatives


| Alternative                        | Why not                                                                                                                                                                                                                                                                        |
| ---------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Raw pointers                       | Dangle after free and cannot detect reuse                                                                                                                                                                                                                                      |
| `shared_ptr` / `weak_ptr` per node | Every step of every visit would change an atomic reference count. The reader and writer threads would then contend on those counts, plus a control-block allocation per node. `weak_ptr::lock` on every child is the slowest form of the same check a generation compare does. |
| Node ids for child links           | One hash lookup per descent step                                                                                                                                                                                                                                               |
| Index without generation           | Has the ABA problem shown in [Selection](#use-selection)                                                                                                                                                                                                                       |




## Verification


| Claim                                               | Status                                                                                               |
| --------------------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| A stale handle never resolves to a different node   | **Pending.** Test: delete, force the grace period, reallocate the slot, then resolve the old handle. |
| A slot is not freed while a reader may still see it | **Pending.** Stress test under ThreadSanitizer, with a reader looping while the writer churns slots. |
| Every holder in this file resolves before use       | **Pending.** Implementation review.                                                                  |


