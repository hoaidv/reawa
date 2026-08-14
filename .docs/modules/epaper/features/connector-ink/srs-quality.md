---
feature: connector-ink
parent_req: [REQ-09]
version: 0.1.0
lifecycle: active
---

# SRS — Connector-ink (Quality)

Measurable bars for [REQ-09](../../prd.md#device-connectors).
Does not relax [SRS-EP-01](../local-pen-ink/srs-logic.md) ink latency or
[SRS-EP-14](../ink-box/srs-quality.md) enclose bars.

## Prioritised quality goals

1. **Ink latency** — recognizer path must not move pen-down → pixel p95 above 30 ms.
2. **Gesture truth** — live warp committed geometry = last previewed; 0 px jump.
3. **Mirror parity** — 0 divergent connector nodes (REQ-07).
4. **Ship gate** — default-on false positives ≤2%.

## [SRS-EP-20] Connector recognition, warp, and ship gate {#srs-ep-20-connector-quality}

| Field | Value |
|---|---|
| Source | Creator pen-up / bound-node drag / corpus replay |
| Stimulus | Recognize, warp, or default-on recognizers |
| Artifact | Connector node + panel + Infini mirror |
| Environment | Device in-session; host fixtures for geometry |
| Response | Verdict + visible geometry + log line |
| Response measure | See table |

| Scenario | Target |
|---|---|
| Pen-up → connector visible | p95 ≤500 ms; 0 peer messages |
| Re-warp during drag | ≥5 Hz; 0 full-panel invalidations; UI freeze ≤200 ms |
| Commit vs last preview | 0 px jump @ 100% zoom |
| Never re-bake round-trip | 0.000 u on host fixtures (ADR-0020 I1) |
| Device vs Infini samples | 0 divergent nodes on shared fixtures |
| False positives (both armed) | ≤2% of `pen` strokes; 0 on a fresh page's first 20 unless named |
| Recall intended UX1/UX2 | ≥90% |
| EP-016 / EP-017 under new dispatch | 0 changed verdicts except deliberate D21 fall-through |

### State coverage (dual-ask)

| State id | Designer | QA |
|---|---|---|
| `conn.blink` | required | required |
| `conn.selected` | required | required |
| `conn.rejected` | required | required |
| `conn.live_warp` | required | required |
| `conn.orphan` | required | required |

## Non-goals

- Pixel-match the EXP contact sheet
- Shipping default-on before G1/G2 pass
