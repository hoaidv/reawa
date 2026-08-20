---
feature: region-sync
parent_req: [REQ-02, REQ-10, REQ-19]
version: 0.6.0
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
| Soft refresh under pan/zoom spam | Min interval between soft paints | ≥ **250 ms**; latest pending wins |
| Settle / accepted `doc_load` / committed local op | Sharp paint | Immediate; AA on; no soft fade left behind |
| **Repaints sourced from an inbound peer picture** | Count | **0** |
| Local ink → wire | `stroke_*` (preview) with world brush + panel x/y | Always |
| Stroke width | Live + vector: `world × s_panel` | Always ([ADR-0012](../../../../adr/ADR-0012-world-stroke-viewport-parity.md)) |
| Zoom parity | Grow region world width (zoom out) | Panel stroke px shrinks for same world width (±5%) |
| Hot path | Socket I/O must not block pen sample callback | Always |
| Bitmap `region_refresh` | Ignored | Always |

### Notes

- **Map** is never coalesced; **paint** is. Ghosting between paints is accepted.
- Ghosting is a *timing* allowance, never a *content* allowance: a settled frame that disagrees with
  the local document is a defect, not slow e-ink.
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
| Epaper follow **on**, two-finger starts | Epaper follow | **off** before local pan applies |
| Link down, two-finger | Local map still updates | Always |

---

## [SRS-EP-51] Viewport-follow Infini quality {#srs-ep-51-follow-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-19](../../prd.md#viewport-follow). **Constrains:** [SRS-EP-49](./srs-logic.md#srs-ep-49-viewport-follow). Does **not** steal [SRS-EP-03](#srs-ep-03) (REQ-02) or [SRS-EP-26](#srs-ep-26-two-finger-quality) (REQ-10).

| Field | Value |
|---|---|
| Source | Creator toggle / disconnect / follower local-nav |
| Stimulus | Enable / disable / peer enable / drop / pan while following |
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
| Follower local-nav (empty pan > **20 mm** or two-finger) while following Infini | Follow **off** before local pan applies; **0** continued Infini apply after that gesture starts |

---

## Superseded

Prior “production `append_ink` + RegionSession owns map” wording is **target architecture**;
**shipped** path is documented above (0.4.0, 2026-08-11).
SRS-EP-26 is additive (REQ-10); it does not supersede SRS-EP-03.
