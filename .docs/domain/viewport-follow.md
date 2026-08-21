---
entity: ViewportFollow
slug: viewport-follow
lifecycle: active
owner: architect
owning_context: session
---

# Viewport follow

## Definition

Session-scoped **optional** coupling of two independent cameras. When follow is on, the **follower** applies the **leader’s** viewport. When follow is off (the default), each peer pans its own camera. Follow is **not** a document field, **not** a last-writer token, and **not** restored across reconnect.

## Anatomy

| Field / part | Type / shape | Notes |
|---|---|---|
| `direction` | `none` \| `infini_to_epaper` \| `epaper_to_infini` | Closed enum. Exactly one value. `none` = both cameras local |
| `seq` | monotonic uint | Control-plane ordering for `viewport_follow` messages; latest wins |
| Viewport payload | `{ translate, scale, drawingRegion, orientation, settle, seq, source }` | Unchanged shape; `source` is `infini` or `epaper`. Flows **only** along `direction` |

`infini_to_epaper` means Epaper follows Infini (viewport **down**). `epaper_to_infini` means Infini follows Epaper (viewport **up**).

## Invariants

- **Mutual exclusion:** the enum cannot be both directions at once. Dual-on count = **0** by construction.
- **Default and disconnect:** `none`. Connection lost forces `none` on both peers **before** the next gesture. Reconnect / `hello` does **not** carry last `direction`.
- **Not persisted.** Never SVG, never `doc_change` / `doc_load`.
- **Apply vs emit:** only the follower applies inbound `viewport`. Only the leader emits `viewport` (≤30 Hz, latest wins, `settle: true` on flush). Inbound `viewport` while `direction` does not name this peer as follower is ignored — it does **not** turn follow on.
- **Follower local-nav:**
  - **Epaper** following Infini: one-finger empty pan past threshold and two-finger pan/pinch are **ignored** (0 camera change; follow stays on). Box pick / move / resize still runs.
  - **Infini** following Epaper: a navigation gesture (trackpad/mouse pan or pinch/zoom) sets `direction = none`, then drives Infini’s local camera.
- Infini→Infini follow is out of this entity.

## Relationships

| Related | Cardinality | Notes |
|---|---|---|
| Session | 1:1 live value | TCP `:9877`; control type `viewport_follow` |
| Vector document | none | Must not appear in SVG / `doc_*` |
| Pen-button map | none | Different settings family ([pen-button-map](./pen-button-map.md)) |
| Viewport token (last-writer) | retired | [ADR-0023](../adr/ADR-0023-viewport-last-writer.md) superseded by [ADR-0029](../adr/ADR-0029-independent-cameras-viewport-follow.md) |

## States

| State | Meaning | Transitions |
|---|---|---|
| `follow.none` | Both cameras local; 0 viewport either way | → `infini_to_epaper` when Epaper toggle on (session live); → `epaper_to_infini` when Infini toggle on |
| `follow.infini_to_epaper` | Epaper applies Infini viewport | → `none` on Epaper toggle off, Epaper local-nav, disconnect; → `epaper_to_infini` when Infini toggle on |
| `follow.epaper_to_infini` | Infini applies Epaper viewport | → `none` on Infini toggle off, Infini local-nav, disconnect; → `infini_to_epaper` when Epaper toggle on |

No session / connection lost: force `follow.none`. Toggle unavailable or shown off.

## Non-goals / out of scope

- Icon placement and visual craft (Designer / [SRS-EP-50](../modules/epaper/features/region-sync/srs-ui.md) / [SRS-IN-27](../modules/infini/features/tablet-sync/srs-ui.md))
- One-finger pan vs palm millimetre threshold ([SRS-EP-21](../modules/epaper/features/ink-box/srs-logic.md#srs-ep-21-one-finger))
- Document sync, load handshake, preview strokes
- Infini following another Infini

## Linked modules / features

| Module | Feature / SRS | Role |
|---|---|---|
| `epaper` | [SRS-EP-49](../modules/epaper/features/region-sync/srs-logic.md#srs-ep-49-viewport-follow) / [SRS-EP-50](../modules/epaper/features/region-sync/srs-ui.md) / [SRS-EP-51](../modules/epaper/features/region-sync/srs-quality.md) | Toggle + apply Infini map while following |
| `epaper` | [SRS-EP-02](../modules/epaper/features/region-sync/srs-logic.md) | Map apply (gated) |
| `epaper` | [SRS-EP-24](../modules/epaper/features/region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) | Local two-finger; publish only if Infini following |
| `infini` | [SRS-IN-26](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-26-viewport-follow) / [SRS-IN-27](../modules/infini/features/tablet-sync/srs-ui.md) / [SRS-IN-28](../modules/infini/features/tablet-sync/srs-quality.md) | Toggle + session enum |
| `infini` | [SRS-IN-20](../modules/infini/features/infinity-canvas/srs-logic.md#srs-in-20-follow-viewport) | Apply tablet viewport while Infini following |
| `infini` | [SRS-IN-21](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-21-viewport-token) | Session emit/apply gates |
