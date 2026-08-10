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
