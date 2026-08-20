---
id: ADR-0029
title: Independent cameras + optional one-way viewport follow
status: accepted
date: 2026-08-20
deciders: [architect, pm]
supersedes: [ADR-0023]
amends: [ADR-0009, ADR-0015]
source: TRACK-005 / Epaper [REQ-19] / Infini [REQ-06]
---

# ADR-0029 — Independent cameras + optional one-way viewport follow

## Context

Human 2026-08-20 and [BRD-07](../brd.md#brd-07-infinity-canvas--tablet-sync-infini--epaper): Epaper and Infini cameras are **independent by default**. Always-on Infini→tablet viewport drive is obsolete. Optional **follow** is mutually exclusive, off on disconnect, and not restored on reconnect.

[ADR-0023](./ADR-0023-viewport-last-writer.md) made the viewport channel a **last-writer token** (idle Infini; tablet claims; Infini steals). That is **not** the product model. Last-writer steal is how cameras fight. This ADR supersedes it.

Document channel stays one-way ([ADR-0014](./ADR-0014-document-ownership-inversion.md), [ADR-0015](./ADR-0015-one-way-sync-contract.md) §1–§4, §6). Viewport payload shape is unchanged. Infini→Infini follow is a Non-Goal.

Quality goals at stake:

| Goal | Bar |
|---|---|
| Map freshness while following | Next pen sample uses the leader map, p95 ≤100 ms ([Epaper REQ-02](../modules/epaper/prd.md#region-sync)) |
| Independent default | Follow off → 0 peer-driven region changes |
| Follow exclusivity | **0** intervals where both follows are on |
| Disconnect | Follow **off** before the next gesture; reconnect does not restore |
| 0 camera fight | Follower local-nav turns **that** follow off (PM completion, accepted) |
| Document contract | Viewport never carries document ops; 0 extra `doc_*` from follow |

## Decision

Each connected peer owns **its own camera** (`translate` + uniform `scale` + `drawingRegion`). Follow is **optional session state**, not the session, not a document field, not a last-writer token.

Anatomy: [domain/viewport-follow](../domain/viewport-follow.md). Behaviour: [SRS-EP-49](../modules/epaper/features/region-sync/srs-logic.md#srs-ep-49-viewport-follow) / [SRS-IN-26](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-26-viewport-follow).

| Rule | Value |
|---|---|
| Default | `direction = none`. Both cameras local. **0** `viewport` messages either way |
| Exactly one direction | `infini_to_epaper` (Epaper follows Infini) **xor** `epaper_to_infini` (Infini follows Epaper). Enabling one **sets** the enum (the other cannot stay on) |
| Viewport on the wire | **Only along the active follow direction.** Same payload as today (`translate`, `scale`, `drawingRegion`, `orientation`, `settle`, `seq`). Add `source: infini \| epaper` |
| Apply | The **follower** applies inbound `viewport` immediately to its map. The leader and a peer with follow off **ignore** inbound `viewport` (log; do not apply; do **not** treat arrival as implicit follow-on) |
| Control plane | Session message `viewport_follow` `{ direction, seq }` on TCP `:9877`. Latest `seq` wins. **Not** a document message |
| Disconnect | Both peers force `none` locally. No restore on reconnect (hello does not carry last follow) |
| Follower local-nav | A navigation gesture on the **follower** sets `none`, then drives that peer’s local camera. **0** continued apply of the leader after that gesture starts. Box pick/move/resize is **not** local-nav |
| Leader local-nav | Publishes along the active direction (coalesce ≤30 Hz; `settle: true` on flush) |
| Link down | Local cameras still work; follow is already `none` |
| Document channel | Unchanged. Follow never emits `doc_load` / `doc_change` / `doc_snapshot` |
| Chrome | Icon toggle on **both** peers. **Not** a ToolChip exclusive tool, recognizer, or hand-tool tile |

`viewportOwner` / last-writer steal / 150 ms token release are **withdrawn**.

## Consequences

- Amends [ADR-0009](./ADR-0009-shared-document-viewport.md): Infini is **not** the sole owner of translate + uniform scale. Each peer owns its camera; the viewport **channel** exists only while follow is on.
- Amends [ADR-0015](./ADR-0015-one-way-sync-contract.md) §5 and §7: `viewport` is **not** always D→T continuous. Direction follows `viewportFollow.direction`. `viewport_follow` is an allowed session type (sibling to `pen_button_map` — not document, not debug).
- [SRS-EP-02](../modules/epaper/features/region-sync/srs-logic.md) remains inbound **map apply**, now **gated** by Epaper follow. [SRS-EP-24](../modules/epaper/features/region-sync/srs-logic.md) remains tablet gesture → local map; publish **only if Infini follow is on**.
- New parents for toggle behaviour: [SRS-EP-49](../modules/epaper/features/region-sync/srs-logic.md#srs-ep-49-viewport-follow) / [SRS-IN-26](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-26-viewport-follow). Do not parent REQ-19 / Infini REQ-06 on the old mapping sections.
- Sensitivity: **0 dual-on** vs **toggle latency**. The enum makes dual-on impossible; p95 ≤300 ms is the comfort bar for the peer’s follow turning off.
- Trade-off point: **independent cameras (no fight)** vs **one shared picture**. Follow is opt-in; follower nav dropping follow is how they do not fight. Last-writer steal is **not** restored.
- One-finger empty pan vs palm is a **documented millimetre threshold** in [SRS-EP-21](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger), not this ADR.

## Alternatives Considered

| Approach | Independent default | 0 fights | Opt-in match | Doc one-way | Why |
|---|---|---|---|---|---|
| Status quo ADR-0023 last-writer | − | 0 (steal) | − | + | Rejected — not the product model; steal restores a shared camera |
| Always-on Infini writer (ADR-0009 / ADR-0015 §5 as written) | − | + | − | + | Rejected — obsolete always-on drive |
| Two cameras, no follow | + | + | − | + | Rejected — fails [REQ-19](../modules/epaper/prd.md#viewport-follow) / Infini [REQ-06](../modules/infini/prd.md#viewport-follow) |
| Bidirectional viewport always | − | − | n/a | + | Rejected — cameras fight; dual apply |
| Exclusive lock RPC before pan | 0 | + | 0 | + | Rejected — round trip on the map path; fails link-down |
| **Independent + exclusive one-way follow (this ADR)** | + | + | + | + | Winner |

Trade-off point: **who may write viewport** is replaced by **whether anyone is following**. The document channel is not the place to hang this.
