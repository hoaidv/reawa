---
feature: infinity-canvas
parent_req: [REQ-01]
version: 0.1.0
lifecycle: active
---

# SRS — Infinity canvas (Quality)

## [SRS-IN-03] Gesture frame budget

| Scenario | Metric | Target | Measurement |
|---|---|---|---|
| Continuous trackpad pan 5 s | Dropped frames on 60 Hz display | ≤ 2 / s perceived; prefer vsync-aligned | Manual + optional `requestAnimationFrame` counter |
| Pinch zoom 5 s | Same | Same | Manual |
| Mixed pan+zoom | Transform remains translate+uniform scale | Always | Visual: circle stays circular |

---

## [SRS-IN-22] Tablet-viewport apply quality {#srs-in-22-follow-quality}

<!-- lifecycle: active -->
<!-- revised: 2026-08-20 — ADR-0029. Apply-while-following; not last-writer. -->

**Parent:** [REQ-01](../../prd.md#infinity-canvas). **Constrains:** [SRS-IN-20](./srs-logic.md#srs-in-20-follow-viewport), [SRS-IN-21](../tablet-sync/srs-logic.md#srs-in-21-viewport-token). Does **not** steal [SRS-IN-03](#srs-in-03) parent (REQ-01 gesture budget). **Does not parent [REQ-06](../../prd.md#viewport-follow)** — that is [SRS-IN-28](../tablet-sync/srs-quality.md#srs-in-28-follow-quality). **Decision:** [ADR-0029](../../../../adr/ADR-0029-independent-cameras-viewport-follow.md).

| Scenario | Target |
|---|---|
| Infini follow **on**, tablet pan after settle | Infini view matches tablet region (**0** divergent viewports) |
| Infini follow **off**, tablet pans | Infini canvas change from that gesture **0**; **0** competing `viewport` down from a stolen token (token withdrawn) |
| Infini follow **on**, Infini local-nav starts | Infini follow **off** before local pan applies; **0** continued tablet apply |
| Infini frame budget during follow | Must not regress [SRS-IN-03](#srs-in-03) when Infini is **not** gesturing |
