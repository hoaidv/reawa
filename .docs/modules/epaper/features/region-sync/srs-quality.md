---
feature: region-sync
parent_req: [REQ-02, REQ-10, REQ-19]
version: 0.8.0
lifecycle: active
---

# SRS — Region sync Epaper (Quality)

Parent REQ: [REQ-02](../../prd.md#region-sync). Follow quality: [SRS-EP-51](#srs-ep-51-follow-quality).

## [SRS-EP-03] Map-before-refresh, coalesce, and stroke fidelity

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014. Parity is device-document vs desktop-mirror, not
     device raster vs pushed snapshot. Same id, content revised. -->

> **Revised 2026-08-13.** The parity row used to compare the device raster against a pushed
> `doc_snapshot` — it measured how well the device copied the desktop. There is nothing to copy now.
> Parity is measured in the other direction: does the **mirror** match the device's document
> ([SRS-IN-08](../../../infini/features/tablet-sync/srs-quality.md)). Map, coalesce, and stroke
> fidelity rows are unchanged and remain the floor.

| Scenario | Metric | Target |
|---|---|---|
| Viewport received **while Epaper follow is on** → next pen sample world mapping | Must use new viewport + gut UV | Always |
| Map apply latency after viewport on wire **while following** | p95 | ≤ 100 ms (align SRS-IN-08) |
| Infini pans ≥5 s **while Epaper follow is off** | Infini-driven region changes | **0**; next pen uses local camera |
| **Panel raster vs the device's own document** | Same figures for the region after settle | Always ([ADR-0014](../../../../adr/ADR-0014-document-ownership-inversion.md) §2) |
| **Panel raster vs desktop mirror** | Same figures after the change stream settles | Always — convergence measured on the desktop ([SRS-IN-08](../../../infini/features/tablet-sync/srs-quality.md)) |
| Soft refresh under pan/zoom spam | GUI-thread vector FullClear of the camera | **0** on pan/zoom (blit + LatestJob); latest pending wins, in-flight is allowed to finish |
| Soft pan/zoom preview | Reuse previous panel bitmap (pan shift / zoom scale-blit); ghosting OK | Always — not a FullClear |
| Camera sharpen | LatestJob vector of the current camera; latest pending wins; deliver on GUI when not inking | Mid-gesture when a job finishes; always at settle |
| Unchanged camera (`cam=none`) | Blit | **0**; still submit sharpen if the preview is a blit |
| Newly revealed pan strips / zoom-in AA | Present after the matching LatestJob delivers | Always after settle; also mid-gesture when a job completes |
| Settle / accepted `doc_load` | Sharp paint | Job delivers AA vector; no soft fade left behind once idle |
| Ordinary local `append_ink` | Document FullClear / InPlaceDirty on that pen-up | **0** — live stamps are settle ([CHL-0029](../../../../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)); camera job must not swap `m_image` during a stroke ([SRS-EP-01](../local-pen-ink/srs-logic.md)) |
| Structural local op | Sharp paint of the changed AABB | InPlaceDirty (or FullClear if AABB missing/huge) |
| **Repaints sourced from an inbound peer picture** | Count | **0** |
| Local ink → wire | `stroke_*` (preview) with world brush + **world** x/y | Always |
| Stroke width | Live + vector: `world × s_panel` | Always ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)) |
| Zoom parity | Grow region world width (zoom out) | Panel stroke px shrinks for same world width (±5%) |
| Hot path | Socket I/O must not block pen sample callback | Always |
| Bitmap `region_refresh` | Ignored | Always |

### Notes

- **Map** is never coalesced; **paint** is. Ghosting between paints is accepted.
- Ghosting is a *timing* allowance, never a *content* allowance: a settled frame that disagrees with
  the local document is a defect, not slow e-ink.
- **Soft camera** may blit the previous `m_image` (pan translate, zoom scale). That is the
  interactive path. Vector paint of on-camera ink runs on a **LatestJob** worker (one in-flight,
  one pending). Unlike object-erase overlay, a newer camera **does not** abort in-flight —
  the finished buffer is warped toward the current camera, then the pending job runs.
  The GUI swaps when the job matches (or warps a slightly stale result). It does not
  FullClear on the pointer stack.
- Newly revealed strips and zoom-in AA are **not** guaranteed on the blit frame; they appear when
  the job for that camera delivers. Settle always leaves a job in flight for the final camera.
- **Ordinary ink settle** is live stamps, not a full-panel sharp paint
  ([CHL-0029](../../../../../.plan/iter-005/challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)).
  A FullClear that agrees with the document still **fails SRS-EP-01** if it queues the next pen-down.
- Field 2026-08-31 (blit + LatestJob, ink skip, connector punch):
  [memory/2026-08-31-field-latency.md](../../../memory/2026-08-31-field-latency.md),
  [memory/camera-pan-zoom-rasterize.md](../../../memory/camera-pan-zoom-rasterize.md).
- Library `RegionSession` coalesce semantics remain the target for a future Qt wiring story.

---

## [SRS-EP-26] Two-finger map-apply quality {#srs-ep-26-two-finger-quality}

<!-- lifecycle: active -->
<!-- revised: 2026-08-20 — ADR-0029. Local Must; 0 divergent viewports only while Infini following. -->

**Parent:** [REQ-10](../../prd.md#hand-touch). **Constrains:** [SRS-EP-24](./srs-logic.md#srs-ep-24-two-finger-viewport). Does **not** steal [SRS-EP-03](#srs-ep-03) parent (REQ-02). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md).

| Scenario | Metric | Target |
|---|---|---|
| Two-finger pan/pinch ≥5 s → next pen sample map | p95 map apply | ≤**100 ms** |
| After settle, Infini follow **on** | Divergent viewports | **0** |
| Infini follow **off**, tablet pans | Infini canvas change from that gesture | **0**; **0** viewport up |
| Epaper follow **on**, two-finger starts | Two-finger | **blocked**; follow stays on; **0** local camera change |
| Link down, two-finger | Local map still updates | Always |

---

## [SRS-EP-51] Viewport-follow Infini quality {#srs-ep-51-follow-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-19](../../prd.md#viewport-follow). **Constrains:** [SRS-EP-49](./srs-logic.md#srs-ep-49-viewport-follow). Does **not** steal [SRS-EP-03](#srs-ep-03) (REQ-02) or [SRS-EP-26](#srs-ep-26-two-finger-quality) (REQ-10).

| Field | Value |
|---|---|
| Source | Creator toggle / disconnect |
| Stimulus | Enable / disable / peer enable / drop / pan-while-following (ignored on Epaper) |
| Artifact | `follow.direction`, inbound map apply, peer toggle |
| Environment | Session live or lost |
| Response | Exclusive follow or both off; map tracks leader only while following |
| Response measure | See table |

| Scenario | Target |
|---|---|
| Enable Epaper follow from both-off | Map apply p95 ≤**100 ms**; Infini follow stays off |
| Enable Epaper follow while Infini follow on | **0** intervals both on; Infini follow off p95 ≤**300 ms** |
| Dual-on (any connected interval) | Count **0** |
| Disconnect while following | Follow **off** before next gesture; reconnect stays **off** until opt-in |
| No session | Toggle off or unavailable; **0** follow-on persist |
| `viewport_follow` / follow toggle | **0** `doc_load` / `doc_change` / `doc_snapshot` |
| Follower pan/pinch while following Infini | **0** camera change; follow **stays on**; box pick/move/resize still runs |

---

## Superseded

Prior “production `append_ink` + RegionSession owns map” wording is **target architecture**;
**shipped** path is documented above (0.4.0, 2026-08-11).
SRS-EP-26 is additive (REQ-10); it does not supersede SRS-EP-03.
