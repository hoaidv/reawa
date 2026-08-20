---
id: ADR-0023
title: Viewport last-writer token (tablet vs Infini)
status: superseded
date: 2026-08-19
deciders: [architect, pm]
supersedes: null
superseded-by: [ADR-0029]
amends: [ADR-0009, ADR-0015]
source: TRACK-005 / [REQ-10]
---

# ADR-0023 — Viewport last-writer token (tablet vs Infini)

> **Superseded 2026-08-20** by [ADR-0029](./ADR-0029-independent-cameras-viewport-follow.md).
> Last-writer is **not** the product model. Do not implement this token. Forward: independent cameras + optional exclusive one-way follow.

## Context

[ADR-0009](./ADR-0009-shared-document-viewport.md) and [ADR-0015](./ADR-0015-one-way-sync-contract.md) §5 make **Infini the sole writer** of the viewport channel (`viewport` down, ≤30 Hz, latest wins). [Epaper REQ-10](../modules/epaper/prd.md#hand-touch) two-finger pan/pinch now requires the **tablet** to change the same viewport Infini uses and **publish** so the desktop follows, with **0 competing viewport bursts** while the tablet gesture is in flight. Infini Non-Goals keep Infini a navigator; they do not keep it the exclusive writer during a tablet gesture.

Quality goals at stake:

| Goal | Bar |
|---|---|
| Map freshness | Next pen sample uses the new map, p95 ≤100 ms ([REQ-10](../modules/epaper/prd.md#hand-touch), [SRS-EP-03](../modules/epaper/features/region-sync/srs-quality.md)) |
| One picture | After settle, Infini view = tablet drawing region (0 divergent viewports) |
| Ink path | Two-finger must not sit between a pen sample and its pixel ([SRS-EP-01](../modules/epaper/features/local-pen-ink/srs-logic.md)) |
| Link down | Local two-finger still updates the device map ([REQ-10](../modules/epaper/prd.md#hand-touch)) |

Ship of the two-finger slice remains blocked on a [BRD-07](../brd.md) amendment; this ADR is the contract that slice will implement.

## Decision

The viewport channel is **still one shared `{translate, uniform scale, orientation, drawingRegion, settle}`**. Ownership is a **last-writer token**, not two independent cameras.

| Rule | Value |
|---|---|
| Idle owner | `infini` — desktop pan/pinch publishes `viewport` down as today |
| Tablet two-finger start | Device **claims** `viewportOwner = epaper`; applies the gesture to its local map **immediately**; publishes `viewport` **up** (same payload shape, `source: epaper`) |
| While `epaper` owns | Infini **applies** inbound tablet viewport to its canvas and **sends 0** `viewport` messages (no competing bursts) |
| Tablet two-finger end | Device flushes `settle: true`, then **releases** the token after **150 ms** with no further two-finger samples |
| Infini claim | A **new** desktop pan/pinch while the token is `epaper` **steals** the token (`infini`); device applies the next down `viewport` as today. Steal is edge-triggered on Infini gesture start, not on pointer idle jitter |
| One-finger box-move in flight | Two-finger does **not** run; token stays with whoever already holds it; **0** viewport pan from the box-move |
| Link down | Device still mutates local map; publish queues or drops with the session; **0** Infini bursts because there is no session |
| Document channel | Unchanged. Viewport never carries document ops ([ADR-0015](./ADR-0015-one-way-sync-contract.md)) |

Token is **session state**, never a document field. It is not persisted.

## Consequences

- Amends ADR-0009 “Infini owns translate + uniform scale” to “Infini is the **default** writer; the gesturing peer is the **current** writer.”
- Amends ADR-0015 §5 “viewport direction is desktop→tablet only” with an **up** `viewport` while `epaper` owns. Message **type** stays `viewport`; direction is now bidirectional under the token. After load, Infini still sends **0 document** messages.
- [SRS-EP-02](../modules/epaper/features/region-sync/srs-logic.md) remains the **map apply** section for *inbound* Infini viewport. New [SRS-EP-24](../modules/epaper/features/region-sync/srs-logic.md) owns tablet gesture → local map + publish. Do not overload SRS-EP-02 as parent of REQ-10.
- Infini follow: [SRS-IN-20](../modules/infini/features/infinity-canvas/srs-logic.md) + [SRS-IN-21](../modules/infini/features/tablet-sync/srs-logic.md).
- Sensitivity: **token steal latency** vs **0 fighting bursts**. 150 ms release is a comfort number; a miss that causes a fight is a defect, a miss that delays Infini grab by one frame is not.
- [CHL-0022](../../.plan/iter-005/challenges/CHL-0022-shipped-no-device-pan.md) records shipped prose that still says there is no on-device pan — PM must adopt those amendments; this ADR does not silently rewrite SRS-EP-04 / SRS-EP-11.

## Alternatives Considered

| Approach | Map freshness | 0 fights | Link-down local pan | Maintainability | Why |
|---|---|---|---|---|---|
| Status quo (Infini-only writer) | + (desktop) | + | − | + | Rejected — fails REQ-10 two-finger publish |
| Two independent cameras + periodic rebase | − | − | + | − | Rejected — divergent viewports after settle |
| Exclusive lock RPC before pan | − | + | − | − | Rejected — round trip on the ink/map path; fails link-down |
| **Last-writer token (this ADR)** | + | + | + | 0 | Winner — same payload, one owner at a time |

Trade-off point: **who may write viewport during a tablet gesture** vs **Infini remaining a navigator**. Token steal lets Infini take over without a second camera.
