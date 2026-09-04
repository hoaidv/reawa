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
| Re-warp during drag | ≥5 Hz; 0 full-panel invalidations; UI freeze ≤200 ms; origin spine **gone** from CanvasLayer (not only the box) |
| Commit vs last preview | 0 px jump @ 100% zoom; **0** leftover origin connector pixels; **0** missing middle of the new spine |
| Never re-bake round-trip | 0.000 u on host fixtures (ADR-0020 I1) |
| Device vs Infini samples | 0 divergent nodes on shared fixtures |
| False positives (both armed) | ≤2% of `pen` strokes; 0 on a fresh page's first 20 unless named |
| Recall intended UX1/UX2 | ≥90% |
| EP-016 / EP-017 under new dispatch | 0 changed verdicts except deliberate D21 fall-through |

---

## [SRS-EP-37] Endpoint style and endpoint-ink quality {#srs-ep-37-endpoint-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-13](../../prd.md#connector-ends). **Constrains:** [SRS-EP-34](./srs-logic.md#srs-ep-34-end-styles), [SRS-EP-35](./srs-logic.md#srs-ep-35-endpoint-ink). Does **not** steal [SRS-EP-20](#srs-ep-20-connector-quality) parent (REQ-09).

| Scenario | Target |
|---|---|
| Endpoint-ink over an end (Path B) | **0** new free Ink there; **0** second connector; stored `{n, e}` and drawn leave survive bound-node transform; world paint follows re-warped leave (**0** orphaned samples) |
| Append on same end | List grows; one undo peels **last** stroke only |
| Same stroke on spine, empty, or recog off | **0** endpoint-ink steals |
| Brush/object/area on ticks only | Decoration clipped or gone; connector **remains** |
| Unintended endpoint-ink on writing corpus | Counts toward ≤**2%** false-positive ship gate with REQ-09 recognizers |
| Path A toolbar on Epaper | **0** (not this campaign) |

---

## [SRS-EP-40] Attachment warp-follow quality {#srs-ep-40-attachment-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-14](../../prd.md#connector-attachments). **Constrains:** [SRS-EP-38](./srs-logic.md#srs-ep-38-attachment-t).

| Scenario | Target |
|---|---|
| Bound SmartGroup move with attachment | Attachment stays on spine (`t` preserved) at ≥**5 Hz**; pen-up pose = last preview (**0 px** jump) |
| Undo box move | Connector **and** attachment pre-move pose ±1 px @ 100% zoom |
| Connector with 0 attachments | REQ-09 / SRS-EP-20 bars still hold (**0** regression) |
| Rest spine rebake during drag | **0** (ADR-0020 I1) |

### State coverage (dual-ask)

| State id | Designer | QA |
|---|---|---|
| `conn.blink` | required | required |
| `conn.selected` | required | required |
| `conn.rejected` | required | required |
| `conn.live_warp` | required | required |
| `conn.orphan` | required | required |
| `conn.end_style_*` | Infini / later ([SRS-EP-36](./srs-ui.md)) — **not Epaper this campaign** | Path B bars live on [SRS-EP-37](#srs-ep-37-endpoint-quality); Path A chrome is 0 on device |
| `attach.*` | required ([SRS-EP-39](./srs-ui.md)) | required ([SRS-EP-40](#srs-ep-40-attachment-quality)) |

## Non-goals

- Pixel-match the EXP contact sheet
- Shipping default-on before G1/G2 pass
