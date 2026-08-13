---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.5.0
lifecycle: active
---

# SRS — Tablet sync Infini (Quality)

Parent REQ: [REQ-03](../../prd.md#tablet-sync).

## [SRS-IN-08] Latency, consistency, and paint parity

<!-- revised: 2026-08-13 — CHL-0008 / ADR-0014 / ADR-0015. Parity is now measured as the mirror
     converging on published device changes, not the device raster matching a pushed snapshot.
     Same id, content revised; no supersession. -->

> **Revised 2026-08-13.** The old parity row asked whether the device's raster matched the picture
> Infini pushed. That question no longer exists — Infini pushes no picture. Parity is now:
> **does the mirror converge on what the device published, and does the desktop stay silent?**

| Scenario | Metric | Target |
|---|---|---|
| Preview stroke sample leaves Epaper → visible on Infini | p95 | ≤ 50 ms |
| Device commits an op → applied to the Infini mirror | p95 | ≤ 300 ms |
| Mirror vs device document, after settle | Figures ∩ region | 0 divergent figures; 0 divergent nodes |
| **Inbound document messages after `doc_load`** | Count per session | **0** — assert by message type, not by eye |
| Change stream integrity | Ops lost / reordered over a 10-op offline queue | 0 lost, 0 reordered |
| Replay idempotency | Same stream applied twice | Identical tree (dedupe by `opId`) |
| Reconnect convergence | After `queue_empty` → `doc_load` → `load_ack` | 0 divergent nodes; 0 queued changes discarded |
| Gap handling | `baseSeq` mismatch injected | Mirror marked suspect; resync requested; **suspect mirror never saved** |
| Viewport change → Epaper map applied | p95 | ≤ 100 ms to map apply; panel refresh may trail |
| Viewport publish under gesture spam | Outbound rate | ≤ 30 Hz; settle pose always flushed (`settle: true`) |
| Marker visibility | Idle vs gesturing | Hidden when idle; visible while gesturing; hide ~100 ms after settle |
| Preview hygiene | Orphaned preview paths after a dropped session | 0 persisted, 0 saved to the mirror |
| Stroke width parity | World × Infini scale vs world × `s_panel` | ≤ 5% relative error @ matched region scale |
| Zoom stroke feel | Halve Infini scale (or grow region world extent) | Apparent stroke thickness halves on Infini **and** Epaper (live + stored) |
| Gut orientation | Tall default / wide cycle | Vertical `gutToLeft` correct; wide not L/R mirrored |

### Dual-ask

REQ-03 Needs design: no — marker is a gesture affordance. The **device** carries the session /
publish status affordance ([SRS-EP-05](../../../epaper/features/tool-modes/srs-ui.md)), because that
is where unpublished work lives.
