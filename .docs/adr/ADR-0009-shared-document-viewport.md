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
| **Shared op-log + viewport channel** | + | + | 0 | Chosen |
| Dual independent docs + periodic full resync | − | − | + cheap until desync | Rejected — violates “never render differently” |
| Infini-authoritative bitmap push to RM | − | + pixels | − bandwidth | Rejected — kills local ink feel |
| CRDT full generality | 0 | + | − complexity | Deferred until multi-editor needs appear |
