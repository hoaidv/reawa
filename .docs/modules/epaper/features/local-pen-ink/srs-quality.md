---
feature: local-pen-ink
parent_req: [REQ-11]
version: 0.1.0
lifecycle: active
---

# SRS — Local pen-matched ink (Quality)

Ink-latency floor remains [SRS-EP-01](./srs-logic.md) p95 ≤30 ms for the **pen tip**. This file adds **erase** bars for [REQ-11](../../prd.md#erase). Path B also [SRS-EP-33](../device-document/srs-quality.md) via undo exactness in [SRS-EP-13](../device-document/srs-quality.md).

## [SRS-EP-30] Erase latency and correctness {#srs-ep-30-erase-quality}

<!-- lifecycle: active -->

**Parent:** [REQ-11](../../prd.md#erase). **Constrains:** [SRS-EP-27](./srs-logic.md#srs-ep-27-eraser-nib), [SRS-EP-28](../device-document/srs-logic.md#srs-ep-28-selection-erase).

| Field | Value |
|---|---|
| Source | Eraser nib / Erase command / barrel `temp_erase` |
| Stimulus | Rub or invoke Erase |
| Artifact | Ink samples / selected nodes |
| Environment | With or without session; with or without nib |
| Response | Samples/nodes gone or no-op |
| Response measure | See table |

| Scenario | Target |
|---|---|
| Nib rub across ink → intersecting samples gone | p95 ≤**50 ms** after gesture end |
| New Ink nodes created by Path A | **0** |
| Undo after Path A | Pre-erase document ±1 px @ 100% zoom |
| No eraser nib, pen tip | Path A fires **0** times |
| Selection-erase, non-empty | **0** leftover selected nodes on next settled frame; one undo restores |
| Selection-erase, empty | **0** nodes change |
| No session | Same local result as linked |

Does not relax SRS-EP-01 pen-tip latency.
