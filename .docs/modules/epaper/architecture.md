---
module: epaper
lifecycle: active
owner: architect
---

# Architecture — Epaper

A view, not a spec: it references SRS/ADR ids and defines none of its own.
View over [PRD](./prd.md). Sibling: [infini/architecture.md](../infini/architecture.md).

Created 2026-08-13 with [CHL-0008](../../../.plan/iter-003/challenges/CHL-0008-architecture-rework.md).
Until then the device was a thin client — ink in, pixels out — and needed no architecture view. It
now holds the working document ([ADR-0014](../../adr/ADR-0014-document-ownership-inversion.md)),
which is the largest single increase in device scope so far.

## Quality goals (prioritised)

1. **Ink latency** — pen-down → pixel p95 ≤30 ms, unconditionally
   ([SRS-EP-01](./features/local-pen-ink/srs-logic.md)). Everything below is subordinate: if the
   document, the undo ring, or the publisher cannot fit under this ceiling, they change, not the
   ceiling.
2. **Gesture truth** — committed geometry equals released geometry; 0 px jump, 0 snap-back
   ([SRS-EP-14](./features/ink-box/srs-quality.md)). This is the defect the whole rework exists to
   remove.
3. **Local sufficiency** — create, group, move, resize, and undo behave identically with the link
   down ([SRS-EP-13](./features/device-document/srs-quality.md) offline parity).
4. **Refresh discipline** — manipulation feedback ≥5 Hz with 0 full-panel invalidations
   ([SRS-EP-03](./features/region-sync/srs-quality.md)).
5. **Document fidelity** — a document authored here round-trips through desktop save/open unchanged
   (±1 world unit @ 100% zoom).

## Constraints

- reMarkable 2: 1872×1404 1-bit panel, partial-refresh floor ≈250 ms (`kRefreshMinIntervalMs`), no
  hover, no keyboard, no cursor. Pen for content, finger for chrome.
- Qt/QML fullscreen, C++; xochitl stopped while the app runs.
- Link is USB-Ethernet TCP and is **occasionally down** — treated as a normal state, not an error.
- **In-memory only this iter**: no on-device persistence, no file browser, no multi-document
  ([Non-Goals](./prd.md)). An app restart loses unpublished work, accepted by PM.
- No on-device pan/zoom — the viewport is driven by the desktop
  ([SRS-EP-02](./features/region-sync/srs-logic.md)).
- Transform = translate + scale. Rotation is reserved, never set
  ([REQ-08](./prd.md#node-manipulation)).

## Solution strategy

Three layers, ordered by how much the ink budget protects them.

1. **Ink path (untouchable).** Digitizer → Round 19 map → local paint, unchanged from
   [SRS-EP-01](./features/local-pen-ink/srs-logic.md). Nothing added by this rework may sit between
   a sample and its pixel.
2. **Document layer (new).** At pen-up the stroke becomes a node in a `DeviceDocument`
   ([SRS-EP-07](./features/device-document/srs-logic.md)), the tool decides what happens next
   ([SRS-EP-10](./features/ink-box/srs-logic.md)), manipulation transforms real ink
   ([SRS-EP-11](./features/ink-box/srs-logic.md)), and every structural op pushes one undo entry and
   one queue entry. The panel paints from this tree — never from a peer picture.
3. **Sync layer (inverted).** Inbound is `viewport` plus exactly one handshake-gated `doc_load` per
   epoch; outbound is an ordered `doc_change` stream plus advisory stroke previews
   ([SRS-EP-08](./features/device-document/srs-logic.md),
   [ADR-0015](../../adr/ADR-0015-one-way-sync-contract.md)).

The strategy is deliberately boring where it can be: the undo ring is whole-document snapshots
(depth 20) rather than inverse-op algebra, and the change stream is one op per gesture rather than a
diff format. Both choices buy correctness with memory and bytes, which is the right trade on a
device whose scarce resource is *latency*, not storage.

## Domain entities (consumed)

| Entity | Canonical doc | Notes |
|---|---|---|
| Vector document (Ink, Text, Primitive, Group, Frame, Connector, SmartGroup) | [domain/vector-document](../../domain/vector-document.md) | Link only — the device implements these semantics, it does not redefine them |

Device-authored kinds this iter: `Ink`, `SmartGroup`. Other kinds arrive via `doc_load` and must
round-trip and paint; unknown kinds are carried opaquely so a device session cannot make the
desktop's file lossy.

Device-local, never shared: tool mode, selection, undo ring, publish queue, session epoch
([SRS-EP-09](./features/device-document/srs-data.md)).

## Context view

```mermaid
flowchart LR
  artist["Artist with pen"] --> epaper["Epaper — owns the working document"]
  epaper -->|"doc_change (ordered ops)"| infini["Infini — mirror, file, viewport"]
  epaper -->|"stroke_* preview (advisory)"| infini
  infini -->|"viewport (continuous)"| epaper
  infini -->|"doc_load (once per epoch)"| epaper
```

## Container / component view

```mermaid
flowchart TB
  subgraph device["Epaper Qt"]
    input["TabletAppFilter — pen + touch"]
    ink["Local ink + Round 19 map — SRS-EP-01"]
    tools["Tool state — SRS-EP-04"]
    doc["DeviceDocument + undo ring — SRS-EP-07"]
    recog["Recognition + membership — SRS-EP-10"]
    manip["Hit-test + transforms — SRS-EP-11"]
    paint["Rasterize from document — SRS-EP-02"]
    sync["Session: queue, publisher, load handshake — SRS-EP-08"]
    input --> ink
    input --> tools
    ink --> doc
    tools --> recog
    tools --> manip
    recog --> doc
    manip --> doc
    doc --> paint
    doc --> sync
  end
  sync <-->|"TCP JSON-lines :9877"| infini["Infini"]
```

The single arrow worth staring at is `doc --> paint`. In the pilot that arrow came from the wire.

## Crosscutting concepts

- **Authority:** one writer per session and it is this device
  ([ADR-0014](../../adr/ADR-0014-document-ownership-inversion.md) §1). A peer message may never
  alter the document except an accepted `doc_load`.
- **Consistency:** local-first. Ops commit locally, then publish; the queue preserves order and is
  never coalesced, because the undo unit and the change unit are the same gesture.
- **Degradation:** the link going down removes *publishing*, not *function*. Nothing in the tool
  chip may be gated on the session ([SRS-EP-05](./features/tool-modes/srs-ui.md)).
- **Refresh:** map is never coalesced, paint is. Ghosting is a timing allowance, never a content
  allowance — a settled frame that disagrees with the document is a defect.
- **Error posture:** retired message types are rejected, logged, and surfaced rather than ignored.
  The pilot's extra snapshots were tolerated silently for weeks, and silence is what made them
  expensive.
- **Observability:** message-type counts at the transport make the "0 inbound document messages"
  invariant checkable; op-commit timestamps drive the publish-latency budget. Instrumentation stays
  out of the paint loop.

## Decisions

- [ADR-0014](../../adr/ADR-0014-document-ownership-inversion.md) — the device owns the working document
- [ADR-0015](../../adr/ADR-0015-one-way-sync-contract.md) — one-way sync contract v1
- [ADR-0013](../../adr/ADR-0013-ink-box-tool-modes.md) — §1 device-local tool state and §6 world-unit enclose guard survive; §2–§5 superseded
- [ADR-0011](../../adr/ADR-0011-smart-group.md) — Smart Group semantics (host moved, meaning unchanged)
- [ADR-0010](../../adr/ADR-0010-tree-of-vectors.md) — tree-of-vectors document
- [ADR-0012](../../adr/ADR-0012-world-stroke-viewport-parity.md) — world stroke width + viewport paint parity
- `ADR-0016` (deferred) — node manipulation model, constrained by the capability descriptor in
  [node-manipulation srs-product](./features/node-manipulation/srs-product.md)

## Risks & technical debt

| Risk | Threatens | Likelihood × impact | Mitigation / accepted |
|---|---|---|---|
| Document + hit-test cannot fit under the ≤30 ms ink budget | Quality goal 1 — **invalidates the rework** | M×H | Measure before the first REQ-04 story; a miss is a `CHL-*`, not a design workaround |
| C++/TS geometry divergence | Document fidelity | M×H | Shared fixtures (`ops/`, `enclose/`, `fixed-ink/`, `round-trip/`) + the domain doc |
| Undo ring memory (20 whole-tree snapshots) | Ink latency | M×M | Measured in [SRS-EP-13](./features/device-document/srs-quality.md); shrink the ring before slowing ink |
| Live manipulation exceeds the partial-refresh budget | Gesture feel | M×M | ≥5 Hz / 0 full-panel bar; CHL-0006 established that slow is acceptable |
| Device constants inherited from desktop values (LOD 0.35, 8 px tolerance) | Manipulation usability | **H×M** | **Closed.** Device locks: handle 28/56 du, LOD min on-panel axis 96 du ([SRS-EP-12](./features/ink-box/srs-ui.md)). Miss-rate on hardware files a `CHL-*` in du, never 8 CSS px / 0.35 |
| No undo affordance on a three-tool chip | Recoverability | H×M | **Deferred this campaign ([CHL-0010](../../../.plan/iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md)).** Undo logic ships regardless ([SRS-EP-07](./features/device-document/srs-logic.md) / EP-015) |
| Unpublished work lost on app restart | Data loss | L×M (accepted) | Publish per committed op + visible pending state |
| RM2 touch unreachable from Qt | Tool arming | M×H | Spike shipped; fallback is pen-on-chip |
| `regionsync/` library still unwired from the Qt binary | Dual path confusion | H×M | Wire it as the document layer lands, or retire it |
