---
title: Document forest — concurrency
lifecycle: active
owner: architect
source: ADR-0041
---

# Concurrency: one writer, concurrent readers

<a id="concurrency"></a>

Part of the [document forest](./index.md). Decision: [ADR-0041](../../adr/ADR-0041-document-forest.md).

## Situation

At the same moment:

- a **reader** thread is walking the forest to render tiles, or to answer a query
  such as a hit-test or a lasso preview; and
- the **writer** thread is changing the forest because of ink, manipulation, erase,
  undo or sync.

The reader must not crash, must not read a half-written value, and must converge on
the current document. The writer must not wait for the reader. Nobody clones the
document.

The pen’s live stroke is drawn by the existing fast path straight to the panel. That
path is not a document reader. The stroke becomes an `Ink` node when it is committed.

## Roles

| Role | Thread | May write the document | Reads |
|---|---|---|---|
| Writer | one document thread | yes, the only one | consistent: nobody else writes |
| Render reader | render thread | no | may see a mix of before and after; corrected by damage |
| Query reader | UI or tool thread | no | validated; retries if a commit happened during the query |

Every mutation is a request sent to the writer: a commit from a gesture, an inbound
sync op, an undo. The writer applies them one at a time. One writer means writes
never conflict, and every decision the writer makes reads a stable document.

**Rule:** a computation that *decides* an edit runs on the writer. That includes
draw-into membership, enclose capture, the 80% tests at commit, and reparent on
move. A reader may compute a preview of the same thing, but the writer recomputes
it before committing ([handles.md](./handles.md#use-results)).

## Guarantees to a reader

A reader:

1. never reads freed memory;
2. never reads half of one write to a field group (a transform, a sample buffer, a
   child list);
3. never mistakes a new node for an old one ([handles.md](./handles.md));
4. **may** see some nodes after a commit and others before it, while inside one visit;
5. converges: once the writer goes idle, every rendered region is eventually
   re-rendered from the final document.

Guarantee 4 is the price of not copying. Guarantee 5 is what makes it acceptable
for paint. Queries that cannot accept 4 use the [commit sequence](#exact-queries).

## How each kind of data is written

<a id="granules"></a>

One principle covers every case: **a reader never sees a partly written group of
fields, and memory is never freed while a reader may still hold it.** Data is
divided into *granules*, and each kind of granule is written by one of four rules.

<a id="header-fields"></a>

### Rule 1 — small fixed-size fields: in place, under a version

Header `transform` and `bounds`, plus a payload’s scalar fields (style,
`manipMode`, a group’s `origin`, a connector’s `style`), are written in place. Each node’s
`version` is used as a *sequence lock* (seqlock):

```text
writer:                               reader:
  version += 1        // now odd        loop:
  write the fields                        v1 = version (acquire)
  version += 1        // even again       if v1 is odd: retry
                                          copy the fields
                                          v2 = version (acquire)
                                          if v1 == v2: done, else retry
```

The writer never blocks. A reader retries only if it overlapped the few
instructions of that one write. In C++ every such field is accessed atomically, so
this is not a data race. `epaper/` builds as C++17, so the fields are declared
`std::atomic<…>` and read and written with `memory_order_relaxed`, inside the
`version` acquire/release pair. `std::atomic_ref` would need C++20.

### Rule 2 — variable-size data: build beside, swap one pointer

Ink samples replaced by erase, polygon points, a connector’s rest path, and each
container’s `Children` (its links together with its R-tree) are reached through one
pointer per granule.

```text
writer:
  build the new buffer completely
  store the pointer (release)
  retire the old buffer, tagged with the current epoch
```

A reader that loaded the old pointer keeps reading the old, complete buffer. A
reader that loads after the store reads the new one. Only the replaced granule is
copied, for example one container’s child list, never the document.

`Children` is one granule because its links and its R-tree must agree with each
other. A reader always sees a list and an index from the same version.

### Rule 3 — append-only growth: chunks plus a published count

If a stroke is ever appended to while it is already in the document, its samples
live in fixed-size chunks that never move. The writer writes the new samples, then
publishes the count with a release store. A reader loads the count with acquire and
reads that many. Nothing is copied. The default flow commits ink at pen-up, so this
rule is optional. It is specified so that live ink in the document is possible
without changing the model.

### Rule 4 — the R-tree inside a `Children`: path copy

Changing one child’s entry copies only the R-tree cells on the path from that
entry’s leaf cell to the root, then publishes a new `Children` by Rule 2. Cells off
that path are shared by the old and new versions. A reader descending the old
version sees a complete old tree. Cost: about `M · log_M(n)` entries per update
([spatial-index.md](./spatial-index.md#maintenance)).

### Topology

Insert, remove, reorder and reparent are Rule 2 applied to the affected parents’
`Children`. Remove then marks the slot `Retired` ([handles.md](./handles.md#slot-lifecycle)).
Reparent publishes the new parent’s `Children` before the old parent’s, so a reader
may briefly see the node in both places, which only causes extra paint, rather than
in neither.

## Commit protocol

<a id="commit"></a>

```text
commit(request):
  validate against the current document        writer reads are stable
  commitSeq += 1                              odd: a commit is in progress
  for each changed node, deepest first:
      write its granules (rules 1–4)
      recompute its derived bounds and paint extent; update its entry in the parent’s R-tree
      continue up the parent chain while a container's paint extent, bounds or cost changed
  re-derive dependents (connectors bound to moved boxes, labelled nodes)
  commitSeq += 1                              even: the commit is complete
  retire replaced granules at the current epoch
  post damage = world extents of every changed node, before and after
```

Updating deepest first means a parent’s entry box never shrinks to exclude a child
that is already visible in a newer state.

**Damage is posted after the last publish.** The renderer receives it through a
message queue that has release/acquire ordering. Any render job that starts after
the damage arrives therefore enters its read section after the commit, and sees it.
That ordering is the whole convergence argument.

## Reading

<a id="read-sections"></a>

```text
enterRead():  announce the current epoch for this thread
… walk; pointers obtained here are valid until exitRead …
exitRead():   clear the announcement
```

A read section is meant to be short: one render job or one query. Long work is split
across sections. The renderer’s jobs are pixel regions, so nothing it keeps between
sections can be invalidated ([rendering.md](./rendering.md#why-regions)).

<a id="exact-queries"></a>

### Exact queries from a reader thread

```text
repeat up to 3 times:
  s1 = commitSeq (acquire); if odd, retry
  enterRead(); run the query; exitRead()
  s2 = commitSeq (acquire)
  if s1 == s2: return the result
send the query to the writer and return its answer
```

The fallback to the writer bounds the retries, even during continuous inking.

## What a render reader can see wrong, and why it heals

| Concurrent edit | What a render job may do | Healed by |
|---|---|---|
| Node moved | Use the old R-tree entry with the new transform, so cull it or draw it in the wrong place | Damage covers the old and new extents |
| Node inserted | Miss it, because the old `Children` is still published | Damage covers the new extent |
| Node removed | Reach it through the old `Children` and find it `Retired`, so skip it | Damage covers the old extent |
| Ancestor entry box not yet grown | Cull a child that is now visible | Damage |
| Connector end moved | Draw the connector at its previous warp | Damage includes the connector’s old and new extents |

Each case is wrong only inside the area that the edit damages, and that area is
re-rendered after the edit is published.

## Reclamation

<a id="reclamation"></a>

Retired granules and `Retired` slots are freed with *epoch-based reclamation* (EBR):

```text
globalEpoch: E
each reader: announced epoch, or idle

writer, after a commit:
  if every non-idle reader has announced E: globalEpoch = E + 1
  free garbage retired at epoch ≤ E − 2       nobody can still hold it
  free Retired slots retired at epoch ≤ E − 2: gen += 1, state = Free
```

With one or two reader threads this is a few atomic loads per commit. Garbage
outlives a commit by about two read sections, and read sections are one render job
long, so memory held for reclamation stays small. A reader that stalls for a long
time delays freeing, but it never makes a free unsafe.

## Alternatives

| Alternative | Reader | Writer | Why not chosen |
|---|---|---|---|
| **A. Granule rules plus EBR (chosen)** | Lock-free; per-node mixed state; heals through damage | Never waits; copies only what it changes | — |
| B. One reader–writer lock | Consistent | Waits for the current render job | Edits would wait for visits instead of running alongside them. With jobs capped at one slice (about 10 ms) this is **credible as a first step**, and the switch to A is local. Kept as the fallback if the granule rules prove too costly to build. |
| C. Persistent tree with path copy to the root | Consistent snapshot | Every edit copies its ancestors’ child lists | A deeply nested erase copies every ancestor’s list. Handles would also have to become version-specific. Revisit if many readers need multi-node consistency. |
| D. Render thread keeps its own replica, updated by ops | Consistent | Cheap | That replica is a second document |
| E. Clone the document per visit | Consistent | Cheap | Forbidden |

## Verification

| Claim | What would refute it | Status |
|---|---|---|
| A reader never reads a torn granule or freed memory | ThreadSanitizer or AddressSanitizer report in a stress test (render loop plus edit loop) | **Pending** |
| The writer never waits for a reader | A lock or wait on the commit path | **Pending** (review) |
| Paint converges after edits stop | A tile that stays different from a fresh single-threaded render | **Pending** |
| Exact queries terminate during continuous inking | The retry loop never exits | **Pending** |
