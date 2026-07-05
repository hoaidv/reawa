---
feature: pen-input-relative
parent_req: [REQ-03]
version: 1.0.0
lifecycle: active
---

# SRS — Relative Pen Input (Quality)

## Prioritised quality goals

1. **Responsiveness** — Low pen-to-cursor latency
2. **Robustness** — No cursor teleport after external input interference

## [SRS-RW-34] Pen-to-cursor latency {#responsiveness}

| Field | Value |
|---|---|
| Source | Active SSH pen stream |
| Stimulus | Continuous pen hover movement |
| Artifact | RelativePenDriver → CGEventPost |
| Environment | Single display, connected session |
| Response | Cursor tracks pen without perceptible lag |
| Response measure | p95 frame processing ≤ 16 ms on session thread (design target; profile in future iter) |

Constrains: [SRS-RW-30](srs-logic.md).

## [SRS-RW-35] External cursor interference recovery

| Field | Value |
|---|---|
| Source | User with trackpad during active pen hover gesture |
| Stimulus | Trackpad moves cursor away from gesture-expected position; pen resumes |
| Artifact | RelativePenDriver |
| Environment | Relative mode, connected |
| Response | Cursor continues from live position without jump backward |
| Response measure | 0 visible teleport events in manual test script (10 alternation cycles) |

Constrains: [SRS-RW-31](srs-logic.md).

---

## Superseded

_None yet._
