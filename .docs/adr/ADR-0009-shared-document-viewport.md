---
id: ADR-0009
title: Shared document + viewport channel for Infini↔Epaper consistency
status: accepted
date: 2026-08-10
deciders: [architect, pm]
supersedes: null
---

# ADR-0009 — Shared document + viewport channel for Infini↔Epaper consistency

## Context

[REQ-03](../modules/infini/prd.md#tablet-sync) / Epaper
[REQ-02](../modules/epaper/prd.md#region-sync) require: (1) Epaper local ink,
(2) vectors appear on Infini, (3) Infini pan/zoom updates Epaper’s drawing region,
(4) Epaper must not render a **different document** than Infini for that region,
(5) full e-paper refresh may lag, but **mapping** for the next pen sample must be fresh.

Pushing bitmaps RM↔desktop fails latency (EXP lesson). Keeping two independent editors
in sync by ad-hoc patches risks silent divergence.

## Decision

Split the session into **two channels** over one transport:

1. **Document channel (authoritative vector log)**  
   Both peers apply the same append-only **stroke/ops log** (in-memory model from
   [REQ-02](../modules/infini/prd.md#vector-document)). Epaper appends local strokes;
   Infini appends only if/when it gains edit tools (v0: Infini is mostly viewer +
   viewport). Persistence (SVG) is a serialization of that model, not a second source
   of truth during a live session.

2. **Viewport channel (drawing region)**  
   Infini owns `translate + uniform scale` (and thus the axis-aligned drawing region
   in world space). Epaper applies viewport updates **immediately** to its input→world
   map and ink transform, then schedules an e-paper refresh of that region **asynchronously**.
   Ghosting / stale pixels are allowed; wrong mapping is not.

**Same picture rule:** a panel refresh paints from the **current document snapshot**
clipped to the **current viewport**. If document and viewport are both applied in
order, Epaper and Infini cannot disagree on content for that region — they may disagree
temporarily on *how freshly the panel was refreshed*.

Do **not** require continuous full-document mirror cost on every pen sample: stream
ops; refresh raster when the device can.

## Consequences

- Implementers must version ops and define idempotent apply on both Qt and Electron sides.
- Viewport messages are small and high-priority; document ops may batch.
- Conflict if Infini later edits the same stroke Epaper is drawing — out of v0 scope
  (Infini non-destructive viewer during draw).
- Mitigates “expensive always-in-sync documents”: sync the **log**, not two mutable DOMs
  with bidirectional dirty trees.

## Alternatives Considered

| Approach | Latency | Consistency | Cost | Notes |
|---|---|---|---|---|
| **Shared op-log + viewport channel** | + | + | 0 | Chosen (target) |
| Dual independent docs + periodic full resync | − | − | + cheap until desync | Rejected — violates “never render differently” |
| Infini-authoritative bitmap push to RM | − | + pixels | − bandwidth | Rejected — kills local ink feel |
| CRDT full generality | 0 | + | − complexity | Deferred until multi-editor needs appear |

## Amendments

### 2026-08-11 — Interim wire matches shipped code

Production Infini↔Epaper today uses:

| Channel | Shipped messages |
|---|---|
| Viewport | `viewport` (+ `orientation`, `settle`, `drawingRegion`) |
| Document picture | one-shot / rare `doc_snapshot` (WorldLayer nodes) |
| Ink stream | Epaper→Infini `stroke_begin|point|end` (panel coords; world brush width) |

Bitmap `region_refresh` is **rejected** on device. Live bidirectional `doc_op` /
`append_ink` remains the **target** shape of this ADR and is exercised in unit tests /
`regionsync/` headers, but is **not** the Qt/Electron live path yet.

Same-picture rule still holds: Epaper paints from its local vector node list ∩ current
`drawingRegion` after applying viewport; Infini shows the same WorldLayer content for
that AABB. Migration to a pure op-log does not change the rule — only the encoding.

### 2026-08-20 — Independent cameras; follow-gated viewport ([ADR-0029](./ADR-0029-independent-cameras-viewport-follow.md))

The Decision’s “Infini owns `translate + uniform scale`” clause is **amended**, not rewritten:
each peer owns **its** camera by default. Viewport messages flow **only while follow is on**,
and **only along the active follow direction**. Document channel unchanged. Last-writer
([ADR-0023](./ADR-0023-viewport-last-writer.md)) is superseded. Same-picture for a region still
holds **when the follower is applying the leader’s `drawingRegion`**; while follow is off the
two cameras may show different regions of the same document (that is the product).
