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

## [SRS-IN-22] Tablet-follow viewport quality {#srs-in-22-follow-quality}

<!-- lifecycle: active -->

**Parent:** Epaper [REQ-10](../../../epaper/prd.md#hand-touch). **Constrains:** [SRS-IN-20](./srs-logic.md#srs-in-20-follow-viewport), [SRS-IN-21](../tablet-sync/srs-logic.md#srs-in-21-viewport-token). Does **not** steal [SRS-IN-03](#srs-in-03) parent (Infini REQ-01).

| Scenario | Target |
|---|---|
| Tablet pan while both idle | Infini sends **0** competing `viewport` bursts |
| After tablet settle | Infini view matches tablet region (**0** divergent viewports) |
| Infini frame budget during follow | Must not regress [SRS-IN-03](#srs-in-03) when Infini is **not** gesturing |
