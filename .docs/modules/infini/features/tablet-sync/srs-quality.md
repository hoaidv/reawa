---
feature: tablet-sync
parent_req: [REQ-03]
version: 0.6.0
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
is where unpublished work lives. Device Log ([SRS-IN-18](./srs-ui.md#srs-in-18-device-log-panel))
is desktop debug chrome, also `needs_design: false`.

---

## [SRS-IN-19] Debug-log isolation and backpressure {#srs-in-19-debug-log-isolation}

Parent REQ: [REQ-03](../../prd.md#tablet-sync).
Constrains: [SRS-IN-17](./srs-logic.md#srs-in-17-debug-log-channel), [SRS-IN-18](./srs-ui.md#srs-in-18-device-log-panel).
Device peer: [SRS-EP-16](../../../epaper/features/device-document/srs-quality.md#srs-ep-16-debug-log-ship-quality).

Prioritised for this slice: **(1)** document/paint isolation **(2)** bounded memory
**(3)** overlay must not stall pan/zoom.

| Field | Value |
|---|---|
| Source | Infini paint / viewport flush / `doc_change` apply |
| Stimulus | Device Log panel open and `debug_log` arriving at ≥200 lines/s |
| Artifact | Canvas paint, viewport publish, VectorDocument applier |
| Environment | Normal session + debug sidecar both live |
| Response | Paint and ingest paths do **no** debug-port I/O; logs land on a side queue |
| Response measure | **0** TCP reads/writes on `:9878` from the canvas paint stack, viewport flush, or VectorDocument apply; p95 pan/zoom frame budget unchanged vs panel closed ([SRS-IN-03](../infinity-canvas/srs-quality.md) / [SRS-IN-08](#srs-in-08) viewport ≤30 Hz) |

| Scenario | Metric | Target |
|---|---|---|
| `:9878` line applied to `VectorDocument` | Mutations | **0** |
| ADR-0015 types arriving on `:9878` | Applied to mirror or viewport | **0** — dropped |
| `debug_*` arriving on `:9877` | Applied as document / viewport | **0** — protocol defect, existing unknown-type path |
| In-memory buffer | Cap | **10_000**; overflow drops oldest; **0** unbounded growth |
| Drop under backpressure (Infini) | Process crash / paint stall | **0**; oldest dropped |
| Overlay open / close | p95 click → overlay visible / hidden | ≤100 ms |
| Filter | Device messages sent | **0** — client-side only |

---

## [SRS-IN-25] Pen-button map publish quality {#srs-in-25-map-publish-quality}

<!-- lifecycle: active -->

**Parent:** Infini [REQ-05](../../prd.md#pen-button-map). **Constrains:** [SRS-IN-23](./srs-logic.md#srs-in-23-pen-map-publish). Does **not** steal [SRS-IN-08](#srs-in-08) parent (REQ-03).

| Scenario | Target |
|---|---|
| Save map → next tablet gesture uses it | p95 ≤**300 ms** after device apply; in-flight unchanged |
| Publish `pen_button_map` | **0** `doc_load` / `doc_change` / `doc_snapshot` for that publish |
| 0-button UI | Slots absent or disabled; **0** fake bindings |

---

## Superseded

_None. SRS-IN-19 is additive._
SRS-IN-25 is additive.
